#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TCP_PORT 5100

typedef struct {
    int sock;
    int running; // 1: 실행 중, 0: 종료
    pthread_mutex_t mutex;
} ServerArg;

void* send_thread(void* arg) {
    ServerArg* t_arg = (ServerArg*)arg;
    char send_buf[BUFSIZ];

    while (1) {
      pthread_mutex_lock(&t_arg->mutex);
      if (!t_arg->running) {
        pthread_mutex_unlock(&t_arg->mutex);
        break;
      }
      pthread_mutex_unlock(&t_arg->mutex);

      printf("[클라이언트] 메시지 입력 (q 입력 시 종료): ");
      if (fgets(send_buf, BUFSIZ, stdin) == NULL)
          continue;

      send(t_arg->sock, send_buf, strlen(send_buf), 0);

      if (strncmp(send_buf, "q", 1) == 0) {
        pthread_mutex_lock(&t_arg->mutex);
        t_arg->running = 0;
        pthread_mutex_unlock(&t_arg->mutex);

        shutdown(t_arg->sock, SHUT_WR);
        break;
      }
    }

    pthread_exit(NULL);
}

void* recv_thread(void* arg) {
    ServerArg* t_arg = (ServerArg*)arg;
    char recv_buf[BUFSIZ];
    int n;

    while ((n = recv(t_arg->sock, recv_buf, BUFSIZ - 1, 0)) > 0) {
        recv_buf[n] = '\0';
        printf("[클라이언트] 서버 응답: %s", recv_buf);
    }

    if (n == 0)
      printf("서버가 클라이언트와의 연결을 종료했음\n");

    pthread_mutex_lock(&t_arg->mutex);
    t_arg->running = 0;
    pthread_mutex_unlock(&t_arg->mutex);

    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    int sock;
    struct sockaddr_in servaddr;
    pthread_t tid_send, tid_recv;

    if (argc < 2) {
        printf("Usage: %s IP_ADDRESS\n", argv[0]);
        return -1;
    }

    // 1. 소켓 생성
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    // 2. 서버 주소 설정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr.s_addr);
    servaddr.sin_port = htons(TCP_PORT);

    // 3. 서버에 연결
    if (connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect()");
        close(sock);
        return -1;
    }

    // 4. 스레드 인자 동적 할당
    ServerArg* arg = (ServerArg*)malloc(sizeof(ServerArg));
    if (!arg) {
        perror("malloc()");
        close(sock);
        return -1;
    }
    arg->sock = sock;
    arg->running = 1;
    pthread_mutex_init(&arg->mutex, NULL);

    // 5. 송수신 스레드 생성
    pthread_create(&tid_send, NULL, send_thread, arg);
    pthread_create(&tid_recv, NULL, recv_thread, arg);

    // 6. 스레드 종료 대기
    pthread_join(tid_send, NULL);
    pthread_join(tid_recv, NULL);

    // 7. 소켓 종료 및 메모리 해제
    close(sock);
    pthread_mutex_destroy(&arg->mutex);
    free(arg);

    printf("클라이언트 종료\n");
    return 0;
}
