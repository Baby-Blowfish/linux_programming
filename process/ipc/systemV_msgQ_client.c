#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/msg.h>           // System V 메시지 큐 관련 헤더

#define MSQKEY 51234           // 서버와 동일한 메시지 큐 키 사용

// 메시지 구조체 정의
struct msgbuf {
    long mtype;                // 메시지 타입 (필수, long형, 양수)
    char mtext[BUFSIZ];        // 메시지 본문
};

int main(int argc, char **argv)
{
    key_t key;
    int rc, msqid;
    char* msg_text = "hello world\n";   // 전송할 메시지
    struct msgbuf *mb;

    // 메시지 구조체 동적 할당
    mb = (struct msgbuf*)malloc(sizeof(struct msgbuf));

    // 고정된 키를 사용
    key = MSQKEY;

    // 메시지 큐에 접근 (이미 생성되어 있어야 함)
    if ((msqid = msgget(key, 0666)) < 0)
    {
        perror("msgget()");
        return -1;
    }

    // ------------------------
    // 1. 일반 메시지 전송 (mtype = 1)
    // ------------------------
    mb->mtype = 1;                        // 메시지 타입 1번
    strcpy(mb->mtext, msg_text);         // 본문 복사
    rc = msgsnd(msqid, mb, strlen(msg_text) + 1, 0);  // +1: null 문자까지 전송

    if (rc == -1)
    {
        perror("msgsnd()");
        return -1;
    }

    // ------------------------
    // 2. 종료 신호 전송 (mtype = 2)
    // ------------------------
    mb->mtype = 2;                        // 메시지 타입 2번 (종료 명령)
    memset(mb->mtext, 0, sizeof(mb->mtext));  // 텍스트 초기화 (빈 메시지 전송)
    rc = msgsnd(msqid, mb, strlen(msg_text), 0);  // 여기선 strlen(msg_text)지만, 실제로는 0 바이트 전송됨

    if (rc == -1)
    {
        perror("msgsnd()");
        return -1;
    }

    return 0;
}

