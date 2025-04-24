#include "producer_consumer.h"
#include <stdio.h>
#include <unistd.h>

void queue_init(shared_data_t *data)
{
  data->front = data->rear = data->count = 0;
  data->done = 0;
  data->total_produced = 0;
  pthread_mutex_init(&data->mutex, NULL);
  pthread_cond_init(&data->cond_not_full, NULL);
  pthread_cond_init(&data->cond_not_empty, NULL);
}

int is_empty(shared_data_t *data)
{
  return data->count == 0;
}

int is_full(shared_data_t *data)
{
  return data->count == BUFFER_SIZE;
}

void enqueue(shared_data_t *data, int item)
{
  data->buffer[data->rear] = item;
  data->rear = (data->rear + 1) % BUFFER_SIZE;
  data->count++;
}

int dequeue(shared_data_t *data)
{
  int item = data->buffer[data->front];
  data->front = (data->front + 1) % BUFFER_SIZE;
  data->count--;
  return item;
}

void *producer(void *arg)
{
  thread_arg_t *targ = (thread_arg_t *)arg;
  shared_data_t *data = targ->shared;

  for (int i = 0; i < MAX_ITEMS_PER_PRODUCER; i++)
  {
    sleep(1 + targ->thread_id % 2);

    pthread_mutex_lock(&data->mutex);

    while (is_full(data) && !data->done)
      pthread_cond_wait(&data->cond_not_full, &data->mutex);

    if (data->done)
    {
      pthread_mutex_unlock(&data->mutex);
      break;
    }

    enqueue(data, i);
    data->total_produced++;
    printf("🧃 Producer %d produced %d (count: %d)\n", targ->thread_id, i, data->count);

    pthread_cond_signal(&data->cond_not_empty);
    pthread_mutex_unlock(&data->mutex);
  }

  // 마지막 producer가 종료 조건 설정
  pthread_mutex_lock(&data->mutex);
  if (data->total_produced >= MAX_ITEMS_PER_PRODUCER * NUM_PRODUCERS)
  {
    data->done = 1;
    pthread_cond_broadcast(&data->cond_not_empty); // 기다리는 consumer 모두 깨움
  }
  pthread_mutex_unlock(&data->mutex);

  printf("✅ Producer %d exiting\n", targ->thread_id);

  return NULL;
}

void *consumer(void *arg)
{
  thread_arg_t *targ = (thread_arg_t *)arg;
  shared_data_t *data = targ->shared;

  while (1)
  {
    pthread_mutex_lock(&data->mutex);

    while (is_empty(data) && !data->done)
    {
      pthread_cond_wait(&data->cond_not_empty, &data->mutex);
    }

    if (data->done && is_empty(data))
    {
      pthread_mutex_unlock(&data->mutex);
      printf("🛑 Consumer %d exiting\n", targ->thread_id);
      break;
    }

    int item = dequeue(data);
    printf("🍽️ Consumer %d consumed %d (count: %d)\n", targ->thread_id, item, data->count);

    pthread_cond_signal(&data->cond_not_full);
    pthread_mutex_unlock(&data->mutex);

    sleep(1 + targ->thread_id % 2);
  }
  return NULL;
}
