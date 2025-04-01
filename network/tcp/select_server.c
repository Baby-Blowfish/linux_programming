#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT 5100
#define MAX_CLIENT 5

int main(int argc, char** argv)
{
    int ssock;                               // 서버 소켓
    struct sockaddr_in servaddr, cliaddr;    // 주소 구조체
    socklen_t clen;
    char mesg[BUFSIZ];
    int n;

    fd_set readfd;                           // select()용 read fd 집합
    int maxfd;                               // select()에 넣을 fd 최대값 + 1
    int client_fd[MAX_CLIENT] = {0};         // 클라이언트 소켓 저장 배열
    int client_count = 0;

    // 1. 소켓 생성
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        return -1;
    }

    // 포트 재사용
    int optval = 1;
    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // 2. 주소 구조체 설정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    // 3. 바인드
    if (bind(ssock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind()");
        return -1;
    }

    // 4. 리슨
    if (listen(ssock, 8) < 0)
    {
        perror("listen()");
        return -1;
    }

    printf("[서버] select 서버  포트 %d에서 대기 중...\n", SERVER_PORT);

    while (1)
    {
        FD_ZERO(&readfd);
        FD_SET(ssock, &readfd); // 서버 소켓 등록
        maxfd = ssock;

        // 클라이언트 소켓을 readfd에 추가
        for (int i = 0; i < client_count; i++) {
            FD_SET(client_fd[i], &readfd);
            if (client_fd[i] > maxfd)
                maxfd = client_fd[i];
        }

        maxfd = maxfd + 1; // select()의 첫 번째 인자는 최대 fd + 1

        // select 대기
        if (select(maxfd, &readfd, NULL, NULL, NULL) < 0) {
            perror("select()");
            break;
        }

        // 5. 새 클라이언트 연결 수락
        if (FD_ISSET(ssock, &readfd)) {
            clen = sizeof(cliaddr);
            int csock = accept(ssock, (struct sockaddr*)&cliaddr, &clen);
            if (csock < 0) {
                perror("accept()");
            } else {
                if (client_count < MAX_CLIENT) {
                    inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
                    printf("[서버] 클라이언트 접속: %s (fd: %d)\n", mesg, csock);

                    client_fd[client_count++] = csock;
                } else {
                    printf("[서버] 최대 접속 수 초과, 연결 종료\n");
                    close(csock);
                }
            }
        }

        // 6. 기존 클라이언트 처리
        for (int i = 0; i < client_count; i++) {
            int fd = client_fd[i];

            if (FD_ISSET(fd, &readfd)) {

                memset(mesg, 0, sizeof(mesg));
                n = recv(fd, mesg, BUFSIZ-1,0);

                if (n <= 0) {
                    // 클라이언트 종료
                    printf("[서버] 클라이언트(fd: %d) 연결 종료\n", fd);
                    shutdown(fd, SHUT_RDWR);
                    close(fd);

                    // 소켓 배열에서 제거
                    for (int j = i; j < client_count - 1; j++)
                        client_fd[j] = client_fd[j + 1];
                    client_count--;
                    i--; // 배열 이동했으므로 i 보정
                    continue;
                }

                mesg[n] = '\0';
                printf("[서버] 받은 메시지(fd: %d): %s", fd, mesg);
                write(fd, mesg, n); // 에코

                // "Q" 입력 시 전체 종료
                if (strncmp(mesg, "Q", 1) == 0) {
                    printf("[서버] 전체 종료 요청 (from fd: %d)\n", fd);

                    for (int j = 0; j < client_count; j++) {
                        shutdown(client_fd[j], SHUT_RDWR);
                        close(client_fd[j]);
                        printf(" - 클라이언트 종료: fd %d\n", client_fd[j]);
                    }
                    shutdown(ssock, SHUT_RDWR);
                    close(ssock);
                    return 0;
                }
            }
        }
    }


    shutdown(ssock, SHUT_RDWR);
    close(ssock);
    return 0;
}

