/**
 * @file multi_echo_server.c
 * @brief fork() 기반 TCP 멀티 클라이언트 에코 서버 + shutdown()
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define TCP_PORT 5100
#define BUF_SIZE 1024

void child_process(int csock, struct sockaddr_in cliaddr) {
    char buf[BUF_SIZE];
    int n;

    printf("[서버] 클라이언트 접속: %s\n", inet_ntoa(cliaddr.sin_addr));

    while ((n = recv(csock, buf, BUF_SIZE - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("[서버: %s] 받은 메시지: %s", inet_ntoa(cliaddr.sin_addr), buf);

        // "q"로 시작하면 종료
        if (strncmp(buf, "q", 1) == 0)
            break;

        send(csock, buf, n, 0);  // 에코 응답
    }

    printf("[서버: %s] 연결 종료\n", inet_ntoa(cliaddr.sin_addr));
    shutdown(csock, SHUT_RDWR);
    close(csock);
    exit(0); // 자식 프로세스 종료
}

void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main()
{
    int ssock, csock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clen = sizeof(cliaddr);

    // 자식 프로세스 종료 시 좀비 제거
    signal(SIGCHLD, sigchld_handler);

    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock < 0) {
        perror("socket()");
        exit(1);
    }

    int optval = 1;
    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    if (bind(ssock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        exit(1);
    }

    if (listen(ssock, 10) < 0) {
        perror("listen()");
        exit(1);
    }

    printf("[서버] 멀티 클라이언트 서버 실행 중...\n");

    while (1) {
        csock = accept(ssock, (struct sockaddr*)&cliaddr, &clen);
        if (csock < 0) {
            perror("accept()");
            continue;
        }

        // 클라이언트마다 자식 프로세스 생성
        pid_t pid = fork();
        if (pid == 0) {
            // 자식 프로세스: 클라이언트 처리
            close(ssock); // 자식은 서버 소켓 안 씀
            child_process(csock, cliaddr);
        } else if (pid > 0) {
            close(csock); // 부모는 클라이언트 소켓 안 씀
        } else {
            perror("fork()");
            close(csock);
        }
    }

    close(ssock);
    return 0;
}

