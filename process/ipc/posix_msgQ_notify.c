#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define MQ_NAME "/hyojin_queue"
#define MAX_SIZE 128

// 시그널 핸들러
void handler(int signo) {
    mqd_t mq = mq_open(MQ_NAME, O_RDONLY);
    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    char buffer[MAX_SIZE];
    if (mq_receive(mq, buffer, MAX_SIZE, NULL) > 0) {
        printf("🔔 시그널 알림 수신: %s\n", buffer);
    }

    mq_close(mq);
}

int main() {
    mqd_t mq;
    struct sigevent sev;

    // 시그널 등록
    signal(SIGUSR1, handler);

    // 큐 열기
    mq = mq_open(MQ_NAME, O_RDONLY);
    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    // mq_notify 설정: 메시지 도착 시 SIGUSR1 보내기
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    if (mq_notify(mq, &sev) == -1) {
        perror("mq_notify");
        exit(1);
    }

    printf("⏳ 메시지를 기다리는 중 (SIGUSR1)...\n");

    while (1) {
        pause();  // 시그널을 기다림
    }

    mq_close(mq);
    return 0;
}


