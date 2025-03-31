#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <string.h>

#define MQ_NAME "/hyojin_queue"
#define MAX_SIZE 128

int main() {
    mqd_t mq;
    struct mq_attr attr;

    // 메시지 큐 속성 설정
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    // 메시지 큐 열기 (없으면 생성)
    mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, 0644, &attr);
    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }

    char message[MAX_SIZE];
    printf("보낼 메시지 입력: ");
    fgets(message, MAX_SIZE, stdin);

    if (mq_send(mq, message, strlen(message) + 1, 0) == -1) {
        perror("mq_send");
        exit(1);
    }

    printf("✅ 메시지 전송 완료: %s", message);
    mq_close(mq);

    return 0;
}

