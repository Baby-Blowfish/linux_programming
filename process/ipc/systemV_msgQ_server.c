#include <stdio.h>
#include <unistd.h>           // write() 함수 사용을 위한 헤더
#include <sys/msg.h>          // System V 메시지 큐 관련 함수/구조체 포함

#define MSQKEY 51234          // 메시지 큐의 고유 키 값

// 메시지 구조체 정의
struct msgbuf {
    long mtype;               // 메시지 타입 (필수, 양수)
    char mtext[BUFSIZ];       // 메시지 본문 (BUFSIZ: 시스템 기본 버퍼 크기)
};

int main(int argc, char **argv)
{
    key_t key;
    int n, msqid;
    struct msgbuf mb;

    key = MSQKEY;             // 고정된 키 사용 (송신자와 일치해야 함)

    // 메시지 큐 생성: 없으면 생성하고, 있으면 오류 (IPC_EXCL)
    if ((msqid = msgget(key, IPC_CREAT | IPC_EXCL | 0666)) < 0)
    {
        perror("msgget()");   // 이미 큐가 존재할 경우 오류 발생
        return -1;
    }

    // 메시지를 계속 수신하는 루프
    while ((n = msgrcv(msqid, &mb, sizeof(mb), 0, 0)) > 0)
    {
        switch (mb.mtype)
        {
            case 1:
                // 타입 1 메시지를 출력 (화면에 출력)
                write(1, mb.mtext, n);
                break;

            case 2:
                // 타입 2 수신 시 메시지 큐 삭제
                if (msgctl(msqid, IPC_RMID, (struct msqid_ds *)0) < 0)
                {
                    perror("msgctl()");
                    return -1;
                }
                break;
        }
    }

    return 0;
}

