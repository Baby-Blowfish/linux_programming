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
int client_count = 0;    // 연결된 클라이언트 수

typedef struct
{
    int csock;                      // 소켓 디스크립터
    pid_t pid;
    int mesgflag;
    int child_to_parent[2];         // 자식 -> 부모로 데이터를 전송하는 파이프
    int parent_to_child[2];         // 부모 -> 자식으로 데이터를 전송하는 파이프
    char *id;
    char *pw;
    char mesg[BUFSIZ];
} Client;

Client clients[MAX_CLIENTS];

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
        clients[client_count].csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
        if (clients[client_count].csock < 0) {
            perror("accept() :"); // 연결 수락 실패 시 오류 출력
            continue; // 에러 발생 시 다음 클라이언트 연결 처리로 진행
        }

        // 6. 클라이언트의 IP 주소를 문자열 형식으로 변환하여 출력
        memset(mesg, 0, BUFSIZ);  // 메시지 버퍼 초기화
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ); // 클라이언트 IP 출력
        printf("Client %d is connected: %s\n",client_count, mesg);

        // 자식 <-> 부모 파이프 생성
        if (pipe(clients[client_count].parent_to_child) < 0 || pipe(clients[client_count].child_to_parent) < 0) {
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
            close(clients[client_count].parent_to_child[1]);
            // 자식 -> 부모 읽기 파이프 닫기 (자식은 이쪽에서 읽지 않음)
            close(clients[client_count].child_to_parent[0]);
            // 부모 소켓 종료
            close(ssock);

            clients[client_count].pid = getppid();        // 자식프로세스는 부모의 pid 저장

            do {

                memset(clients[client_count].mesg, 0, BUFSIZ);  // 메시지 버퍼 초기화

                // 클라이언트로부터 데이터 수신 (read)
                if ((readBufferByte = read(clients[client_count].csock, clients[client_count].mesg, BUFSIZ)) <= 0) {
                    perror("csock read() :");   // 수신 실패 시 오류 메시지 출력
                    break;
                }

                printf("%d chiled Receved : %s",client_count,clients[client_count].mesg);

                clients[client_count].mesgflag = 1;             // 1 : 유효한 메시지.

                // 자식 -> 부모 파이프로 데이터 전송
                if (write(clients[client_count].child_to_parent[1], &clients[client_count], sizeof(clients[client_count])) < 0) {
                    perror("child_to_parent pipe write()"); // 전송 실패 시 오류 메시지 출력
                }

                    // 부모 프로세스에게 SIGUSR1 시그널을 보냄
                kill(clients[client_count].pid, SIGUSR1);


            } while (1); // "q" 또는 "w" 수신 시 루프 종료

            printf("Child %d process terminating\n",client_count);
            // 부모 -> 자식 읽기 파이프 종료
            close(clients[client_count].parent_to_child[0]);
            // 자식 -> 부모 쓰기 파이프 종료
            close(clients[client_count].child_to_parent[1]);
            close(clients[client_count].csock);  // 클라이언트 소켓 닫기
            exit(0);       // 자식 프로세스 종료
        }
        else
        // 부모 -> 자식 읽기 파이프 닫기 (부모는 이쪽에서 읽지 않음)
        close(clients[client_count].parent_to_child[0]);
        // 자식 -> 부모 쓰기 파이프 닫기 (부모는 이쪽에서 쓰지 않음)
        close(clients[client_count].child_to_parent[1]);

        clients[client_count].pid = pid;   // 부모프로세스는 자식의 pid 저장

        client_count++;

    } while((client_count) < MAX_CLIENTS);

    printf("Parent process terminating\n");

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        // 부모 -> 자식 쓰기 파이프 종료
        close(clients[i].parent_to_child[1]);
        // 자식 -> 부모 읽기 파이프 종료
        close(clients[i].child_to_parent[0]);
    }

    // 서버 소켓 닫기
    close(ssock);

    return 0;
}



// SIGUSR1 시그널 핸들러
static void sigHandler(int signo) {
    if (signo == SIGUSR1) {

        for (int i = 0; i < client_count; i++) {     // 어떤 자식 -> 부모 읽기 파이프에서 왔는지 확인

            memset(clients[i].mesg, 0, BUFSIZ);  // 버퍼 초기화
            clients[i].mesgflag = 0;             // mesgflag 초기화

            // Nonblock 처리
            int flags = fcntl(clients[i].child_to_parent[0], F_GETFL, 0);
            fcntl(clients[i].child_to_parent[0], F_SETFL, flags | O_NONBLOCK);

            // 논블로킹으로 read() 호출
            ssize_t n = read(clients[i].child_to_parent[0], &clients[i], sizeof(clients[i]));

            if(clients[i].mesgflag > 0) // 파이프로 읽은 데이터가 유효한 메시지라면
            {

                for (int j = 0; j < client_count; j++)  // 보낸 클라이언트 말고 다른 클라이언트에게 메시지를 전송하기
                {

                    if(j != i)
                    {
                        write(clients[j].csock, clients[i].mesg, strlen(clients[i].mesg));
                        printf(" send to %d soket\n",j);
                    }

                }
            }

        }

    }
}

