#include <stdio.h>      // 표준 입출력 함수 사용
#include <string.h>     // 문자열 처리 함수 사용 (memset, strncmp 등)
#include <unistd.h>     // 유닉스 표준 함수 사용 (fork, pipe, close 등)
#include <stdlib.h>     // 표준 라이브러리 함수 사용 (exit 등)
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // signal()
#include <sys/socket.h> // 소켓 함수 사용 (socket, connect, send, recv 등)
#include <arpa/inet.h>  // 인터넷 프로토콜 관련 함수 (inet_pton 등)

// Signal setting
static void sigHandler(int);    // 시그널 처리용 핸들러

// Pipe setting
int child_to_parent[2];         // 파이프를 전역으로 선언

// Buffer setting
char buffer[BUFSIZ];            // 전역으로 버퍼 선언

// TCP Socket setting
#define TCP_PORT 5100   // 서버와 연결할 포트 번호
int ssock;       // 소켓 디스크립터


int main( int argc, char **argv) {

    // Process setting
    pid_t pid;  // 프로세스 ID
    int status; // 상태 코드

    // Buffer setting
    int readBufferByte;

    // TCP Socket setting
    struct sockaddr_in servaddr; // 서버 주소 정보 구조체


    //----------------------------------------------
    // 프로그램 실행 시 IP 주소가 입력되었는지 확인
    if (argc < 2) {
        printf("Usage : %s IP_ADDRESS\n", argv[0]); // 사용법 출력
        return -1;
    }

    // 자식 -> 부모로 보내는 파이프 생성
    if (pipe(child_to_parent) < 0) {
        perror("pipe()");
        return -1;
    }

    // 시그널 처리를 위한 핸들러 등록
    if(signal(SIGUSR1, sigHandler) == SIG_ERR)
    {
        perror("signal");
        return -1;
    }

    // 소켓 생성 (AF_INET: IPv4, SOCK_STREAM: TCP 통신, 0: 프로토콜 자동 선택)
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() :");    // 소켓 생성 실패 시 오류 출력
        return -1;
    }

    // 서버 주소 구조체 초기화 및 설정
    memset(&servaddr, 0, sizeof(servaddr)); // 구조체 초기화
    servaddr.sin_family = AF_INET;          // IPv4 주소 체계 설정
    inet_pton(AF_INET, argv[1], &(servaddr.sin_addr.s_addr)); // IP 주소 변환
    servaddr.sin_port = htons(TCP_PORT);    // 포트 번호 설정

    // 서버에 연결 시도
    if (connect(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect() :");   // 연결 실패 시 오류 출력
        return -1;
    }




    //----------------------------------------------
    // 부모 프로세스와 자식 프로세스로 분기
    if((pid = fork()) < 0)
    {
      perror("fork()"); // 포크 실패 시 오류 출력
      return -1;
    }
    else if(pid == 0) // // 자식 프로세스 ( 키보드로부터 메시지를 받는 역할)
    {
        // 자식 -> 부모 읽기 파이프 닫기
        close(child_to_parent[0]);
        // 자식 소켓 종료
        close(ssock);                       // 소켓 종료

        do
        {
            memset(buffer, 0, BUFSIZ);  // buffer 초기화

            // 키보드 입력 받기
            if((readBufferByte=read(0, buffer, sizeof(buffer))<0))
            {
                perror(" stdin read()");
            }

            // 자식 -> 부모 쓰기 파이프
            if(write(child_to_parent[1],buffer,strlen(buffer))<=0)
            {
                perror("child_to_parent pipe write()");
            }

            // 부모에게 SIGUSR1 시그널 보내기
            kill(getppid(), SIGUSR1);


        } while (strncmp(buffer,"/q",2));    // 로그인 로그아웃 flag 필요


        printf("child process terminating....\n");      // 자식 프로세스 종료 메시지 출력
        close(child_to_parent[1]);          // 자식 -> 부모 쓰기 파이프 종료
        exit(0);                            // 자식 프로세스 종료

    } else // 부모 프로세스 (서버로부터 메시지를 받고 화면 출력)
    {
        // 자식 -> 부모 쓰기 파이프 닫기
        close(child_to_parent[1]);

        do
        {
            memset(buffer, 0, BUFSIZ); // 버퍼 초기화

            // server와 연결된 soket의 응답 대기
            if ((readBufferByte = recv(ssock, buffer, BUFSIZ, 0)) < 0) {
                perror("parent soket read() :");   // 수신 실패 시 오류 출력
                return -1;
            }
            // 응답 받은 데이터 출력 printf
            if((write(1, buffer, strlen(buffer))<0))
            {
                perror(" stdout write()");
            }

        } while (strncmp(buffer,"/q",2));

        printf("parent process terminating...\n");      // 자식 프로세스 종료 메시지 출력
        // 자식 -> 부모 읽기 파이프 닫기
        close(child_to_parent[0]);
        // 부모 소켓 종료
        close(ssock);
        // 자식 프로세스 종료 대기
        waitpid(pid, &status, 0);
    }

    return 0;
}

// SIGUSR1 시그널 핸들러
static void sigHandler(int signo) {
    if (signo == SIGUSR1) {
        // 파이프에서 데이터 읽기
        memset(buffer, 0, BUFSIZ);
        if (read(child_to_parent[0], buffer, BUFSIZ) > 0) {
            // 서버로 메시지 전송 (비차단 모드로 전송)
            if (write(ssock, buffer, strlen(buffer)) <= 0) {
                perror("parent soket write()");    // 전송 실패 시 오류 출력
            }
        } else {
            perror("pipe read()");
        }
    }
}