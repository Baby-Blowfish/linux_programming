#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>

#define MQ_NAME "/hyojin_queue"

int main() {
    mqd_t mq;
    struct mq_attr attr;

    mq = mq_open(MQ_NAME, O_RDONLY);
    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    if (mq_getattr(mq, &attr) == -1) {
        perror("mq_getattr");
        exit(1);
    }

    printf("✅ 큐 상태:\n");
    printf(" - 최대 메시지 수: %ld\n", attr.mq_maxmsg);
    printf(" - 메시지 크기: %ld\n", attr.mq_msgsize);
    printf(" - 현재 메시지 수: %ld\n", attr.mq_curmsgs);

    mq_close(mq);
    return 0;
}

