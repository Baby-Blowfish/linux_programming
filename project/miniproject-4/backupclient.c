#include "Common.h"

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512
#define NAME_SIZE 20

char name[NAME_SIZE] = "[DEFAULT]";

void *WriteThread(void *arg);
void *ReadThread(void *arg);


int main(int argc, char *argv[])
{
	int retval;

	if(argc != 2)
	{
		printf("Usage : %s <Name>\n",argv[0]);
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



	// 스레드 생성
	pthread_t tid[2];
	retval=pthread_create(&tid[0], NULL, WriteThread, (void*)(long long)sock);
	if(retval != 0) { close(sock); };
	retval=pthread_create(&tid[1], NULL, ReadThread, (void*)(long long)sock);
	if(retval != 0) { close(sock); };

	/*
	while(1)
	{
	}
	*/
	
	// 스레드  종료 대기
	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);

	// 소켓 닫기
	close(sock);
	return 0;
}


void *WriteThread(void *arg)
{
	int retval;
	SOCKET sock = (SOCKET)(long long)arg;
	char msg[BUFSIZE];
	char name_msg[NAME_SIZE + BUFSIZE];	// 가변 길이 데이터
	int len;	// 고정 길이 데이터
	bool client_first_set = true;

	while(1) {
		
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

		memset(msg, 0, sizeof(msg));
		memset(name_msg, 0, sizeof(name_msg));
		
		if(fgets(msg, BUFSIZE, stdin) == NULL)
			break;

		// '\n', '\r' 문자 제거
		len = (int)strlen(msg);
		if(msg[len - 1] == '\n' || msg[len -1] == '\r')
		{
			msg[len - 1] = '\0';
			msg[len] = '\0';
		}

		sprintf(name_msg, "%s %s", name, msg);
		
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

		// "q"가 입력되면 소켓 종료
		if(!strcmp(msg,"q"))
			break;			
	}
	
		
	// 쓰래드 종료
	printf("write 쓰래드 종료\n");
	return 0;
}

void *ReadThread(void *arg)
{
	int retval;
	SOCKET sock = (SOCKET)(long long)arg;
	char name_msg[NAME_SIZE + BUFSIZE];	// 받은 데이터
	char name_msg_copy[BUFSIZE + NAME_SIZE]; // strtok()을 위한 복사본
	char *name;
	char *msg;
	int len;	// 고정 길이 데이터

	while (1) {
		
		memset(name_msg, 0, sizeof(name_msg));
		
		// 데이터 받기(고정 길이)
		retval = recv(sock, (char*)&len, sizeof(int), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display_msg("recv()");
			break;
		}
		else if (retval == 0) // 클라이언트와 연결이 끊어졌을 경우
			break;

		// 데이터 받기(가변  길이)
		retval = recv(sock, name_msg, len, MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display_msg("recv()");
			break;
		}
		else if (retval == 0) // 클라이언트와 연결이 끊어졌을 경우 
			break;

		// 받은 데이터 출력
		name_msg[retval] = '\0';
		printf("[Recv] [%d +%d byte] %s\n",(int)sizeof(len), retval, name_msg);




		// 원본 문자열 보호를 위해 복사본 사용
		strncpy(name_msg_copy, name_msg, sizeof(name_msg_copy));
		name_msg_copy[sizeof(name_msg_copy) - 1] = '\0'; // 문자열 종료 보장
		
		// 이름 추출
		name = strtok(name_msg_copy, "[]");  // 대괄호로 이름 추출

		if (name == NULL) {
				printf("메시지 파싱 오류\n");
				continue;
		}

		strtok(NULL, ":");           // ':' 이전까지 건너뛰기
		msg = strtok(NULL, "");      // 메시지 추출

		if (msg == NULL) {
				printf("메시지 파싱 오류\n");
				continue;
		}

		// 앞 공백 제거
		while (isspace((unsigned char)*msg)) msg++; // 앞 공백 제거

		//printf("이름: %s, 메시지: %s\n", name, msg);


		// "q"가 입력되면 소켓 종료
		if(!strcmp(msg,"q"))
			break;			
	}
	
	// 쓰래드 종료
	printf("read 쓰래드 종료\n");

	return 0;
}
