#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>

#define MQ_NAME "/hyojin_queue"
#define MAX_SIZE 128

int main() {
    mqd_t mq;
    char buffer[MAX_SIZE];

    // O_NONBLOCK 설정으로 대기 없이 즉시 응답
    mq = mq_open(MQ_NAME, O_RDONLY | O_NONBLOCK);
    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    int ret = mq_receive(mq, buffer, MAX_SIZE, NULL);
    if (ret == -1) {
        if (errno == EAGAIN) {
            printf("📭 큐가 비어있음: 메시지 없음\n");
        } else {
            perror("mq_receive");
        }
    } else {
        printf("📥 메시지 수신: %s\n", buffer);
    }

    mq_close(mq);
    return 0;
}

