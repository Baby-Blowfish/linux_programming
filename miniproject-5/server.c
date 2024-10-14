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

ClientInfo clients[MAX_CLNT];	// 클라이언트 정보 배열
int client_count = 0;			// 현재 클라이언트 수

// 클라이언트와 데이터 통신
void *ProcessClient(void *arg)
{
	SOCKET client_sock = (SOCKET)(long long)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	socklen_t addrlen;
	int len;	// 고정 길이 데이터
	char name_msg[BUFSIZE + NAME_SIZE];	// 가변 길이 데이터
	char name_msg_copy[BUFSIZE + NAME_SIZE]; // strtok()을 위한 복사본
	char *name, *msg;
	int retval;
	
	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr *)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	// 클라이언트 정보 추가
	clients[client_count].sock = client_sock;
	client_count++;



	while (1) {
		// 데이터 받기(고정 길이)
		retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display_msg("recv()");
			break;
		}
		else if (retval == 0) // 클라이언트와 연결이 끊어졌을 경우
			break;

		// 데이터 받기(가변  길이)
		retval = recv(client_sock, name_msg, len, MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display_msg("recv()");
			break;
		}
		else if (retval == 0) // 클라이언트와 연결이 끊어졌을 경우 
			break;

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

		// 이름 저장
		strncpy(clients[client_count].name, name, sizeof(clients[client_count].name));
		

		// 받은 데이터 출력
		name_msg[retval] = '\0';
		printf("[TCP/%s:%d] [Recv] [%d +%d byte] %s \n", addr, ntohs(clientaddr.sin_port), (int)sizeof(len),retval,name_msg);

		
		
		// 'q'가 입력된 경우 클라이언트 연결 종료 처리
		if (strcmp(msg, "q") == 0)
		{
			// 데이터 보내기(고정 길이)
			retval = send(client_sock, (char *)&len, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display_msg("send()");
				break;
			}

			// 데이터 보내기(가변 길이)
			retval = send(client_sock, name_msg, len, 0);
			if (retval == SOCKET_ERROR) {
				err_display_msg("send()");
				break;
			}
			printf("[TCP/%s:%d] [Send] [%d +%d byte] %s \n", addr, ntohs(clientaddr.sin_port),(int)sizeof(len),retval,name_msg);
			

			for(int i = 0; i < client_count; i++)
			{
				if(clients[i].sock == client_sock)
				{
					for (int j = i; j < client_count - 1; j++)
					{
						clients[j] = clients[j + 1];
					}
					client_count--;
					break;
				}	
			}

			break;
		}
		else
		{
			for( int i =  0; i < client_count; i++)	// 모든 클라이언트에게 메시지 브로드캐스트
			{
				if(clients[i].sock != client_sock)	//  자신에게는 보내지 않음
				{
					// 데이터 보내기(고정 길이)
					retval = send(clients[i].sock, (char *)&len, sizeof(int), 0);
					if (retval == SOCKET_ERROR) {
						err_display_msg("send()");
						break;
					}

					// 데이터 보내기(가변 길이)
					retval = send(clients[i].sock, name_msg, len, 0);
					if (retval == SOCKET_ERROR) {
						err_display_msg("send()");
						break;
					}
					printf("[TCP/%s:%d] [Send] [%d +%d byte] %s \n", addr, ntohs(clientaddr.sin_port),(int)sizeof(len),retval,name_msg);
				}
			}	
			
		}


	}
	
	sleep(1);
	// 소켓 닫기
	close(client_sock);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",addr, ntohs(clientaddr.sin_port));
	return 0;
}

int main(int argc, char *argv[])
{
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
		retval = pthread_create(&tid, NULL, ProcessClient,
			(void *)(long long)client_sock);
		if (retval != 0) { close(client_sock); }
	}

	// 소켓 닫기
	close(listen_sock);
	return 0;
}


