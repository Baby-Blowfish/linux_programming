#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>


sem_t *sem;
static int cnt = 7;
static bool isRun = true;

void p()
{
    sem_wait(sem);
    cnt--;
}

void v()
{
    sem_post(sem);
    cnt++;
}

void *pthreadV(void *arg)
{
    while(isRun)
    {
        usleep(200000);
        v();
        cnt++;
        printf("increase : %d\n", cnt);
        fflush(stdout);
    } 
    return NULL;
}

void *pthreadP(void *arg)
{
    
    while(isRun)
    {
        p();
        cnt--;
        printf("decrease : %d\n", cnt);
        fflush(stdout);
        if(cnt <= 0) isRun=false;
        usleep(100000);
    }    
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t ptV, ptP;

    const char* name = "posix_sem";
    unsigned int value = cnt;

    sem = sem_open(name, O_CREAT,S_IRUSR | S_IWUSR, value);

    if(pthread_create(&ptV, NULL, pthreadV,NULL)<0)
    {
        perror("ptV :");
        return -1;
    }
    if(pthread_create(&ptP, NULL, pthreadP,NULL)<0)
    {
        perror("ptP :");
        return -1;
    }
    pthread_join(ptV,NULL);
    pthread_join(ptP,NULL);

    sem_close(sem);
    printf("sem_destroy() : %d\n",sem_destroy(sem));

    sem_unlink(name);
    
    return 0;
}


    
