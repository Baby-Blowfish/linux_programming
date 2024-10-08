
#include "server_functions.h"

int main(int argc, char **argv) {

    // 프로세스 및 TCP 소켓 관련 변수 설정
    pid_t pid;  // 자식 프로세스 ID를 저장할 변수
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
                memset(clients[client_count]->mesg, 0, BUFSIZ); // 메시지 버퍼 초기화

                // 클라이언트 소켓에서 메시지 읽기
                if ((readBufferByte = read(clients[client_count]->csock, clients[client_count]->mesg, BUFSIZ)) <= 0) {
                    perror("Client csock read() :");
                    break;
                }

                printf("%d child socket received : %s", clients[client_count]->client_id, clients[client_count]->mesg);  // 자식 프로세스가 메시지를 받았을 때 출력

                // 부모에게 메시지 전송 (자식->부모 파이프로 데이터 전송)
                if (write(clients[client_count]->child_to_parent[1], clients[client_count], sizeof(Client)) < 0) {
                    perror("child_to_parent pipe 1 write()");
                }

                // 부모에게 SIGUSR1 신호 보내기 (부모 프로세스가 데이터를 처리할 수 있도록 신호 전달)
                kill(clients[client_count]->pid, SIGUSR1);

            } while (strncmp(clients[client_count]->mesg,"/q",2));  // "/q" 입력 시 종료 (종료 신호 처리)

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


