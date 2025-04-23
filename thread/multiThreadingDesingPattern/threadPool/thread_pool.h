#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>

typedef struct
{
  void (*function)(void *);
  void *arg;
  int retry_count; // retry attempts
  int timeout_ms;  // max wait time to enqueue
} task_t;

typedef struct
{
  pthread_t *threads;
  task_t *queue;
  int queue_size;
  int front, rear, count;
  int shutdown;
  int dont_accept;
  int num_threads;

  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_cond_t queue_not_full;
} thread_pool_t;

int thread_pool_init(thread_pool_t *pool, int num_threads, int queue_size);
int thread_pool_add(thread_pool_t *pool, void (*function)(void *), void *arg);
int thread_pool_add_timeout_retry(thread_pool_t *pool, void (*function)(void *), void *arg,
                                  int retry_count, int timeout_ms);

void thread_pool_destroy(thread_pool_t *pool);

#endif