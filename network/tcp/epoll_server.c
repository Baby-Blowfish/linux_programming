/**
 * @file epoll_echo_server.c
 * @brief epoll 기반 에코 서버 - 연결 유지 + 반복 read/send 처리
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define SERVER_PORT 5100
#define MAX_EVENTS 64

// 소켓을 논블로킹(non-blocking) 모드로 설정하는 함수
void setnonblocking(int fd)
{
    int opts = fcntl(fd, F_GETFL);
    opts |= O_NONBLOCK;
    fcntl(fd, F_SETFL, opts);
}

int main()
{
    int ssock, csock;
    socklen_t clen;
    struct sockaddr_in servaddr, cliaddr;
    struct epoll_event ev, events[MAX_EVENTS];
    int epfd, nready;
    char buf[BUFSIZ];

    // 1. 서버 소켓 생성
    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock < 0) {
        perror("socket()");
        return -1;
    }

    // 2. 서버 소켓 논블로킹 설정
    setnonblocking(ssock);

    // 3. 주소 구조체 설정 및 바인드
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if (bind(ssock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        return -1;
    }

    // 4. 리슨 상태로 변경
    if (listen(ssock, SOMAXCONN) < 0) {
        perror("listen()");
        return -1;
    }

    // 5. epoll 인스턴스 생성
    epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1()");
        return -1;
    }

    // 6. 서버 소켓을 epoll에 등록
    ev.events = EPOLLIN;
    ev.data.fd = ssock;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, ssock, &ev)<0)
    {
      perror("epoll_ctl()");
      return 1;
    }

    printf("[서버] epoll 기반 에코 서버 실행 중 (포트 %d)\n", SERVER_PORT);

    // 7. 이벤트 루프 시작
    while (1)
    {
        nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nready == -1) {
            perror("epoll_wait()");
            break;
        }

        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;

            // 7-1. 새 클라이언트 연결 요청 처리
            if (fd == ssock) {
                clen = sizeof(cliaddr);
                csock = accept(ssock, (struct sockaddr*)&cliaddr, &clen);
                if (csock > 0) {
                    setnonblocking(csock); // 클라이언트 소켓도 논블로킹 설정

                    // epoll에 클라이언트 소켓 추가
                    ev.events = EPOLLIN | EPOLLET; // Edge Triggered 방식
                    ev.data.fd = csock;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, csock, &ev);

                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &cliaddr.sin_addr, ip, sizeof(ip));
                    printf("[서버] 클라이언트 접속: %s (fd: %d)\n", ip, csock);
                }
            }
            // 7-2. 기존 클라이언트의 데이터 수신 처리
            else if (events[i].events & EPOLLIN) {

                if(events[i].data.fd < 0) continue; // 소켓이 아닌경우 처리

                memset(buf, 0, sizeof(buf));

                int len = read(fd, buf, sizeof(buf));

                if (len <= 0) {
                    // 연결 종료 또는 에러
                    printf("[서버] 클라이언트 종료: fd %d\n", fd);
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                } else {
                    printf("[서버] 메시지 수신 (fd %d): %s", fd, buf);
                    write(fd, buf, len); // 에코 응답
                }
            }
        }
    }

    // 8. 정리
    close(ssock);
    close(epfd);
    return 0;
}

