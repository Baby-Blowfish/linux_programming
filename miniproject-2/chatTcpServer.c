#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#define TCP_PORT 5100   // 사용할 TCP 포트 번호
#define MAX_CLIENTS 8   // 최대 클라이언트 수

// Signal 핸들러 선언
static void sigHandlerParent(int);    // 서버 측 signal 핸들러 (부모 프로세스가 처리)
static void sigHandlerChild(int);     // 클라이언트 측 signal 핸들러 (자식 프로세스가 처리)

// 버퍼 설정
char mesg[BUFSIZ];            // 메시지 버퍼, 클라이언트에서 송수신되는 데이터 저장용

// TCP 소켓 설정
int ssock;              // 서버 소켓 디스크립터
int client_count = 0;    // 현재 접속된 클라이언트 수
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50

// 클라이언트 정보 구조체 정의
typedef struct {
    int action;  // 1: 회원가입, 2: 로그인, 3: 로그아웃, 4: 채팅종료
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    char mesg[BUFSIZ];              // 클라이언트 메시지 버퍼 (클라이언트에서 수신된 데이터를 저장)
    int child_to_parent[2];         // 자식 -> 부모로 데이터를 전송하는 파이프
    int parent_to_child[2];         // 부모 -> 자식으로 데이터를 전송하는 파이프
    pid_t pid;                      // 자식 프로세스 ID (클라이언트 연결을 처리하는 자식 프로세스 ID)
    int csock;                      // 클라이언트 소켓 디스크립터 (클라이언트와 서버가 통신하는 소켓)
    int client_id;                  // 클라이언트 고유 ID (서버에서 구분하기 위한 ID)

} __attribute__((packed, aligned(4))) Client;  // 4바이트로 정렬

Client* clients[MAX_CLIENTS];  // 동적으로 클라이언트를 관리하기 위한 포인터 배열

// 클라이언트 제거 함수 선언
void remove_client(int index); // 클라이언트가 종료될 때 클라이언트를 목록에서 제거하는 함수



// longinout 관련 함수
void login(int i);
void logout(int i);
void chat_on(int i);
int childFlag = 1;

int main(int argc, char **argv) {

    // 프로세스 및 TCP 소켓 관련 변수 설정
    pid_t pid;  // 자식 프로세스 ID를 저장할 변수
    int status; // 자식 프로세스 상태 저장
    int readBufferByte; // 읽은 데이터 크기를 저장할 변수

    // TCP 소켓 주소 관련 변수 설정
    socklen_t clen;      // 클라이언트 주소 구조체 크기
    struct sockaddr_in servaddr, cliaddr;  // 서버 및 클라이언트 주소 구조체

    // Signal 설정 (서버 측에서 SIGUSR1, 클라이언트 측에서 SIGUSR2 신호를 받도록 설정)
    if (signal(SIGUSR1, sigHandlerParent) == SIG_ERR) {    // 서버가 SIGUSR1 신호를 받을 때 처리
        perror("signal");
        return -1;
    }
    if (signal(SIGUSR2, sigHandlerChild) == SIG_ERR) {    // 클라이언트가 SIGUSR2 신호를 받을 때 처리
        perror("signal");
        return -1;
    }

    // 1. 서버 소켓 생성
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  // TCP 소켓 생성 (IPv4, TCP 사용)
        perror("socket() :");
        return -1;
    }

    // 2. 서버 소켓 주소 설정
    memset(&servaddr, 0, sizeof(servaddr));     // 서버 주소 초기화 (모든 필드를 0으로 초기화)
    servaddr.sin_family = AF_INET;              // IPv4 프로토콜 사용
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 모든 IP 주소에서 접속을 허용 (0.0.0.0)
    servaddr.sin_port = htons(TCP_PORT);        // 지정된 포트 번호를 사용 (5100번 포트)

    // 3. 소켓과 주소를 바인딩 (서버 소켓을 지정된 주소와 연결)
    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind() :");
        return -1;
    }

    // 4. 클라이언트 연결 대기 (최대 8개의 클라이언트 연결 대기)
    if (listen(ssock, 8) < 0) {
        perror("listen() : ");
        return -1;
    }

    clen = sizeof(cliaddr); // 클라이언트 주소 구조체의 크기를 설정

    do {
        // 5. 클라이언트 연결 수락 (accept 호출로 클라이언트 연결을 대기)
        memset(&cliaddr, 0, clen);  // 클라이언트 주소를 초기화
        int new_sock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);  // 클라이언트 연결 수락
        if (new_sock < 0) {
            perror("accept() :");
            continue;  // 오류가 발생하면 다음 클라이언트 연결을 대기
        }

        // 클라이언트가 최대 수에 도달했을 경우 처리 (더 이상 클라이언트를 수용하지 않음)
        if (client_count >= MAX_CLIENTS) {
            printf("Maximum clients connected.\n");
            close(new_sock);  // 클라이언트 소켓 닫기
            continue;  // 더 이상 처리하지 않음
        }

        // 6. 새로운 클라이언트 구조체 생성 및 초기화 (동적 할당)
        clients[client_count] = (Client *)malloc(sizeof(Client));  // 클라이언트 구조체 메모리 할당
        if (clients[client_count] == NULL) {  // 메모리 할당 실패 처리
            perror("malloc() :");
            return -1;
        }

        clients[client_count]->csock = new_sock;  // 클라이언트 소켓을 구조체에 저장

        // 클라이언트의 IP 주소를 출력
        memset(mesg, 0, BUFSIZ);  // 메시지 버퍼 초기화
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);  // 클라이언트 IP 주소를 문자열로 변환
        printf("Client Socket %d is connected: %s\n", client_count, mesg);  // 연결된 클라이언트 정보 출력

        // 7. 부모-자식 간 통신을 위한 파이프 생성 (부모->자식, 자식->부모 간 통신을 위한 파이프)
        if (pipe(clients[client_count]->parent_to_child) < 0 || pipe(clients[client_count]->child_to_parent) < 0) {
            perror("pipe()");
            close(new_sock);  // 소켓 닫기
            free(clients[client_count]);  // 메모리 해제
            continue;  // 에러가 발생했으므로 다음 클라이언트 처리로 넘어감
        }

        // 8. 자식 프로세스 생성 (fork 호출)
        if ((pid = fork()) < 0) {  // fork 실패 처리
            perror("fork()");
            close(new_sock);  // 소켓 닫기
            free(clients[client_count]);  // 메모리 해제
            close(clients[client_count]->parent_to_child[0]);  // 파이프 닫기
            close(clients[client_count]->parent_to_child[1]);
            close(clients[client_count]->child_to_parent[0]);
            close(clients[client_count]->child_to_parent[1]);
            continue;  // 에러가 발생했으므로 다음 클라이언트 처리로 넘어감
        }
        else if (pid == 0) {  // 자식 프로세스 처리

            close(clients[client_count]->parent_to_child[1]);  // 부모->자식 쓰기 파이프 닫기 (자식에서 쓰지 않음)
            close(clients[client_count]->child_to_parent[0]);  // 자식->부모 읽기 파이프 닫기 (자식에서 읽지 않음)
            close(ssock);  // 자식은 서버 소켓을 사용하지 않으므로 닫기
            clients[client_count]->pid = getppid();  // 부모 프로세스의 PID를 저장
            clients[client_count]->client_id = client_count;  // 클라이언트 ID 설정

            do {
                    Client recvCli;

                    // 클라이언트 소켓에서 메시지 읽기
                    if ((readBufferByte = read(clients[client_count]->csock, &recvCli, BUFSIZ)) > 0) {

                        clients[client_count]->action = recvCli.action;
                        strcpy(clients[client_count]->username, recvCli.username);
                        strcpy(clients[client_count]->password, recvCli.password);
                        strcpy(clients[client_count]->mesg, recvCli.mesg);

                        printf("%d child socket received : %s", clients[client_count]->client_id, clients[client_count]->mesg);  // 자식 프로세스가 메시지를 받았을 때 출력

                        // 부모에게 구조체 전송 전송 (자식->부모 파이프로 데이터 전송)
                        if (write(clients[client_count]->child_to_parent[1], clients[client_count], sizeof(Client)) < 0) {
                            perror("child_to_parent pipe 1 write()");
                        }

                        // 부모에게 SIGUSR1 신호 보내기 (부모 프로세스가 데이터를 처리할 수 있도록 신호 전달)
                        kill(clients[client_count]->pid, SIGUSR1);

                    }
                    else{
                        perror("Client csock read() :");
                        break;
                    }
                } while (childFlag);  // "/q" 입력 시 종료 (종료 신호 처리)

            printf("Child %d process terminating\n", client_count);  // 자식 프로세스 종료 메시지 출력

            // 자원 정리 및 종료 (파이프와 소켓 닫기)
            close(clients[client_count]->parent_to_child[0]);
            close(clients[client_count]->child_to_parent[1]);
            close(clients[client_count]->csock);
            exit(0);  // 자식 프로세스 종료

        } else {  // 부모 프로세스 처리
            close(clients[client_count]->parent_to_child[0]);  // 부모->자식 읽기 파이프 닫기 (부모에서 읽지 않음)
            close(clients[client_count]->child_to_parent[1]);  // 자식->부모 쓰기 파이프 닫기 (부모에서 쓰지 않음)

            clients[client_count]->pid = pid;  // 자식 프로세스 ID 저장
            client_count++;  // 클라이언트 수 증가 (새로운 클라이언트 추가)
        }

    } while (1);  // 서버는 계속 실행 (무한 루프)

    // 서버 종료 처리 (서버가 종료되었을 때)
    printf("Parent process terminating\n");

    for (int i = 0; i < client_count; i++) {  // 모든 클라이언트의 자원 해제
        close(clients[i]->parent_to_child[1]);  // 부모->자식 쓰기 파이프 닫기
        close(clients[i]->child_to_parent[0]);  // 자식->부모 읽기 파이프 닫기
    }
    close(ssock);   // 서버 소켓 닫기
    return 0;  // 프로그램 종료
}

// 서버 측 SIGUSR1 처리 함수 (부모 프로세스가 자식 프로세스의 메시지를 처리하는 함수)
static void sigHandlerParent(int signo) {
    if (signo == SIGUSR1) {  // SIGUSR1 신호를 받은 경우
        for (int i = 0; i < client_count; i++) {  // 현재 연결된 클라이언트 중 신호를 보낸 클라이언트를 찾음

            // 스택에 구조체 선언 (포인터 대신 구조체를 직접 선언하여 데이터를 수신)
            Client recvCli;

            // 파이프에서 데이터 읽기 (논블로킹 모드로 설정)
            int flags = fcntl(clients[i]->child_to_parent[0], F_GETFL, 0);
            fcntl(clients[i]->child_to_parent[0], F_SETFL, flags | O_NONBLOCK);

            // 논블로킹 read: 파이프에서 데이터 읽기
            ssize_t n = read(clients[i]->child_to_parent[0], &recvCli, sizeof(Client));

            if (n < 0) {  // 읽기 오류 처리
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 파이프에 읽을 데이터가 없음을 의미 (비동기 작업에서는 흔한 상황)
                } else {
                    // 그 외의 read 에러 처리
                }
            } else if (n == 0) {  // EOF: 파이프가 닫힘 (더 이상 데이터가 없음)
                    printf("Pipe closed (EOF).\n");
            } else {  // 성공적으로 데이터를 읽은 경우
                printf("Parent read Data from %d Client pipe: %s", recvCli.client_id, recvCli.mesg);  // 부모가 자식으로부터 받은 메시지 출력

                // 데이터 업데이트
                clients[i]->action = recvCli.action;
                strcpy(clients[i]->username, recvCli.username);
                strcpy(clients[i]->password, recvCli.password);
                strcpy(clients[i]->mesg, recvCli.mesg);

                // 1: 회원가입, 2: 로그인 요청, 3: 로그아웃 요청, 4: 채팅 off 요청
                // 5 : 기존에 로그인된 유저가 있음, 6 : 비밀번호 틀림 7 :로그인 성공
                // 9 : 채팅 on, 10 : logout, chat off
                switch (clients[i]->action) {
                    // 1: 회원가입, 2: 로그인, 3: 로그아웃, 4: 채팅종료
                    case 1:
                        //recvNamepw(1);
                        break;
                    case 2:     // 로그인 요청
                        login(i);
                        break;
                    case 3:     // 로그 아웃 요청
                        logout(i);
                        printf("Exiting chat.\n");
                        childFlag = 0;
                        break;
                    case 5:
                        chat_on(i);
                    default:
                        printf("Invalid option.\n");
                        break;
                }
                
            }
        }
    }
}

// 자식 측 SIGUSR2 처리 함수 (부모 프로세스가 자식 프로세스에게 전달하는 신호를 처리)
static void sigHandlerChild(int signo) {
    if (signo == SIGUSR2) {  // SIGUSR2 신호를 받은 경우

        // 스택에 구조체 선언 (포인터 대신 구조체를 직접 선언하여 데이터를 수신)
        Client recvCli;

        // 부모에서 자식으로 전달된 메시지 읽기
        if (read(clients[client_count]->parent_to_child[0], &recvCli, sizeof(Client)) > 0) {  // 부모->자식 파이프에서 데이터 읽기
            if (recvCli.client_id < clients[client_count]->client_id) {  // 클라이언트 ID가 변경되었으면
                clients[client_count]->client_id--;  // 클라이언트 ID를 감소시킴
            }
        } else {
            perror("pipe read()");  // 읽기 실패 시 오류 메시지 출력
        }
    }
}

// 클라이언트 목록에서 종료된 클라이언트를 제거하는 함수
void remove_client(int index) {
    if (clients[index] != NULL) {
        close(clients[index]->csock);                   // 클라이언트와 연결된 소켓 닫기
        close(clients[index]->child_to_parent[0]);      // 자식->부모 파이프 닫기
        close(clients[index]->parent_to_child[1]);      // 부모->자식 파이프 닫기

        printf("Child process %d removed\n", index);  // 클라이언트 제거 메시지 출력

        free(clients[index]);  // 동적으로 할당된 클라이언트 메모리 해제
        clients[index] = NULL;  // 포인터 초기화

        client_count--;  // 총 클라이언트 수 감소

        // 클라이언트 목록을 한칸씩 당기기 (클라이언트 목록에서 빈자리를 없앰)
        for (int i = index; i < client_count; i++) {
            clients[i] = clients[i + 1];
        }

        clients[client_count] = NULL;  // 마지막 클라이언트 포인터를 초기화
    }
}

// 로그인 함수
void login(int i) {

    for(int x =0; x < client_count; x++)
    {
        if(x != i)  // 해당 클라이언트 말고 다른 클라이언트랑 비교
        {
            // 기존에 클라이언트 이름이 등록되어 있는 경우
            if(!strcmp(clients[x]->username,clients[i]->username))
            {
                clients[i]->action = 5;     //  기존에 유저가 있음을 나타냄
                break;
            }
            else if(!strcmp(clients[x]->password,clients[i]->password))
            {
                clients[i]->action = 7;    // 로그인 되었음을 나타냄
                break;
            }
            else
            {
                clients[i]->action = 6;    // 비밀번호 틀림
                break;
            }
        }
    }

    if(clients[i]->action == 5)         //  기존에 유저가 있음을 나타냄
    {
        sprintf(clients[i]->mesg, "%d Already logged in \n", i);
        if (write(clients[i]->csock, clients[i], sizeof(Client)) > 0) {  // 메시지 전송 성공
        printf("Parent send Data to %d Client pipe: success\n", i);
        }
        else {  // 메시지 전송 실패 처리
            printf("Parent send Data to %d Client pipe: fail\n", i);
            perror("write()");
        }
    }
    else if(clients[i]->action == 6)    // 비밀번호 틀림
    {
        sprintf(clients[i]->mesg, "%d Invalid username or password. ",i);
        if (write(clients[i]->csock, clients[i], sizeof(Client)) > 0) {  // 메시지 전송 성공
        printf("Parent send Data to %d Client pipe: success\n", i);
        }
        else {  // 메시지 전송 실패 처리
            printf("Parent send Data to %d Client pipe: fail\n", i);
            perror("write()");
        }
    }
    else if(clients[i]->action == 7)    // 로그인 되었음을 나타냄
    {
        // 다른 클라이언트에게 메시지를 브로드캐스트 (모든 클라이언트에게 전송)
        sprintf(clients[i]->mesg, "%d is chat on ", i);
        printf("%d is login",i);
        for (int j = 0; j < client_count; j++) {

            if (write(clients[j]->csock, clients[i], sizeof(Client)) > 0) {  // 메시지 전송 성공
                printf("Parent send Data to %d Client pipe: success\n", j);
            }
            else {  // 메시지 전송 실패 처리
                printf("Parent send Data to %d Client pipe: fail\n", j);
                perror("write()");
            }

        }
    }

}

// 로그아웃 함수
void logout(int i)
{

    if (!strncmp(clients[i]->mesg, "/q", 2))   // "/q" 메시지인 경우 클라이언트 종료 처리

    // 종료됨을 알림 (다른 클라이언트에게 알림)
    for (int j = 0; j < client_count; j++) {
        if (j != i) {  // 다른 클라이언트에게만 알림
            sprintf(clients[i]->mesg, "%d is terminating... \n", i);
            write(clients[j]->csock, clients[i], sizeof(Client));  // 메시지를 다른 클라이언트에 전송

            write(clients[j]->parent_to_child[1], clients[i], sizeof(Client));  // 다른 자식 프로세스에게 종료를 알림

            if (j > i)  // 종료된 자식 프로세스보다 뒤에 생성된 프로세스들의 클라이언트 ID 갱신
                clients[j]->client_id--;
            kill(clients[j]->pid, SIGUSR2);  // SIGUSR2 신호로 갱신된 정보를 전달
        }
    }

    write(clients[i]->csock, clients[i], sizeof(Client));  // 종료된 클라이언트에 종료 메시지 전송
    remove_client(i);  // 클라이언트를 목록에서 제거
}

void chat_on(int i)
{
    // 다른 클라이언트에게 메시지를 브로드캐스트 (모든 클라이언트에게 전송)
    for (int j = 0; j < client_count; j++) {
        if (j != i) {  // 자기 자신을 제외한 클라이언트들에게 전송
            if (write(clients[j]->csock, clients[i], strlen(clients[i]->mesg)) > 0) {  // 메시지 전송 성공
                printf("Parent send Data to %d Client pipe: success\n", j);
            }
            else {  // 메시지 전송 실패 처리
                printf("Parent send Data to %d Client pipe: fail\n", j);
                perror("write()");
            }
        }
    }
}
