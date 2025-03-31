#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

void *worker_thread(void *arg);
void *signal_handler_thread(void *arg);

int main() {
    pthread_t tid_worker, tid_sig;

    // 1. 시그널 집합 생성 및 설정
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);   // Ctrl+C
    sigaddset(&set, SIGTERM);  // kill -15

    // 2. 프로세스 전체에서 시그널 block
    //    (모든 스레드가 이 마스크를 상속받음)
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    // 3. 스레드 생성
    pthread_create(&tid_worker, NULL, worker_thread, NULL);
    pthread_create(&tid_sig, NULL, signal_handler_thread, (void*)&set);

    // 4. 스레드 종료 대기
    pthread_join(tid_worker, NULL);
    pthread_join(tid_sig, NULL);

    return 0;
}

void *worker_thread(void *arg) {
    int count = 0;
    while (count < 10) {
        printf("[worker_thread] Working... %d\n", count++);
        sleep(1);
    }
    return NULL;
}

void *signal_handler_thread(void *arg) {
    sigset_t *set = (sigset_t *)arg;
    int sig;

    while (1) {
        // sigwait은 시그널이 발생할 때까지 블로킹됨
        int ret = sigwait(set, &sig);
        if (ret != 0) {
            perror("sigwait");
            pthread_exit(NULL);
        }

        // 수신된 시그널 처리
        if (sig == SIGINT) {
            printf("[signal_handler_thread] Received SIGINT (Ctrl+C)\n");
        } else if (sig == SIGTERM) {
            printf("[signal_handler_thread] Received SIGTERM (kill)\n");
        } else {
            printf("[signal_handler_thread] Received unknown signal: %d\n", sig);
        }

        // 시그널 하나만 받고 종료하고 싶으면 여기에 break 넣기
    }

    return NULL;
}

