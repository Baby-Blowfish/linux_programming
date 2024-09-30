#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

sem_t *sem;
static int cnt = 8;

void p()
{
    cnt--;
    sem_wait(sem);
}

void v()
{
    cnt++;
    sem_post(sem);
}

int main(int argc, char **argv)
{
    const char* name = "posix_sem";
    unsigned int value = 8;
    int s_value;
    sem = sem_open(name, O_CREAT,S_IRUSR|S_IWUSR,value);
  
    while(1)
    {
        if(cnt <= 0)
        {
            v();
            printf("increas : %d     sem value : %d\n", cnt,s_value);
            usleep(1000);
            break;
        }
        else
        {
            p();
            sem_getvalue(sem,&s_value);
            printf("decreas : %d    sem value : %d\n", cnt,s_value);
            usleep(1000);
            
        }
    }

    sem_close(sem);
    printf("sem_destroy return value : %d\n",sem_destroy(sem));

    sem_unlink(name);

    return 0;
}
