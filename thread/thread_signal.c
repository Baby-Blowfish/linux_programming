#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void *worker(void *arg) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT); // SIGINT 차단

    pthread_sigmask(SIG_BLOCK, &set, NULL);  // 현재 스레드에서 SIGINT block

    while (1) {
        printf("worker thread is running...\n");
        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_create(&tid, NULL, worker, NULL);

    sleep(3);

    printf("main thread sending SIGINT to process\n");
    kill(getpid(), SIGINT);  // 시그널은 프로세스 전체로 감

    pthread_join(tid, NULL);
    return 0;
}

