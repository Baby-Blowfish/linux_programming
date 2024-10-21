#include "Common.h"
#define SCREENSIZE  (1920*1080) 								// frame buffer의 크기
#define BUFFER_MAX  (CHAT_SIZE + SCREENSIZE)		// 받을 데이터의 최대 크기



// 클라이언트 정보를 저장할 구조체
typedef struct {
	SOCKET sock;
	char name[NAME_SIZE];
} ClientInfo;

// 메인 메뉴 함수
void show_main_menu();

int send_fixed_length_data(int sock	, uint32_t data, int max_retries) ;
int send_variable_length_data(int sock, const char *buf, int len, int max_retries);

void *ChatThread(void *arg);
void *VideoStreamThread(void *arg);



int main(int argc, char *argv[])
{
	int retval, choice;


  ClientInfo *client_info = (ClientInfo *)malloc(sizeof(ClientInfo));
	if (client_info == NULL) {
		free(client_info); // 할당된 메모리 해제
		err_quit("Memory allocation error");
	}

	if (argc != 2)
{
    printf("Usage : %s <Name>\n", argv[0]);
    free(client_info);  // 할당된 메모리 해제
    exit(1);
}

// 들어온 인자 즉, 이름의 크기가 20byte 이상인 경우 에러 처리, name이 최대 20byte 이므로 '\0'을 뺸 19byte까지만 받아야함.
if (strlen(argv[1]) >= NAME_SIZE)
{
    printf("Error: Input exceeds maximum Name size of %d characters.\n", NAME_SIZE - 1);
    free(client_info);  // 할당된 메모리 해제
    exit(1);
}

		// 이름을 안전하게 복사
  memset(client_info->name, 0, NAME_SIZE);  // 이름 배열을 0으로 초기화 (널 종결자 포함)
  strncpy(client_info->name, argv[1], strlen(argv[1]));  // 이름 크기만큼 초기화,  strlen()은 '\0'이 마지막에 있어야 올바른 동작(argv[1] 문자열로 받아옴)
	client_info->name[NAME_SIZE - 1] = '\0';  // 안전한 널 종결자 추가

	// 소켓 생성
	client_info->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (client_info->sock == INVALID_SOCKET) 
	{
		free(client_info); // 할당된 메모리 해제
		err_quit("socket()");
	}
		

	// connect()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(client_info->sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) 
	{
		free(client_info); // 할당된 메모리 해제
		err_quit("connect()");
	}

	printf("Connected to the server.\n");




	// 서버에 이름 등록
	char *buf = (char*)malloc(CHAT_SIZE*sizeof(char));  //
	if (buf == NULL) {
			perror("Memory allocation error");
			exit(1);
	}

	// 메시지를 포맷: [name] : <message>
  format_message("name", argv[1], "Register my name", buf);

	// 고정 길이 데이터 전송 (메시지 길이 전송)
	if (send_fixed_length_data(client_info->sock, (uint32_t)strlen(buf), 5) == -1) {
			printf("Failed to send fixed length data.\n");
			return;
	}

	// 가변 길이 데이터 전송 (메시지 본문 전송)
	if (send_variable_length_data(client_info->sock, buf, (uint32_t)strlen(buf), 5) == -1) {
			printf("Failed to send variable length data.\n");
			return;
	}

	free(buf);

	pthread_t chat_tid;
	pthread_t video_tid;
	
	while(1)
	{
		// 메인 메뉴를 표시
		show_main_menu();
		scanf("%d", &choice);  // 사용자가 선택한 기능

		if (choice == 1) {
				// 채팅 기능 실행
				pthread_t chat_tid;
				retval = pthread_create(&chat_tid, NULL, ChatThread, (void *)(client_info));
				if (retval != 0) { close(client_info->sock); free(client_info); err_quit("pthread_create()");};
				pthread_join(chat_tid, NULL);  // 채팅 스레드가 끝날 때까지 대기
		} else if (choice == 2) {
				// 비디오 스트리밍 기능 실행
				pthread_t video_tid;
				retval = pthread_create(&chat_tid, NULL, ChatThread, (void *)(client_info));
				if (retval != 0) { close(client_info->sock); free(client_info); err_quit("pthread_create()");};
				pthread_join(chat_tid, NULL);  // 채팅 스레드가 끝날 때까지 대기
		} else if (choice == 3) {
				// 프로그램 종료
				printf("프로그램을 종료합니다.\n");
				break;
		} else {
				printf("잘못된 선택입니다. 다시 선택하세요.\n");
		}
	}


	// 스레드  종료 대기
	pthread_join(chat_tid, NULL);
	// 스레드  종료 대기
	pthread_join(video_tid, NULL);
	free(client_info); // 동적 할당 해제
	// 소켓 닫기
	close(client_info->sock);

	return 0;
}




// 메인 메뉴 표시 함수
void show_main_menu() {
    printf("\n====== 메인 메뉴 ======\n");
    printf("1. 채팅\n");
    printf("2. 비디오 스트리밍\n");
    printf("3. 종료\n");
    printf("선택: ");
}

// 메시지를 포맷하는 함수
void format_message(const Message *msg, char *formatted_message) {
    snprintf(formatted_message, BUFFER_CHUNK_SIZE, "%s | %s | %d | %s",
             msg->command, msg->user, msg->message_length, msg->message);
}

// 고정 길이 데이터 전송 함수
int send_fixed_length_data(int sock, uint32_t data, int max_retries) {
    uint32_t network_data = htonl(data);  // 엔디안 변환
    int total_sent = 0;
    int retry_count = 0;

    while (total_sent < sizeof(uint32_t)) {
        int sent = send(sock, ((char *)&network_data) + total_sent, sizeof(uint32_t) - total_sent, 0);
        if (sent == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                if (++retry_count > max_retries) {
                    printf("Max retries reached, aborting.\n");
                    return -1;
                }
                continue;  // 재시도
            } else {
                perror("send() error");
                return -1;
            }
        }
        total_sent += sent;
        retry_count = 0;  // 정상 전송되었으면 재시도 횟수 초기화
    }
    return 0;
}

// 가변 길이 데이터 전송 함수
int send_variable_length_data(int sock, const char *buf, int len, int max_retries) {
    int total_sent = 0;
    int retry_count = 0;

    while (total_sent < len) {
        int sent = send(sock, buf + total_sent, len - total_sent, 0);
        if (sent == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                if (++retry_count > max_retries) {
                    printf("Max retries reached, aborting.\n");
                    return -1;
                }
                continue;  // 재시도
            } else {
                perror("send() error");
                return -1;
            }
        }
        total_sent += sent;
        retry_count = 0;  // 정상 전송되었으면 재시도 횟수 초기화
    }
    return 0;
}


// 메시지를 파싱하는 함수
void parse_message(char *data, Message *msg) {
    char *temp;

    // 첫 번째 필드: command
    temp = strtok(data, "|");
    if (temp != NULL) {
        // 앞뒤 공백 제거 및 값 복사
        while (*temp == ' ') temp++;
        strncpy(msg->command, temp, COMMAND_SIZE - 1);
        msg->command[COMMAND_SIZE - 1] = '\0';
    }

    // 두 번째 필드: user
    temp = strtok(NULL, "|");
    if (temp != NULL) {
        // 앞뒤 공백 제거 및 값 복사
        while (*temp == ' ') temp++;
        strncpy(msg->user, temp, NAME_SIZE - 1);
        msg->user[NAME_SIZE - 1] = '\0';
    }

    // 세 번째 필드: message_length
    temp = strtok(NULL, "|");
    if (temp != NULL) {
        msg->message_length = atoi(temp);  // 문자열을 정수로 변환
    }

    // 네 번째 필드: message
    temp = strtok(NULL, "|");
    if (temp != NULL) {
        strncpy(msg->message, temp, MESSAGE_SIZE - 1);
        msg->message[MESSAGE_SIZE - 1] = '\0';  // 널 문자 추가
    }
}




// 고정 길이 데이터 수신 함수
int receive_fixed_length(SOCKET sock, int32_t *data_size) {
    int32_t network_data_size;
    int retval = recv(sock, (char*)&network_data_size, sizeof(int32_t), MSG_WAITALL);

    if (retval == SOCKET_ERROR) {
        perror("recv() error");
        return -1;
    } else if (retval == 0) {  // 클라이언트와 연결이 끊어졌을 경우
        printf("Client closed the connection.\n");
        return -1;
    }

    *data_size = ntohl(network_data_size);  // 엔디안 변환
    return 0;
}

// 가변 길이 데이터 수신 함수 (부분적으로)
int receive_variable_length(SOCKET sock, int data_size) {
    char buffer[BUFSIZE];
    int total_bytes_received = 0;
    int bytes_received;

    while (total_bytes_received < data_size) {
        int chunk_size = (data_size - total_bytes_received) < BUFSIZE ? (data_size - total_bytes_received) : BUFSIZE;
        bytes_received = recv(sock, buffer, chunk_size, 0);
        if (bytes_received <= 0) {
            perror("recv() error");
            return -1;
        }

        total_bytes_received += bytes_received;

        // 받은 데이터 처리 (예: 화면에 출력)
        printf("Received chunk: %d bytes\n", bytes_received);
    }

    printf("Total received: %d bytes\n", total_bytes_received);
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



void *ChatThread(void *arg)
{

	int retval, max_retries = 5;  // 소켓 오류시 시도 횟수
	uint32_t len;  // 고정 길이 데이터(가변길이 데이터의 크기를 나타냄)

	Message *chat_buf = (Message *)malloc(CHAT_SIZE);  // 큰 데이터를 받기 위한 버퍼 할당
	if (chat_buf == NULL) {
		err_quit("chat_buf Memory allocation error");
	}
	memset(chat_buf, 0, BUFFER_CHUNK_SIZE);

	char *formatted_msg = (char *)malloc(BUFFER_CHUNK_SIZE);  // 큰 데이터를 받기 위한 버퍼 할당
	if (formatted_msg == NULL) {
		err_quit("formatted_msg Memory allocation error");
	}
	memset(formatted_msg, 0, BUFFER_CHUNK_SIZE);

	ClientInfo *client_info = (ClientInfo *)arg;
	if (client_info == NULL) {
		err_quit("client_info Memory allocation error");
	}

	fd_set readfds;  // select() 함수에서 사용할 파일 디스크립터 집합

	while (1)
	{
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

		// 서버로부터 메시지 수신
		if (FD_ISSET(client_info->sock, &readfds)) {
			memset(chat_msg, 0, sizeof(chat_msg));

			// 고정 길이 데이터 수신 (데이터의 크기)
			if (receive_fixed_length(client_info->sock, &len) == -1) {
				printf("Error receiving fixed length data.\n");
				break;
			}

			// 가변 길이 데이터 수신
			if (receive_variable_length(client_info->sock, len) == -1) {
				printf("Error receiving variable length data.\n");
				break;
			}

			// 메시지 파싱
			strncpy(name_msg_copy, buf, sizeof(name_msg_copy));
			name_msg_copy[sizeof(name_msg_copy) - 1] = '\0';  // 안전하게 문자열 종료
			// 이름 파싱
			name = strtok(name_msg_copy, "[]");
			if (name == NULL) {
				printf("이름을 추출하는데 실패했습니다.\n");
				continue;  // 포맷이 잘못되면 무시하고 루프 계속
			}

			// 메시지 파싱
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
			if (!strcmp(msg, "q")) {
				printf("q가 입력되어서 종료합니다!\n");
				break;
			}
		}

		// 표준 입력(키보드 입력), 서버로 데이터 전송
		if (FD_ISSET(STDIN_FILENO, &readfds)) {

			memset(chat_buf, 0, CHAT_SIZE);
			memset(formatted_msg, 0, BUFFER_CHUNK_SIZE);

			printf("Input a string (max length: %d): ", MESSAGE_SIZE -1);
			
			if (fgets(chat_buf->message, MESSAGE_SIZE - 1, stdin) == NULL) {
				err_display_msg("fgets()");
				continue;
			}

			// 버퍼의 마지막 문자가 '\n' 또는 '\r'이 아니면 입력이 너무 길다고 판단
			if (strchr(chat_buf->message, '\n') == NULL && strchr(chat_buf->message, '\r') == NULL) {
				printf("Error: Input exceeds maximum buffer size of %d characters.\n", BUFFER_CHUNK_SIZE - 1);
				int ch;
				while ((ch = getchar()) != '\n' && ch != EOF);  // 나머지 입력 데이터를 스트림에서 제거
				continue;  // 입력 처리 안함
			}

			
			len = (uint32_t)strlen(chat_buf->message);		// mesg의 크기
			chat_buf->message[len - 1] = '\0';		// '\n', '\r' 문자 제거


			strncpy(chat_buf->command, "chat", COMMAND_SIZE - 1);  // command 필드에 값 복사
			strncpy(chat_buf->user, client_info->name, NAME_SIZE - 1);           // user 필드에 값 복사
			chat_buf->message_length = len;              // message_length 필드에 값 할당
			//strncpy(chat_buf->message, message, MESSAGE_SIZE - 1);  

			// 메시지를 포맷
    	format_message(chat_buf, formatted_msg);
			len = (uint32_t)strlen(formatted_msg);

			// 고정 길이 데이터 전송
			if (send_fixed_length_data(client_info->sock, len, max_retries) == -1) {
				printf("Failed to send fixed length data.\n");
				break;
			}

			// 가변 길이 데이터 전송
			if (send_variable_length_data(client_info->sock, formatted_msg, len, max_retries) == -1) {
				printf("Failed to send variable length data.\n");
				break;
			}


			// "q"가 입력되면 소켓 종료
			if (!strncmp(chat_buf->message, "q", 1)) {
				printf("Client requests the server to close ChatThread \n");
				break;
			}
		}
	
	}

	// 메모리 해제 및 쓰레드 종료
	free(chat_buf);
	free(formatted_msg);
	printf("ChatThread 종료\n");
	return 0;
}


void *VideoStreamThread(void *arg)
{
	int retval;
	int max_retries = 5;		// 소켓 오류시 시도 횟수
	// 고정 길이 데이터(가변길이 데이터의 크기를 나타냄)
	uint32_t len;

	char cmd[CMD_SIZE];		// 커맨드는 5글자 이하


	// 가변 길이 데이터
	char chat_msg[CMD_SIZE+CHAT_SIZE];  										// 채팅일 경우
	char *buf = (char*)malloc(BUFFER_MAX*sizeof(char));		// BUFER_CHUNK_SIZE보다 큰경우 ex) 이미지
	if (buf == NULL) {
			err_quit("Memory allocation error");
	}

	ClientInfo *client_info = (struct ClientInfo *)arg;

	// 메시지 파싱 변수
	char name_msg_copy[BUFFER_CHUNK_SIZE];
	char *name;														// 보낸 사람  이름
	char *msg;														// 보낸 사람  메시지


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

		memset(chat_msg, 0, sizeof(chat_msg));

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

	// 표준 입력(키보드 입력), 서버로 데이터 전송
    if (FD_ISSET(STDIN_FILENO, &readfds)) {
			
		memset(chat_msg, 0, sizeof(chat_msg));

		printf("Input a string (max length: %d): ", CMD_SIZE + CHAT_SIZE - 1);

		if (fgets(chat_msg, CMD_SIZE + CHAT_SIZE - 1, stdin) == NULL)
		{
				err_display_msg("fgets()");
				continue;
		}

		// 서버에 이름 등록
		char *buf = (char*)malloc((CMD_SIZE + strlen(client_info->name))*sizeof(char));  //
		if (buf == NULL) {
				perror("Memory allocation error");
				exit(1);
		}

		/*
		// name 커맨드 : 이름을 갱신
		sprintf(buf, "[name] : %s", client_info->name);	// 
		uint32_t len = (uint32_t)strlen(buf);	// 공백은 '\n'==NULL 문자가 아님!, strlen은 NULL문자를 제외한 바이트를 반환
		*/
		

		// 고정 길이 데이터 전송
		if (send_fixed_length_data(client_info->sock, len, 5) == -1) {
				printf("Failed to send fixed length data.\n");
				free(client_info); // 할당된 메모리 해제
				free(buf);
				exit(1);
		}

		// 가변 길이 데이터 전송
		if (send_variable_length_data(client_info->sock, buf, len, 5) == -1) {
				printf("Failed to send variable length data.\n");
				free(client_info); // 할당된 메모리 해제
				free(buf);
				exit(1);
		}

		// 버퍼의 마지막 문자가 '\n'이 아니면 입력이 너무 길다고 판단
    if (strchr(chat_msg, '\n') == NULL || strchr(chat_msg, '\r') == NULL) {
			printf("Error: Input exceeds maximum buffer size of %d characters.\n", BUFFER_CHUNK_SIZE - 1);
			int ch;
			while ((ch = getchar()) != '\n' && ch != EOF);	// 나머지 입력 데이터를 스트림에서 제거
			continue;  // 입력 처리 안함
    }
		else if (chat_msg[0] == '\0') {
    	continue;  // 빈 문자열이면 전송하지 않음
		}

		// '\n', '\r' 문자 제거
		len = (int)strlen(chat_msg);	// 현재 읽은 데이터의 길이
		chat_msg[len - 1] = '\0';

		// 이름 추가
		sprintf(chat_msg, "[name] : %s",chat_msg);	// 

		// 고정 길이 데이터 전송 
    if (send_fixed_length_data(client_info->sock, len, max_retries) == -1) {
        printf("Failed to send fixed length data.\n");
        break;
    }

    // 가변 길이 데이터 전송 
    if (send_variable_length_data(client_info->sock, chat_msg, len, max_retries) == -1) {
        printf("Failed to send variable length data.\n");
        break;
    }

		// "q"가 입력되면 소켓 종료
		if (!strcmp(chat_msg, "q"))
			// 쓰레드 종료
			printf("Client requet to server about communication cloes\n");
			break;
		}

	}

	// 쓰레드 종료
	printf("Communication thread 종료\n");
	return 0;

}



