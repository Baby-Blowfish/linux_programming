#include "Common.h"


#define MAX_CLNT   20

pthread_mutex_t mutex_client;	// clients와 client_count 변수에 접근하기 위한 뮤텍스

// signal()
static void sigHandler(int signo);
SOCKET listen_sock;

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
	bool no_name_flag = true;
	int total_sent = 0;
	int to_send = len;


	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr *)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	//printf("%d %d \n",clients[client_count].sock,__LINE__);
	
	// 클라이언트정보 추가
	pthread_mutex_lock(&mutex_client);
		
		// 클라이언트 soket fd 추가
		clients[client_count].sock = client_sock;
		// 클라이언트 수 증가
		client_count++;
	
	pthread_mutex_unlock(&mutex_client);


	// 클라이언트로부터 받은 데이터마 다른 클라이언트들에게 메시지 보내기
	while (1) {

		// 데이터 수신
		int total_received = 0;
		while (total_received < sizeof(int)) {
    	int received = recv(client_sock, (char *)&len + total_received, sizeof(int) - total_received, 0);
			if (retval == SOCKET_ERROR) {
				err_display_msg("recv()");
				printf("%s %s %d\n",__FILE__,__func__,__LINE__);
				break;
			}
			else if (retval == 0) // 클라이언트와 연결이 끊어졌을 경우
			{
				printf("Client closed the connection. \n");
				break;
			}
    	total_received += received;
		}


		memset(name_msg, 0, sizeof(name_msg));	// 메시지 초기화

		// 데이터 받기(가변  길이)
		retval = recv(client_sock, name_msg, len, MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display_msg("recv()");
			printf("%s %s %d\n",__FILE__,__func__,__LINE__);
			break;
		}
		else if (retval == 0) // 클라이언트와 연결이 끊어졌을 경우 
		{
			printf("Client closed the connection. \n");
			break;
		}


		
		/* name, msg 분 리 */		
		
		memset(name_msg_copy, 0, sizeof(name_msg));

		// 원본 문자열 보호를 위해 복사본 사용
		strncpy(name_msg_copy, name_msg, sizeof(name_msg_copy));
		name_msg_copy[sizeof(name_msg_copy) - 1] = '\0'; // 문자열 종료 보장

		// 이름 추출
		name = strtok(name_msg_copy, "[]");  // 대괄호로 이름 추출
		if (name == NULL) {
			printf("이름 추출 실패\n");
			continue;	// 포멧이 잘못되면 무시하고 루프 계속
		}

		// 메시지 추출
		strtok(NULL, ":");           // ':' 이전까지 건너뛰기
		msg = strtok(NULL, "");      // 메시지 추출
		if (msg == NULL) {
			printf("메시지 추출 실패\n");
			continue;
		}

		// 앞 공백 제거
		while (isspace((unsigned char)*msg)) msg++; // 앞 공백 제거
		
		//printf("이름: %s, 메시지: %s\n", name, msg);
		

		/* 클라이언트 이름 추가*/
		pthread_mutex_lock(&mutex_client);
			if(no_name_flag == true)	
			{
				strncpy(clients[client_count-1].name, name, NAME_SIZE);
				no_name_flag = false;
			}
		pthread_mutex_unlock(&mutex_client);

	
		// 받은 데이터 출력
		printf("[TCP/%s:%d] [%d +%d byte] Received from [%s] about [%s]\n", addr, ntohs(clientaddr.sin_port),(int)sizeof(len),retval,name,name_msg);

	



		
		// 'q'가 입력된 경우 클라이언트 연결 종료 처리
		if (strcmp(msg, "q") == 0)
		{
			pthread_mutex_lock(&mutex_client);


			// 클라이언트들에게 종료 메시지 전송
			for( int i =  0; i < client_count; i++)	
			{
				if(clients[i].sock != client_sock)	// 요청 받은 클라이언트가 아닌 다른 클라이언트들에게 정보 알림
				{
					// close mesage 
					memset(name_msg, 0, strlen(name));
					sprintf(name_msg,"[%s] : Client close",name);
					len = (int)strlen(name_msg);

					// 데이터 보내기(고정 길이)
					retval = send(clients[i].sock, (char *)&len, sizeof(int), 0);
					if (retval == SOCKET_ERROR) {
						err_display_msg("send()");
						printf("%s %s %d\n",__FILE__,__func__,__LINE__);
						break;
					}

					// 데이터 보내기(가변 길이)
					retval = send(clients[i].sock, name_msg, len, 0);
					if (retval == SOCKET_ERROR) {
						err_display_msg("send()");
						printf("%s %s %d\n",__FILE__,__func__,__LINE__);
						break;
					}
					printf("[TCP/%s:%d] [%d +%d byte] [%s] Send to [%s] about [%s]\n", addr, ntohs(clientaddr.sin_port),(int)sizeof(len),retval,name,clients[i].name,name_msg);
				}
				else	// 요청 받은 클라이언트에게 종료하라고 알림
				{	
					// send to client about "q"
					memset(name_msg, 0, strlen(name));
					sprintf(name_msg,"[%s] : q",name);
					len = (int)strlen(name_msg);

					// 데이터 보내기(고정 길이)
					retval = send(clients[i].sock, (char *)&len, sizeof(int), 0);
					if (retval == SOCKET_ERROR) {
						err_display_msg("send()");
						printf("%s %s %d\n",__FILE__,__func__,__LINE__);
						break;
					}

					// 데이터 보내기(가변 길이)
					retval = send(clients[i].sock, name_msg, len, 0);
					if (retval == SOCKET_ERROR) {
						err_display_msg("send()");
						printf("%s %s %d\n",__FILE__,__func__,__LINE__);
						break;
					}
					
					printf("[TCP/%s:%d] [%d +%d byte] Send to [%s] about [%s]\n", addr, ntohs(clientaddr.sin_port),(int)sizeof(len),retval,name,name_msg);
				}
			}
			

			// 종료한 클라이언트 제거
			if (client_count < 0) {
				printf("No clients to remove.\n");	
				 // 클라이언트가 없으면 함수 종료
				pthread_exit((void *)0);
			}
			else
			{	
				// clients data update
				for(int i = 0; i < client_count; i++)
				{
					if(clients[i].sock == client_sock)
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


			// 클라이언트를 관리하는 쓰래드 종료
			break;	// while(1) 
		} 정상적으로 입력되었다면 다른 클라이언트들에게 메시지 브로드캐스팅
		else
		{
			pthread_mutex_lock(&mutex_client);
			
			
			for( int i =  0; i < client_count; i++)	// 모든 클라이언트에게 메시지 브로드캐스트
			{
				if(clients[i].sock != client_sock)	//  자신에게는 보내지 않음
				{
					
					
					// 데이터 보내기(고정 길이)
					retval = send(clients[i].sock, (char *)&len, sizeof(int), 0);
					if (retval == SOCKET_ERROR) {
						err_display_msg("send()");
						printf("%s %s %d\n",__FILE__,__func__,__LINE__);
						break;
					}

					// 데이터 보내기(가변 길이) - 모든 데이터를 다 보낼 때까지 반복 전송
					total_sent = 0;
					to_send = len;
					while (total_sent < len) {
						retval = send(client_info.sock, buf + total_sent, to_send, 0);
						if (retval == SOCKET_ERROR) {
							err_display_msg("send()");
							printf("%s %s %d\n",__FILE__,__func__,__LINE__);
							return break;
						}
						total_sent += retval;
						to_send -= retval;
					}


					printf("[TCP/%s:%d] [%d +%d byte] [%s] Send to [%s] about [%s]\n", addr, ntohs(clientaddr.sin_port),(int)sizeof(len),retval,name,clients[i].name,name_msg);
				}
			}		
			pthread_mutex_unlock(&mutex_client);
		}


	}
	
	sleep(1);
	// 소켓 닫기
	close(client_sock);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",addr, ntohs(clientaddr.sin_port));
	pthread_exit((void*)0);

}

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
			printf("%s %s %d\n",__FILE__,__func__,__LINE__);
			break;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));

		// 스레드 생성
		retval = pthread_create(&tid, NULL, ProcessClient, (void *)(long long)client_sock);
		if (retval != 0) { close(client_sock);}

		pthread_detach(tid);
	}
	
	// 뮤텍스 삭제
	pthread_mutex_destroy(&mutex_client);

	for (int i = 0; i < client_count; i++) {
			close(clients[i].sock); // 모든 클라이언트 소켓 닫기
	}

	// 서버 소켓 닫기
	close(listen_sock);
	return 0;
}

static void sigHandler(int signo)
{
	if(signo == SIGINT)
	{
		printf("SIGINT is catched : %d\n",signo);
	}

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

