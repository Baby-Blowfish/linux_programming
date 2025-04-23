#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void task(void *arg)
{
  int id = *(int *)arg;
  printf("🧵 Task %d started\n", id);
  sleep(5); // 오래 걸리게 해서 큐가 차는 걸 유도
  printf("✅ Task %d completed\n", id);
  free(arg);
}

int main()
{
  thread_pool_t pool;
  thread_pool_init(&pool, 2, 2); // 2 threads, 2 queue slots

  printf("=== 🧪 Task Submission (expect timeout) ===\n");

  for (int i = 0; i < 7; i++) // 큐가 넘치도록 일부러 많이 넣음
  {
    int *arg = malloc(sizeof(int));
    *arg = i;

    int rc = thread_pool_add_timeout_retry(&pool, task, arg, 5, 1000); // retry 1회, 1초 타임아웃

    if (rc == 0)
    {
      printf("📌 Task %d submitted\n", i);
    }
    else if (rc == -2)
    {
      printf("⚠️ Task %d timed out after retries\n", i);
      free(arg);
    }
    else
    {
      printf("❌ Task %d add failed (shutdown)\n", i);
      free(arg);
    }
  }

  printf("🌙 All tasks attempted, sleeping 8s...\n");
  sleep(8); // 작업들이 일부 끝날 때까지 대기

  printf("=== 🧪 Shutdown 이후 작업 추가 시도 ===\n");

  thread_pool_destroy(&pool);

  int *arg = malloc(sizeof(int));
  *arg = 99;
  int rc = thread_pool_add_timeout_retry(&pool, task, arg, 1, 1000);
  if (rc == -1)
  {
    printf("❌ Task 99 add failed (shutdown confirmed)\n");
    free(arg);
  }

  return 0;
}
