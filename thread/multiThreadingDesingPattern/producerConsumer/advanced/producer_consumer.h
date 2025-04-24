#ifndef PRODUCER_CONSUMER_H
#define PRODUCER_CONSUMER_H

#include <pthread.h>

#define BUFFER_SIZE 8
#define NUM_PRODUCERS 5
#define NUM_CONSUMERS 2
#define MAX_ITEMS_PER_PRODUCER 5 // 한 producer가 생산할 개수

typedef struct
{
  int buffer[BUFFER_SIZE];
  int front;
  int rear;
  int count;

  pthread_mutex_t mutex;
  pthread_cond_t cond_not_full;
  pthread_cond_t cond_not_empty;

  volatile int done;  // 종료 플래그
  int total_produced; // 전체 생산 수 추적
} shared_data_t;

typedef struct
{
  int thread_id;
  shared_data_t *shared;
} thread_arg_t;

void queue_init(shared_data_t *data);
int is_empty(shared_data_t *data);
int is_full(shared_data_t *data);
void enqueue(shared_data_t *data, int item);
int dequeue(shared_data_t *data);

void *producer(void *arg);
void *consumer(void *arg);

#endif
