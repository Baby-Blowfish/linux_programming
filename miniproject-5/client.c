#include "Common.h"


void *CommunicationThread(void *arg);

struct client 
{
	SOCKET sock;
	char name[NAME_SIZE];
};



int main(int argc, char *argv[])
{
	int retval;

	struct client client_info;		// 클라이언트 정보가 담긴 구조체

	if(argc != 2)
	{
		printf("Usage : %s <Name>\n",argv[0]);
		exit(1);
	}
	
	// 이름을 안전하게 복사
	memset(client_info.name, 0, NAME_SIZE);
	strncpy(client_info.name, argv[1], NAME_SIZE - 1);

	// 소켓 생성
	client_info.sock = socket(AF_INET, SOCK_STREAM, 0);
	if (client_info.sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(client_info.sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");
	printf("Connected to the server.\n");

	// 클라이언트의 이름을 서버에 알림
	char buf[BUFSIZE];  // 동적 할당 대신 정적 할당
	if (buf == NULL) {
			perror("Memory allocation error");
			exit(1);
	}

	// 메모리에 데이터를 저장
	sprintf(buf, "[%s] : is my name", client_info.name);
	int len = (int)strlen(buf);	// '\0'를 고려하지 않은크기임!!

	// 데이터 보내기(고정 길이)
	retval = send(client_info.sock, (char *)&len, sizeof(int), 0);
	if (retval == SOCKET_ERROR) {
		err_display_msg("send()");
		return -1;
	}


	// 데이터 보내기(가변 길이) - 모든 데이터를 다 보낼 때까지 반복 전송
	int total_sent = 0;
	int to_send = len;
	while (total_sent < len) {
		retval = send(client_info.sock, buf + total_sent, to_send, 0);
		if (retval == SOCKET_ERROR) {
			err_display_msg("send()");
			return -1;
		}
		total_sent += retval;
		to_send -= retval;
	}

	// 스레드 생성
	pthread_t tid;
	retval = pthread_create(&tid, NULL, CommunicationThread, (void *)(&client_info));
	if (retval != 0) { close(client_info.sock); };

	/*
	while(1)
	{
	}
	*/
	
	// 스레드  종료 대기
	pthread_join(tid, NULL);

	// 소켓 닫기
	close(client_info.sock);
	return 0;
}

void *CommunicationThread(void *arg)
{
	int retval;
	struct client *client_info = (struct client *)arg;

	char name_msg[BUFSIZE + NAME_SIZE];  	// 가변 길이 데이터 가능
	int len;                              // 고정 길이 데이터(가변길이 데이터의 크기를 나타냄)

	// 메시지 파싱 변수
	char name_msg_copy[BUFSIZE + NAME_SIZE];
	char *name;														// 보낸 사람  이름
	char *msg;														// 보낸 사람의 메시지
	
	char msg_send[BUFSIZE];  							// 보낼 데이터


	  fd_set readfds;  // select() 함수에서 사용할 파일 디스크립터 집합

	

	while(1)
	{
		// fd_set 초기화 및 소켓, 표준 입력 설정
		FD_ZERO(&readfds);
		FD_SET(client_info->sock, &readfds);         // 서버 소켓 감시
		FD_SET(STDIN_FILENO, &readfds); // 표준 입력 감시

		struct timeval timeout;
		timeout.tv_sec = 5;  // 5초 타임아웃
		timeout.tv_usec = 0;

		// select() 호출: 서버 소켓 또는 표준 입력에서 읽을 수 있을 때까지 대기
		retval = select(client_info->sock + 1, &readfds, NULL, NULL, &timeout);
		if (retval == -1) {
				perror("select() error");
				break;
		}




		// 서버로부터 메시지 수신
		if (FD_ISSET(client_info->sock, &readfds)) {

		memset(name_msg, 0, sizeof(name_msg));

		// 데이터 받기(고정 길이)
		retval = recv(client_info->sock, (char *)&len, sizeof(int), MSG_WAITALL);
		if (retval <= 0) {
			if (retval == 0) {
				printf("Server closed the connection.\n");
			} else {
				perror("recv() error");
			}
			break;  // 서버가 소켓을 닫았거나 에러 발생 시 종료
		}


		// 데이터 받기(가변 길이)
		retval = recv(client_info->sock, name_msg, len, MSG_WAITALL);
		if (retval <= 0) {
			if (retval == 0) {
				printf("Server closed the connection.\n");
			} else {
				perror("recv() error");
			}
			break;  // 서버가 소켓을 닫았거나 에러 발생 시 종료
		}



			// 메시지 파싱
			strncpy(name_msg_copy, name_msg, sizeof(name_msg_copy));
			name_msg_copy[sizeof(name_msg_copy) - 1] = '\0'; // 안전하게 문자열 종료
			// 이름 
			name = strtok(name_msg_copy, "[]");
			if (name == NULL) {
				printf("이름을 추출하는데 실패했습니다.\n");
				continue;  // 포맷이 잘못되면 무시하고 루프 계속
			}
			
			// 메시지
			strtok(NULL, ":");  // ':' 구분자를 건너뜀
			msg = strtok(NULL, "");  // 메시지 추출
			if (msg == NULL) {
				printf("메시지 추출 오류\n");
				continue;  // 포맷이 잘못되면 무시하고 루프 계속
			}

			// 앞 공백 제거
			while (isspace((unsigned char)*msg)) msg++;  // 앞 공백 제거

			// 결과 출력
			printf("받은 데이터 : [%s : %s]\n", name, msg);

      // "q"가 입력되면 소켓 종료
      if(!strcmp(msg,"q"))
	  {
		printf("q가 입력되어서 종료합니다!\n"); 
		break;
	  }
    }		

	// 표준 입력(키보드 입력), 서버로 데이터 전송(메시지만)
    if (FD_ISSET(STDIN_FILENO, &readfds)) {
			
		memset(msg_send, 0, sizeof(msg_send));

		if (fgets(msg_send, BUFSIZE, stdin) == NULL)
		{
				perror("fgets()");
				break;\
		}

		// '\n', '\r' 문자 제거
		len = (int)strlen(msg_send);
		if (msg_send[len - 1] == '\n' || msg_send[len - 1] == '\r') {
			msg_send[len - 1] = '\0';
		}

		if (msg_send[0] == '\0') {
    		continue;  // 빈 문자열이면 전송하지 않음
		}


		// 데이터 보내기(고정 길이)
		retval = send(client_info->sock, (char *)&len, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display_msg("send()");
			break;
		}

		// 데이터 보내기(가변 길이)
		retval = send(client_info->sock, msg_send, len, 0);
		if (retval == SOCKET_ERROR) {
			err_display_msg("send()");
			break;
		}

		// "q"가 입력되면 소켓 종료
		if (!strcmp(msg_send, "q"))
			// 쓰레드 종료
			printf("Client requet to server about communication cloes\n");
			break;
		}

	}

	// 쓰레드 종료
	printf("Communication thread 종료\n");
	return 0;

}



// 1. 고정 길이 받기 (데이터 크기 정보 수신)
int receive_fixed_length(SOCKET sock, int *data_size) {
   
	int retval = recv(sock, (char *)data_size, sizeof(int), MSG_WAITALL);
    
	if (retval == SOCKET_ERROR) {
		err_display_msg("recv()");
		printf("%s %s %d\n",__FILE__,__func__,__LINE__);
        return -1;
	}
	else if (retval == 0) // 클라이언트와 연결이 끊어졌을 경우
	{
		printf("Client closed the connection. \n");
        return -1;
	}

}



// 2. 부분 길이 방식으로 가변 데이터 받기
int receive_variable_length(SOCKET sock, int data_size) {
    char buffer[BUFSIZE];
    int total_bytes_received = 0;
    int bytes_received;

    while (total_bytes_received < data_size) {
        int chunk_size = (data_size - total_bytes_received) < BUFSIZE ? (data_size - total_bytes_received) : BUFSIZE;
        bytes_received = recv(sock, buffer, chunk_size, 0);
        if (bytes_received <= 0) {
            perror("Error receiving variable length data");
            return -1;
        }

        total_bytes_received += bytes_received;

        // 받은 데이터 처리 (예: 화면에 출력)
        printf("Received chunk: %d bytes\n", bytes_received);
    }

    printf("Total received: %d bytes\n", total_bytes_received);
    return 0;
}
