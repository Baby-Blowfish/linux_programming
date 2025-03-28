#include <stdio.h>      // 표준 입출력 함수 사용
#include <string.h>     // 문자열 처리 함수 사용 (memset, strncmp 등)
#include <unistd.h>     // 유닉스 시스템 호출 함수 사용 (fork, pipe, read, write 등)
#include <stdlib.h>     // 표준 라이브러리 함수 사용 (exit 등)
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // signal()
#include <sys/socket.h> // 소켓 함수 사용 (socket, bind, listen, accept 등)
#include <arpa/inet.h>  // 인터넷 프로토콜 관련 함수 사용 (inet_ntop 등)
#include <fcntl.h>
#include <errno.h>

#define TCP_PORT 5100   // 서버가 사용할 TCP 포트 정의
#define MAX_CLIENTS 8   // 최대 클라이언트 수

// Signal setting
static void sigHandler(int);    // 시그널 처리용 핸들러

// Buffer setting
char mesg[BUFSIZ];            // 전역으로 버퍼 선언

// TCP Socket setting
int ssock;              // 서버 소켓 디스크립터
int csock;

int child_to_parent[2];         // 자식 -> 부모로 데이터를 전송하는 파이프
int parent_to_child[2];         // 부모 -> 자식으로 데이터를 전송하는 파이프

int main(int argc, char **argv)
{
    // Process setting
    pid_t pid;  // 프로세스 ID
    int status; // 상태 코드

     // Buffer setting
    int readBufferByte;

    // TCP Socket Setting
    socklen_t clen;      // 클라이언트 주소 구조체 크기
    struct sockaddr_in servaddr, cliaddr;  // 서버 및 클라이언트 주소 구조체





//------------------------------------------------------------------------------------------------------

    // 시그널 처리를 위한 핸들러 등록
    if(signal(SIGUSR1, sigHandler) == SIG_ERR)
    {
        perror("signal");
        return -1;
    }
    // 1. 서버 소켓 생성 (AF_INET: IPv4, SOCK_STREAM: TCP, 0: 프로토콜 자동 선택)
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() :");    // 소켓 생성 실패 시 오류 메시지 출력
        return -1;
    }

    // 2. 서버 주소 설정 및 초기화
    memset(&servaddr, 0, sizeof(servaddr));     // 서버 주소 구조체 초기화
    servaddr.sin_family = AF_INET;              // IPv4 주소 체계 설정
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 모든 IP 주소에서 연결 허용
    servaddr.sin_port = htons(TCP_PORT);        // 서버 포트 번호 설정

    // 3. 서버 소켓을 지정한 주소와 포트에 바인딩
    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind() :");       // 바인딩 실패 시 오류 메시지 출력
        return -1;
    }

    // 4. 연결 대기 (대기열 최대 크기: 8)
    if (listen(ssock, 8) < 0) {
        perror("listen() : ");    // 대기 상태 전환 실패 시 오류 메시지 출력
        return -1;
    }

    clen = sizeof(cliaddr); // 클라이언트 주소 구조체 크기 초기화




//----------------------------------------------------------------------------------------------------
    // 서버는 클라이언트 연결을 반복적으로 처리
    do
    {
        // 5. 클라이언트 연결 수락 (accept)
        memset(&cliaddr, 0, clen); // 구조체 초기화
        csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
        if (csock < 0) {
            perror("accept() :"); // 연결 수락 실패 시 오류 출력
            continue; // 에러 발생 시 다음 클라이언트 연결 처리로 진행
        }

        // 6. 클라이언트의 IP 주소를 문자열 형식으로 변환하여 출력
        memset(mesg, 0, BUFSIZ);  // 메시지 버퍼 초기화
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ); // 클라이언트 IP 출력
        printf("Client %d is connected: %s\n",csock, mesg);

        // 자식 <-> 부모 파이프 생성
        if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
            perror("pipe()");
            return -1;
        }

        // 7. 프로세스 포크 (자식 프로세스 생성)
        if ((pid = fork()) < 0) {
            perror("fork()");     // 프로세스 생성 실패 시 오류 메시지 출력
            return -1;
        }
        else if (pid == 0) // 자식 프로세스 (클라이언트로부터 메시지 수신)
        {
            // 부모 -> 자식 쓰기 파이프 닫기 (자식은 이쪽에서 쓰지 않음)
            close(parent_to_child[1]);
            // 자식 -> 부모 읽기 파이프 닫기 (자식은 이쪽에서 읽지 않음)
            close(child_to_parent[0]);
            // 부모 소켓 종료
            close(ssock);


            do {
                memset(mesg, 0, BUFSIZ);  // 메시지 버퍼 초기화

                // 클라이언트로부터 데이터 수신 (read)
                if ((readBufferByte = read(csock, mesg, BUFSIZ)) <= 0) {
                    perror("csock read() :");   // 수신 실패 시 오류 메시지 출력
                    break;
                }

                printf("chiled Receved : %s",mesg);

                // 자식 -> 부모 파이프로 데이터 전송
                if (write(child_to_parent[1], mesg, strlen(mesg)) < 0) {
                    perror("child_to_parent pipe write()"); // 전송 실패 시 오류 메시지 출력
                }

                // 부모 프로세스에게 SIGUSR1 시그널을 보냄
                kill(getppid(), SIGUSR1);

            } while (1); // "q" 또는 "w" 수신 시 루프 종료

            printf("Child %d process terminating\n",csock);
            // 부모 -> 자식 읽기 파이프 종료
            close(parent_to_child[0]);
            // 자식 -> 부모 쓰기 파이프 종료
            close(child_to_parent[1]);
            close(csock);  // 클라이언트 소켓 닫기
            exit(0);       // 자식 프로세스 종료
        }
        else
        // 부모 -> 자식 읽기 파이프 닫기 (부모는 이쪽에서 읽지 않음)
        close(parent_to_child[0]);
        // 자식 -> 부모 쓰기 파이프 닫기 (부모는 이쪽에서 쓰지 않음)
        close(child_to_parent[1]);



    } while(1);

    printf("Parent process terminating\n");

        // 부모 -> 자식 쓰기 파이프 종료
        close(parent_to_child[1]);
        // 자식 -> 부모 읽기 파이프 종료
        close(child_to_parent[0]);

    // 서버 소켓 닫기
    close(ssock);

    return 0;
}

#include <fcntl.h>
#include <errno.h>

// SIGUSR1 시그널 핸들러
static void sigHandler(int signo) {
    if (signo == SIGUSR1) {
        //printf("recevied Signal\n");
        // 자식 -> 부모 읽기 파이프에서 인덱스와 메시지 읽기
        memset(mesg, 0, BUFSIZ);  // 버퍼 초기화

        if ((read(child_to_parent[0], mesg, sizeof(mesg))) > 0)
        {
            if (send(csock, mesg, BUFSIZ, MSG_DONTWAIT) < 0)
            {
                perror("parent socket send()");    // 전송 실패 시 오류 출력
            }
        }
        else perror("parent pipe read()");    // 전송 실패 시 오류 출력
    }
}

