#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


void * worker(void* arg)
{
  int id = *(int*)arg;
  for(int i = 0; i < 5; i++)
  {
    printf("Thread %d: %d\n",id,i);
    sleep(1);
  }

  return NULL;
}

int main()
{
  pthread_t tid[2];
  int ids[2] = {1,2};

  pthread_create(&tid[0], NULL, worker, &ids[0]);
  pthread_create(&tid[1], NULL, worker, &ids[1]);

  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  return 0;

}

