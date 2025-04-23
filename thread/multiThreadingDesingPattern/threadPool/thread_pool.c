#include "thread_pool.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static void *worker_thread(void *arg)
{
  thread_pool_t *pool = (thread_pool_t *)arg;

  // 스레드 이름 지정
  char name[16];
  snprintf(name, sizeof(name), "worker-%ld", pthread_self() % 10000); // 고유 이름
  pthread_setname_np(pthread_self(), name);

  while (1)
  {
    pthread_mutex_lock(&pool->lock);
    while (pool->count == 0 && !pool->shutdown)
    {
      pthread_cond_wait(&pool->notify, &pool->lock);
    }

    if (pool->shutdown && pool->count == 0)
    {
      pthread_mutex_unlock(&pool->lock);
      break;
    }

    task_t task = pool->queue[pool->front];
    pool->front = (pool->front + 1) % pool->queue_size;
    pool->count--;

    pthread_cond_signal(&pool->queue_not_full);

    pthread_mutex_unlock(&pool->lock);
    task.function(task.arg);
  }

  return NULL;
}

int thread_pool_init(thread_pool_t *pool, int num_threads, int queue_size)
{
  pool->threads = malloc(sizeof(pthread_t) * num_threads);
  pool->queue = malloc(sizeof(task_t) * queue_size);
  pool->queue_size = queue_size;
  pool->front = pool->rear = pool->count = pool->shutdown = pool->dont_accept = 0;
  pool->num_threads = num_threads;

  pthread_mutex_init(&pool->lock, NULL);
  pthread_cond_init(&pool->notify, NULL);
  pthread_cond_init(&pool->queue_not_full, NULL);

  for (int i = 0; i < num_threads; i++)
  {
    pthread_create(&pool->threads[i], NULL, worker_thread, pool);
  }

  return 0;
}

int thread_pool_add(thread_pool_t *pool, void (*function)(void *), void *arg)
{
  pthread_mutex_lock(&pool->lock);

  // 큐가 가득 찼거나 종료 중이면 대기 or 종료 확인
  while ((pool->count == pool->queue_size || pool->dont_accept))
  {
    if (pool->dont_accept)
    {
      pthread_mutex_unlock(&pool->lock);
      return -1;
    }
    pthread_cond_wait(&pool->queue_not_full, &pool->lock);
  }

  pool->queue[pool->rear].function = function;
  pool->queue[pool->rear].arg = arg;
  pool->rear = (pool->rear + 1) % pool->queue_size;
  pool->count++;

  pthread_cond_signal(&pool->notify);
  pthread_mutex_unlock(&pool->lock);
  return 0;
}

int thread_pool_add_timeout_retry(thread_pool_t *pool, void (*function)(void *), void *arg,
                                  int retry_count, int timeout_ms)
{
  int attempts = 0;

  while (attempts <= retry_count)
  {
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += timeout_ms / 1000;
    abstime.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (abstime.tv_nsec >= 1000000000)
    {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= 1000000000;
    }

    pthread_mutex_lock(&pool->lock);

    if (pool->dont_accept)
    {
      pthread_mutex_unlock(&pool->lock);
      return -1;
    }

    while (pool->count == pool->queue_size && !pool->dont_accept)
    {
      int rc = pthread_cond_timedwait(&pool->queue_not_full, &pool->lock, &abstime);
      if (rc == ETIMEDOUT)
      {
        pthread_mutex_unlock(&pool->lock);
        attempts++;
        if (attempts > retry_count)
          return -2; // timeout+retry 실패
        sleep(1);    // backoff
        goto retry;
      }
    }

    if (pool->dont_accept)
    {
      pthread_mutex_unlock(&pool->lock);
      return -1;
    }

    pool->queue[pool->rear].function = function;
    pool->queue[pool->rear].arg = arg;
    pool->queue[pool->rear].retry_count = retry_count;
    pool->queue[pool->rear].timeout_ms = timeout_ms;
    pool->rear = (pool->rear + 1) % pool->queue_size;
    pool->count++;

    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
    return 0;

  retry:
    continue;
  }

  return -2;
}

void thread_pool_destroy(thread_pool_t *pool)
{
  pthread_mutex_lock(&pool->lock);
  pool->dont_accept = 1;
  pool->shutdown = 1;
  pthread_cond_broadcast(&pool->notify);
  pthread_mutex_unlock(&pool->lock);

  for (int i = 0; i < pool->num_threads; i++)
  {
    pthread_join(pool->threads[i], NULL);
  }

  free(pool->threads);
  free(pool->queue);

  pthread_mutex_destroy(&pool->lock);
  pthread_cond_destroy(&pool->notify);
  pthread_cond_destroy(&pool->queue_not_full);
}