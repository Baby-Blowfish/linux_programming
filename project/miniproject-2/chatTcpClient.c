#include <stdio.h>      // 표준 입출력 함수 사용
#include <string.h>     // 문자열 처리 함수 사용 (memset, strncmp 등)
#include <unistd.h>     // 유닉스 표준 함수 사용 (fork, pipe, close 등)
#include <stdlib.h>     // 표준 라이브러리 함수 사용 (exit 등)
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // signal()
#include <sys/socket.h> // 소켓 함수 사용 (socket, connect, send, recv 등)
#include <arpa/inet.h>  // 인터넷 프로토콜 관련 함수 (inet_pton 등)



// Signal setting
static void sigHandlerParent(int);    // 서버 측 signal 핸들러 (부모 프로세스가 처리)
static void sigHandlerChild(int);     // 클라이언트 측 signal 핸들러 (자식 프로세스가 처리)

// TCP Socket setting
#define TCP_PORT 5100   // 서버와 연결할 포트 번호
int ssock;       // 소켓 디스크립터

// 클라이언트 정보 구조체 정의
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50

typedef struct {
    int action; 
    // 1: 회원가입, 2: 로그인 요청, 3: 로그아웃 요청, 4: 채팅종료
    // 5 : 기존에 로그인된 유저가 있음, 6 : 비밀번호 틀림 7 :로그인 성공
    // 9 : 채팅 on, 10 : logout, chat off
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    char mesg[BUFSIZ];              // 클라이언트 메시지 버퍼 (클라이언트에서 수신된 데이터를 저장)
    int child_to_parent[2];         // 자식 -> 부모로 데이터를 전송하는 파이프
    int parent_to_child[2];         // 부모 -> 자식으로 데이터를 전송하는 파이프
    pid_t pid;                      // 자식 프로세스 ID (클라이언트 연결을 처리하는 자식 프로세스 ID)

    //----- 클아이언트에서는 아래의 값을 사용하지 않음!
    int csock;                      // 클라이언트 소켓 디스크립터 (클라이언트와 서버가 통신하는 소켓)
    int client_id;                  // 클라이언트 고유 ID (서버에서 구분하기 위한 ID)
} __attribute__((packed, aligned(4))) Client;  // 4바이트로 정렬

Client client;  // 전역변수로 선언



//  함수 선언
void showMenu();
void loginrequest(int action);
void logoutrequest(int action);
int chatOn_flag;    //  1 : 채팅 진행       , 0 : 채팅 종료
int childFlag = 1;      //  1 : 자식프로세스 진행, 0 : 자식프로세스 종료

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

    // Signal 설정 (서버 측에서 SIGUSR1, 클라이언트 측에서 SIGUSR2 신호를 받도록 설정)
    if (signal(SIGUSR1, sigHandlerParent) == SIG_ERR) {    // 서버가 SIGUSR1 신호를 받을 때 처리
        perror("signal");
        return -1;
    }
    if (signal(SIGUSR2, sigHandlerChild) == SIG_ERR) {    // 클라이언트가 SIGUSR2 신호를 받을 때 처리
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


// 7. 부모-자식 간 통신을 위한 파이프 생성 (부모->자식, 자식->부모 간 통신을 위한 파이프)
    if (pipe(client.parent_to_child) < 0 || pipe(client.child_to_parent) < 0) {
        perror("pipe()");
        close(ssock);  // 소켓 닫기
        return -1;
    }


    //----------------------------------------------
    // 부모 프로세스와 자식 프로세스로 분기
    if((pid = fork()) < 0)
    {
        perror("fork()"); // 포크 실패 시 오류 출력
        close(ssock);  // 소켓 닫기
        close(client.parent_to_child[0]);  // 파이프 닫기
        close(client.parent_to_child[1]);
        close(client.child_to_parent[0]);
        close(client.child_to_parent[1]);
        return -1;
    }
    else if(pid == 0) // 자식 프로세스: 메뉴 입력을 받고 이를 부모에게 전달
    {
        close(client.parent_to_child[1]);  // 부모->자식 쓰기 파이프 닫기 (자식에서 쓰지 않음)
        close(client.child_to_parent[0]);  // 자식->부모 읽기 파이프 닫기 (자식에서 읽지 않음)
        close(ssock);                       // 소켓 종료
        client.pid = getppid();  // 부모 프로세스의 PID를 저장

       int choice;

        do
        {
            if(chatOn_flag == 1)    // 채팅이 진행중일 경우
            {
                // 키보드 입력 받기
                if((readBufferByte=read(0, client.mesg, sizeof(BUFSIZ))<0))
                {
                    perror(" stdin read()");
                }

                if(!strncmp(client.mesg,"/q",2))    // 채팅 종료
                {
                    chatOn_flag == 0;
                    client.action = 4;
                }
                // 자식 -> 부모 쓰기 파이프
                if(write(client.child_to_parent[1],&client,sizeof(Client))<=0)
                {
                    perror("1 child_to_parent pipe write()");
                }

                // 부모에게 SIGUSR1 시그널 보내기
                kill(client.pid, SIGUSR1);

            }
            else
            {
                showMenu();  // 메뉴를 보여주기

                if (scanf("%d", &choice) != 1) {  // 입력 오류 처리
                    printf("Invalid input. Please enter a number.\n");
                    while (getchar() != '\n');  // 잘못된 입력이 있을 경우 버퍼를 비움
                    continue;
                }

                switch (choice) {
                    // 1: 회원가입, 2: 로그인, 3: 로그아웃(프로그램 종료)
                    case 1:
                        //
                        break;
                    case 2:     // 로그인 요청
                        loginrequest(2);
                        break;
                    case 3:
                        logoutrequest(3);
                        childFlag = 0;
                        break;
                    default:
                        printf("Invalid option.\n");
                        break;
                }

            }

        } while (childFlag);    // 로그인 로그아웃 flag 필요


        printf("child process terminating....\n");      // 자식 // 자원 정리 및 종료 (파이프와 소켓 닫기)
        close(client.parent_to_child[0]);
        close(client.child_to_parent[1]);
        close(ssock);
        exit(0);                            // 자식 프로세스 종료

    } else // 부모 프로세스 (서버로부터 메시지를 받고 화면 출력)
    {
        close(client.parent_to_child[0]);  // 부모->자식 읽기 파이프 닫기 (부모에서 읽지 않음)
        close(client.child_to_parent[1]);  // 자식->부모 쓰기 파이프 닫기 (부모에서 쓰지 않음)
        client.pid = pid;  // 자식 프로세스의 PID를 저장
        Client recvCli;

        do
        {
            // server와 연결된 soket의 응답 대기
            if ((readBufferByte = recv(ssock, &recvCli, sizeof(Client), 0)) < 0) {
                perror("parent soket read() :");   // 수신 실패 시 오류 출력
                return -1;
            }
            if((write(1, recvCli.mesg, strlen(recvCli.mesg))<0))
            {
                perror(" stdout write()");
            }

           write(client.parent_to_child[1],&recvCli,sizeof(Client));
            kill(client.pid, SIGUSR2);  // SIGUSR2 신호로 갱신된 정보를 전달



        } while (strncmp(recvCli.mesg,"/q",2));

        printf("parent process terminating...\n");      // 자식 프로세스 종료 메시지 출력
        close(client.parent_to_child[1]);  // 부모->자식 쓰기 파이프 닫기
        close(client.child_to_parent[0]);  // 자식->부모 읽기 파이프 닫기
        // 부모 소켓 종료
        close(ssock);
        // 자식 프로세스 종료 대기
        waitpid(pid, &status, 0);
    }

    return 0;
}

// (부모 프로세스가 자식 프로세스의 메시지를 처리하는 함수)
static void sigHandlerParent(int signo) {
    if (signo == SIGUSR1) {

        Client recvCli;

        // 파이프에서 데이터 읽기
        if (read(client.child_to_parent[0], &recvCli, sizeof(Client)) > 0) {
            // 서버로 메시지 전송 (비차단 모드로 전송)
            if (write(ssock, &recvCli, sizeof(Client)) <= 0) {
                perror("parent soket write()");    // 전송 실패 시 오류 출력
            }
        } else {
            perror("pipe read()");
        }
    }
}

// (자식 프로세스가 부모 프로세스의 메시지를 처리하는 함수)
static void sigHandlerChild(int signo) {
    if (signo == SIGUSR2) {

        Client recvCli;

        // 부모에서 자식으로 전달된 메시지 읽기
        if (read(client.parent_to_child[0], &recvCli, sizeof(Client)) > 0) {  // 부모->자식 파이프에서 데이터 읽기

            switch (recvCli.action)
            {
                // 1: 회원가입, 2: 로그인, 3: 로그아웃(클라이언트 종료), 4: 채팅 종료
                case 5:
                case 6:     // 로그인 요청
                    chatOn_flag = 0;
                    break;
                case 7:
                    chatOn_flag = 1;
                    break;
                default:
                    printf("Invalid option.\n");
                    break;
            }
        } else {
            perror("pipe read()");  // 읽기 실패 시 오류 메시지 출력
        }
    }
}

// 메뉴를 출력하고 사용자의 선택에 따라 함수를 실행
void showMenu() {

    printf("+++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("                  VEDA Talk                  \n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("  1. Register                                \n");
    printf("  2. Log In                                  \n");
    printf("  3. Log Out                                 \n");
    printf("  4. exit Chat                               \n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++\n");
    printf(" What do you wanna do? ");

}


// 서버에 데이터 보내기 함수
void loginrequest(int action) {

    client.action = action;

    printf("Enter username : ");
    scanf("%s", client.username);
    printf("Enter password : ");
    scanf("%s", client.password);

    // 자식 -> 부모 쓰기 파이프
    if(write(client.child_to_parent[1],&client,sizeof(Client))<=0)
    {
        perror("2 child_to_parent pipe write()");
    }

    // 부모에게 SIGUSR1 시그널 보내기
    kill(client.pid, SIGUSR1);

    while (getchar() != '\n');  // '\n'을 만날 때까지 입력을 무시합니다

    // 메시지를 출력합니다
    printf("If you go back or start chatting, please press Enter!\n");

    // 엔터 키가 눌릴 때까지 기다립니다
    getchar();  // 엔터가 눌리면 종료

}

// 서버에 데이터 보내기 함수
void logoutrequest(int action) {

    client.action = action;

    strcpy(client.mesg,"/q");
    client.action = 3;
    // 자식 -> 부모 쓰기 파이프
    if(write(client.child_to_parent[1],&client,sizeof(Client))<=0)
    {
        perror("3 child_to_parent pipe write()");
    }

    // 부모에게 SIGUSR1 시그널 보내기
    kill(client.pid, SIGUSR1);

    while (getchar() != '\n');  // '\n'을 만날 때까지 입력을 무시합니다

    // 메시지를 출력합니다
    printf("If you want next action, please press Enter!\n");

    // 엔터 키가 눌릴 때까지 기다립니다
    getchar();  // 엔터가 눌리면 종료

}
