#include "Common.h"
#include <sys/select.h>

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512
#define NAME_SIZE  20

char name[NAME_SIZE] = "[DEFAULT]";

void *CommunicationThread(void *arg);

int main(int argc, char *argv[])
{
    int retval;

    if (argc != 2)
    {
        printf("Usage : %s <Name>\n", argv[0]);
        exit(1);
    }

    memset(name, 0, sizeof(name));
    sprintf(name, "[%s] :", argv[1]);

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // connect()
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    printf("Connected to the server.\n");

    // 스레드 생성
    pthread_t tid;
    retval = pthread_create(&tid, NULL, CommunicationThread, (void *)(long long)sock);
    if (retval != 0) { close(sock); };

    // 스레드 종료 대기
    pthread_join(tid, NULL);

    // 소켓 닫기
    close(sock);
    return 0;
}

void *CommunicationThread(void *arg)
{
    int retval;
    SOCKET sock = (SOCKET)(long long)arg;
    char msg[BUFSIZE];
    char name_msg[NAME_SIZE + BUFSIZE];   // 가변 길이 데이터
    char name_msg_copy[BUFSIZE + NAME_SIZE];
    char *name;
    char *msg;
    int len;                              // 고정 길이 데이터

    if(client_first_set)
		{

			memset(msg, 0, sizeof(msg));
			memset(name_msg, 0, sizeof(name_msg));
			
			sprintf(name_msg, "%s client name", name);
			
			len = (int)strlen(name_msg);

			// 데이터 보내기(고정 길이)
			retval = send(sock, (char *)&len, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display_msg("send()");
				break;
			}

			// 데이터 보내기(가변 길이)
			retval = send(sock, name_msg, len, 0);
			if (retval == SOCKET_ERROR) {
				err_display_msg("send()");
				break;
			}

			printf("[Send] [%d +%d byte]\n", (int)sizeof(len), retval);
			
			client_first_set = false;
		}

    while (1) {


    

    // 표준 입력(키보드 입력)
    if (FD_ISSET(STDIN_FILENO, &readfds)) {

      
    
    }

    
}
