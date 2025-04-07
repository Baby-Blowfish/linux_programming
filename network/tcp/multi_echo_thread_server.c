/**
 * @file multi_echo_server_thread.c
 * @brief pthread 기반 TCP 멀티 클라이언트 에코 서버 + shutdown() + mutex
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TCP_PORT 5100

typedef struct {
    int csock;
    struct sockaddr_in cliaddr;
    int running;
    pthread_mutex_t mutex;
} ClientArg;

void* client_handler(void* arg) {
    ClientArg* carg = (ClientArg*)arg;
    char buf[BUFSIZ];
    int n;

    printf("[서버] 클라이언트 접속: %s\n", inet_ntoa(carg->cliaddr.sin_addr));

    while (1) {
        n = recv(carg->csock, buf, BUFSIZ - 1, 0);
        if (n <= 0)
            break;

        buf[n] = '\0';
        printf("[서버: %s] 받은 메시지: %s", inet_ntoa(carg->cliaddr.sin_addr), buf);

        if (strncmp(buf, "q", 1) == 0) {
            pthread_mutex_lock(&carg->mutex);
            carg->running = 0;
            pthread_mutex_unlock(&carg->mutex);
            break;
        }

        send(carg->csock, buf, n, 0);
    }

    printf("[서버: %s] 연결 종료\n", inet_ntoa(carg->cliaddr.sin_addr));

    shutdown(carg->csock, SHUT_RDWR);
    close(carg->csock);

    pthread_mutex_destroy(&carg->mutex);
    free(carg);

    pthread_exit(NULL);
}

int main() {
    int ssock, csock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clen = sizeof(cliaddr);

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
        close(ssock);
        exit(1);
    }

    if (listen(ssock, 10) < 0) {
        perror("listen()");
        close(ssock);
        exit(1);
    }

    printf("[서버] 멀티 클라이언트 스레드 서버 실행 중...\n");

    while (1) {
        csock = accept(ssock, (struct sockaddr*)&cliaddr, &clen);
        if (csock < 0) {
            perror("accept()");
            continue;
        }

        ClientArg* carg = (ClientArg*)malloc(sizeof(ClientArg));
        if (!carg) {
            perror("malloc()");
            close(csock);
            continue;
        }

        carg->csock = csock;
        carg->cliaddr = cliaddr;
        carg->running = 1;
        pthread_mutex_init(&carg->mutex, NULL);

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, (void*)carg) != 0) {
            perror("pthread_create()");
            close(csock);
            pthread_mutex_destroy(&carg->mutex);
            free(carg);
        } else {
            pthread_detach(tid);
        }
    }

    close(ssock);
    return 0;
}
