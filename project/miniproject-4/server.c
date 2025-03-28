#include "Common.h"
#define SCREENSIZE  (1280*720) 								// v4l2 스펙 1280x720 YUYV 4:2:2
#define BUFFER_MAX  (CHAT_SIZE + SCREENSIZE)		// 받을 데이터의 최대 크기

// signal()
static void sigHandler(int signo);
void *ProcessClient(void *arg);
int change_name(ClientInfo *info, Message *chat_buf);
int chat_broadcast(ClientInfo *info, Message *msg);
void client_close(ClientInfo *info, Message *msg);

#define MAX_CLNT   20
SOCKET listen_sock;
pthread_mutex_t mutex_client;	// clients와 client_count 변수에 접근하기 위한 뮤텍스
ClientInfo clients[MAX_CLNT];	// 클라이언트 정보 배열
int client_count = 0;			// 현재 클라이언트 수



int main(int argc, char *argv[])
{

	// 시그널 등록
	signal(SIGINT, sigHandler);
	
		
	// 뮤텍스 초기화
	pthread_mutex_init(&mutex_client, NULL);
	
	int retval;

	// 소켓 생성
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
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
			printf("%s %s %d\n",__FILE__, __func__ ,__LINE__);
			break;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));


		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",addr, ntohs(clientaddr.sin_port));


		// 스레드 생성
		retval = pthread_create(&tid, NULL, ProcessClient, (void *)&client_sock);	
		if (retval != 0) { close(client_sock);}

		pthread_detach(tid);
	}

	// 뮤텍스 삭제
	pthread_mutex_destroy(&mutex_client);

	for (int i = 0; i < client_count; i++) 
			close(clients[i].sock); // 모든 클라이언트 소켓 닫기

	// 서버 소켓 닫기
	close(listen_sock);

	return 0;
}

// 클라이언트와 데이터 통신
void *ProcessClient(void *arg)
{
	int retval;
	
	ClientInfo *client_info = (ClientInfo *)malloc(sizeof(ClientInfo));
	if (client_info == NULL) {
		free(client_info);
		err_quit("client_info Memory allocation error");
	}
	memset(client_info, 0, sizeof(ClientInfo));

	client_info->sock = *((SOCKET*)arg);

	Message *msg = (Message *)malloc(CHAT_SIZE);
	if (msg == NULL) {
		err_quit("msg Memory allocation error");
	}
	memset(msg, 0, CHAT_SIZE);

	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	socklen_t addrlen;

	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(client_info->sock, (struct sockaddr *)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	

	// 클라이언트정보 추가
	pthread_mutex_lock(&mutex_client);

		// 클라이언트 soket fd 추가
		clients[client_count].sock = client_info->sock;
	
		// 클라이언트 이름 추가
		if(recv_chat_message(client_info->sock, msg,5) < 0)
			err_display_msg("recv_chat_message()");
		if(!strncmp(msg->command,"name",4))
		{
			strncpy(clients[client_count].name, msg->user, strlen(msg->user)+1);
			strncpy(client_info->name, msg->user, strlen(msg->user)+1);
			printf("%s %s\n", msg->user, msg->message);
		}
		
		// 클라이언트 수 증가
		client_count++;

	pthread_mutex_unlock(&mutex_client);


	fd_set readfds;  // select() 함수에서 사용할 파일 디스크립터 집합

	// 클라이언트로부터 받은 데이터마 다른 클라이언트들에게 메시지 보내기
	while (1) {

		// fd_set 초기화 및 소켓, 표준 입력 설정
		FD_ZERO(&readfds);
		FD_SET(client_info->sock, &readfds);  // 서버 소켓 감시
		FD_SET(STDIN_FILENO, &readfds);  // 표준 입력 감시

		struct timeval timeout;
		timeout.tv_sec = 5;  // 5초 타임아웃
		timeout.tv_usec = 0;

		// select() 호출: 서버 소켓 또는 표준 입력에서 읽을 수 있을 때까지 대기
		retval = select(client_info->sock + 1, &readfds, NULL, NULL, &timeout);
		if (retval == -1) {
			perror("select() error");
			break;
		}

		if (FD_ISSET(client_info->sock, &readfds)) {

			memset(msg, 0, CHAT_SIZE);

			if(recv_chat_message(client_info->sock, msg, 5) < 0)
			{
				client_close(client_info, msg); 
				break;	// while(1) 
			}

			printf("---------------------------------------------------------\n");
			printf("Command: %s\n%s Send to %s\nMessage: %s\n", msg->command, client_info->name, msg->user, msg->message);
			printf("---------------------------------------------------------\n");

 
						
			// 'q'가 입력된 경우 클라이언트 연결 종료 처리
			if (!strncmp(msg->message, "q",1))
			{
				client_close(client_info,msg); 
				break;	// while(1) 
			}
			else
			{
				// 여러 기능 수행
				if(!strncmp(msg->command,"name",4)) change_name(client_info,msg);	// 이름 변경
				else if(!strncmp(msg->command,"chat",4)) chat_broadcast(client_info, msg);
				else if(!strncmp(msg->command,"stream",6));
					
			}




		}

		if (FD_ISSET(STDIN_FILENO, &readfds)) {
		}

	}

	sleep(1);
	// 소켓 닫기
	close(client_info->sock);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",addr, ntohs(clientaddr.sin_port));
	pthread_exit((void*)0);

}


static void sigHandler(int signo)
{
	if(signo == SIGINT)
	{
		printf("SIGINT is catched : %d\n",signo);
		// 클라이언트 소켓들 닫기
		pthread_mutex_lock(&mutex_client);

		for (int i = 0; i < client_count; i++) {
			printf("%s clients close\n", clients[i].name);
			close(clients[i].sock);
		}

		// 서버 소켓 닫기
		close(listen_sock);
		
		pthread_mutex_unlock(&mutex_client);


		// 뮤텍스 삭제
		pthread_mutex_destroy(&mutex_client);
			
		exit(0);  // 프로그램 종료

	}


}



// 이름 변경 기능을 구현하는 함수
int change_name(ClientInfo * info, Message *msg) {
	
	pthread_mutex_lock(&mutex_client);

	for( int i =  0; i < client_count; i++)	
	{
		if(clients[i].sock == info->sock)
		{
			strncpy(clients[i].name, msg->user, NAME_SIZE - 1);
			printf("---------------------------------------------------------\n");
			printf("Changed name : %s -> %s\n",info->name,msg->user);
			printf("---------------------------------------------------------\n");
			strncpy(info->name, msg->user, NAME_SIZE - 1);	

		}
	}

	pthread_mutex_unlock(&mutex_client);

	return 0;
}



int chat_broadcast(ClientInfo * info, Message *msg)
{
	pthread_mutex_lock(&mutex_client);

	if(!strncmp(msg->user,"all",3)) // 연결된 클라이언트 제외 다른 모든 클라이언트에게 메시지 브로드캐스트
	{
		for( int i =  0; i < client_count; i++)	
		{
			if(clients[i].sock != info->sock)	send_chat_message(clients[i].sock, info->name, msg->message, 5);
		}	
	}

	pthread_mutex_unlock(&mutex_client);

}


void client_close(ClientInfo * info, Message *msg)
{
	pthread_mutex_lock(&mutex_client);

	// 클라이언트들에게 종료 메시지 전송
	for( int i =  0; i < client_count; i++)	
	{
		if(clients[i].sock != info->sock) send_chat_message(clients[i].sock, info->name, "Closed chat", 5);	// 요청 받은 클라이언트가 아닌 다른 클라이언트들에게 정보 알림
		else send_chat_message(info->sock, info->name, "q", 5);	// 요청 받은 클라이언트에게 종료하라고 알림
	}

	
	// 종료한 클라이언트 제거
	if (client_count < 0) {
		printf("No clients to remove.\n");	
	}
	else
	{	
		// clients data update
		for(int i = 0; i < client_count; i++)
		{
			if(clients[i].sock == info->sock)
			{
				for (int j = i; j < client_count - 1; j++)
				{
					clients[j] = clients[j + 1];
				}
				memset(&clients[client_count-1], 0, sizeof(ClientInfo)); // 모든 필드를 0으로 초기화
				client_count--;
				break;	// for(int i = 0; i < client_count; i++)
			}	
		}
	}

	pthread_mutex_unlock(&mutex_client);

}
