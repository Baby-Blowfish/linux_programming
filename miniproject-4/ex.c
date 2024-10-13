#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512
#define NAME_SIZE  20
#define MAX_CLNT   20

// 클라이언트 정보를 저장할 구조체
typedef struct {
    SOCKET sock;
    char name[NAME_SIZE];
} ClientInfo;

ClientInfo clients[MAX_CLNT]; // 클라이언트 정보 배열
int client_count = 0; // 현재 클라이언트 수

// 클라이언트와 데이터 통신
void *ProcessClient(void *arg) {
    SOCKET client_sock = (SOCKET)(long long)arg;
    struct sockaddr_in clientaddr;
    char addr[INET_ADDRSTRLEN];
    socklen_t addrlen;
    int len; // 고정 길이 데이터
    char name_msg[BUFSIZE + NAME_SIZE]; // 가변 길이 데이터
    char *name, *msg;
    int retval;

    // 클라이언트 정보 얻기
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (struct sockaddr *)&clientaddr, &addrlen);
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

    // 클라이언트 이름 저장
    strncpy(clients[client_count].name, addr, sizeof(clients[client_count].name));
    clients[client_count].sock = client_sock;
    client_count++;

    while (1) {
        // 데이터 받기(고정 길이)
        retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
        if (retval == SOCKET_ERROR) {
            err_display_msg("recv()");
            break;
        } else if (retval == 0) // 클라이언트와 연결이 끊어졌을 경우
            break;

        // 데이터 받기(가변 길이)
        retval = recv(client_sock, name_msg, len, MSG_WAITALL);
        if (retval == SOCKET_ERROR) {
            err_display_msg("recv()");
            break;
        } else if (retval == 0) // 클라이언트와 연결이 끊어졌을 경우 
            break;

        // 받은 데이터 출력
        name_msg[retval] = '\0';
        printf("[TCP/%s:%d] [Recv] [%d +%d byte] %s \n", addr, ntohs(clientaddr.sin_port), (int)sizeof(len), retval, name_msg);

        // 모든 클라이언트에게 메시지 브로드캐스트
        for (int i = 0; i < client_count; i++) {
            if (clients[i].sock != client_sock) { // 자신에게는 보내지 않음
                send(clients[i].sock, (char*)&len, sizeof(int), 0); // 고정 길이
                send(clients[i].sock, name_msg, len, 0); // 가변 길이
            }
        }
    }

    // 클라이언트 연결 종료 처리
    for (int i = 0; i < client_count; i++) {
        if (clients[i].sock == client_sock) {
            // 해당 클라이언트 정보 삭제
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }

    close(client_sock);
    printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));
    return 0;
}

int main(int argc, char *argv[]) {
    int retval;

    // 소켓 생성
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    // bind()
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // listen()
    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    // 데이터 통신에 사용할 변수
    SOCKET client_sock;
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    pthread_t tid;

    while (1) {
        // accept()
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display_msg("accept()");
            break;
        }

        // 접속한 클라이언트 정보 출력
        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
        printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
            addr, ntohs(clientaddr.sin_port));

        // 스레드 생성
        retval = pthread_create(&tid, NULL, ProcessClient, (void *)(long long)client_sock);
        if (retval != 0) { close(client_sock); }
    }

    // 소켓 닫기
    close(listen_sock);
    return 0;
}
