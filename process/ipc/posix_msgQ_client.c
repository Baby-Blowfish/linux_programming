#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>

#define MQ_NAME "/hyojin_queue"
#define MAX_SIZE 128

int main() {
    mqd_t mq;
    char buffer[MAX_SIZE];

    // 메시지 큐 열기 (읽기 전용)
    mq = mq_open(MQ_NAME, O_RDONLY);
    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    // 메시지 수신
    if (mq_receive(mq, buffer, MAX_SIZE, NULL) == -1) {
        perror("mq_receive");
        exit(1);
    }

    printf("📥 수신된 메시지: %s\n", buffer);

    mq_close(mq);
    mq_unlink(MQ_NAME);  // 메시지 큐 제거
    return 0;
}

