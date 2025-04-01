/**
 * @file select_echo_client.c
 * @brief select() 기반 TCP 클라이언트: 입력과 서버 응답을 동시에 처리
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define SERVER_PORT 5100

int main(int argc, char** argv)
{
    int ssock;
    int n;
    struct sockaddr_in servaddr;
    char mesg[BUFSIZ];
    fd_set readfd;

    // 1. IP 주소 인자 확인
    if (argc < 2) {
        printf("Usage : %s <IP_ADDRESS>\n", argv[0]);
        return -1;
    }

    // 2. 소켓 생성
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    // 3. 서버 주소 설정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(servaddr.sin_addr.s_addr));
    servaddr.sin_port = htons(SERVER_PORT);

    // 4. 서버에 연결
    if (connect(ssock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect()");
        return -1;
    }

    printf("[클라이언트] 서버에 연결됨 (%s:%d)\n", argv[1], SERVER_PORT);

    // 5. stdin (fd: 0)을 논블로킹으로 설정
    fcntl(0, F_SETFL, O_NONBLOCK); // SOCK_NONBLOCK → 오타! O_NONBLOCK이 맞습니다

    do
    {
        // 6. 감시 대상 초기화
        FD_ZERO(&readfd);
        FD_SET(0, &readfd);       // 표준 입력 감시
        FD_SET(ssock, &readfd);   // 서버 소켓 감시

        printf("[클라이언트] 메시지 입력 (q 입력 시 종료)\n");

        // 7. select() 호출 (최대 fd + 1)
        int maxfd = (ssock > 0 ? ssock : 0) + 1;
        if (select(maxfd, &readfd, NULL, NULL, NULL) < 0) {
            perror("select()");
            break;
        }

        // 8. 사용자 입력이 있으면 서버로 전송
        if (FD_ISSET(0, &readfd)) {
            memset(mesg, 0, sizeof(mesg));
            n = read(0, mesg, BUFSIZ);
            if (n > 0) {
                write(ssock, mesg, n);
                if (strncmp(mesg, "q", 1) == 0)
                    break;
            }
        }

        // 9. 서버로부터 데이터 수신
        if (FD_ISSET(ssock, &readfd)) {
            memset(mesg, 0, sizeof(mesg));
            n = read(ssock, mesg, BUFSIZ);
            if (n == 0) {
                printf("[클라이언트] 서버가 연결을 종료했습니다.\n");
                break;
            } else if (n > 0) {
                write(1, mesg, n); // 표준 출력으로 출력
                if (strncmp(mesg, "q", 1) == 0)
                    break;
            }
        }

    } while (1);


    if(n == 0)
        printf("서버가 연결 종료했음\n");

    shutdown(ssock, SHUT_RDWR);

    // 10. 종료
    close(ssock);
    printf("[클라이언트] 종료합니다.\n");

    return 0;
}

