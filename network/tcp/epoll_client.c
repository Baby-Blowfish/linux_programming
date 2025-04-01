/**
 * @file epoll_echo_client.c
 * @brief epoll 기반 클라이언트 - 사용자 입력과 서버 응답을 동시에 처리
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define SERVER_PORT 5100
#define MAX_EVENTS 2

void setnonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char* argv[]) {
    int sockfd, epfd;
    struct sockaddr_in servaddr;
    struct epoll_event ev, events[MAX_EVENTS];
    char buf[BUFSIZ];

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <SERVER_IP>\n", argv[0]);
        return 1;
    }

    // 1. 소켓 생성 및 서버 연결
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect()");
        return 1;
    }

    // 2. epoll 인스턴스 생성 및 등록
    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1()");
        return 1;
    }

    setnonblocking(0);       // stdin을 논블로킹으로 설정
    setnonblocking(sockfd);  // 소켓도 논블로킹으로 설정

    // 표준 입력을 epoll에 등록
    ev.events = EPOLLIN;
    ev.data.fd = 0;
    epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev);

    // 소켓을 epoll에 등록
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    printf("[클라이언트] 서버에 연결됨. 메시지를 입력하세요. (q 입력 시 종료)\n");

    while (1) {
        int nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nready == -1) {
            perror("epoll_wait()");
            break;
        }

        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;

            // 사용자 입력 처리
            if (fd == 0 && (events[i].events & EPOLLIN)) {
                memset(buf, 0, sizeof(buf));
                int len = read(0, buf, sizeof(buf));
                if (len > 0) {
                    if (strncmp(buf, "q", 1) == 0) {
                        printf("[클라이언트] 종료 요청됨\n");
                        goto done;
                    }
                    write(sockfd, buf, len);
                }
            }
            // 서버 응답 처리
            else if (fd == sockfd && (events[i].events & EPOLLIN)) {
                memset(buf, 0, sizeof(buf));
                int len = read(sockfd, buf, sizeof(buf));
                if (len == 0) {
                    printf("[클라이언트] 서버 종료 감지\n");
                    goto done;
                } else if (len > 0) {
                    printf("[서버 응답] %s", buf);
                }
            }
        }
    }

done:
    close(sockfd);
    close(epfd);
    return 0;
}

