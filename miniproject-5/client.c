#include "Common.h"
#define SCREENSIZE  (1920*1080) 								// frame buffer의 크기
#define BUFFER_MAX  (CHAT_SIZE + SCREENSIZE)		// 받을 데이터의 최대 크기


// 메인 메뉴 함수
void show_main_menu();
void format_message(const Message *msg, char *formatted_message);
void parse_message(char *data, Message *msg);

int send_fixed_length_data(SOCKET sock	, uint32_t data, int max_retries) ;	// 고정 길이 데이터 송신 
int send_variable_length_data(SOCKET sock, const char *buf, int len, int max_retries);	// 가변 길이 데이터 송신 
int recv_fixed_length_data(SOCKET sock, uint32_t *data, int max_retries);	// 고정 길이 데이터 수신 
int recv_variable_length_data(SOCKET sock, char *buf, int len, int max_retries); // 가변 길이 데이터 수신 함수

int change_name(SOCKET sock, const char *new_name, const char *mmessage, int max_retries);
int send_chat_message(SOCKET sock, const char *username, int max_retries);


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

	// 클라이언트 이름 등록
	change_name( client_info->sock, client_info->name,  "this is my name", 5);

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
				retval = pthread_create(&video_tid, NULL, VideoStreamThread, (void *)(client_info));
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



// 고정 길이 데이터 전송 함수
int send_fixed_length_data(SOCKET sock, uint32_t data, int max_retries) {
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
int send_variable_length_data(SOCKET sock, const char *buf, int len, int max_retries) {
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


// 고정 길이 데이터 수신 함수
int recv_fixed_length_data(SOCKET sock, uint32_t *data, int max_retries) {
	uint32_t network_data;
	int total_received = 0;
	int retry_count = 0;

	while (total_received < sizeof(uint32_t)) {
		int received = recv(sock, ((char *)&network_data) + total_received, sizeof(uint32_t) - total_received, 0);
		if (received == -1) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				if (++retry_count > max_retries) {
						printf("Max retries reached, aborting.\n");
						return -1;
				}
				continue;  // 재시도
			} else {
				perror("recv() error");
				return -1;
			}
		}
		total_received += received;
		retry_count = 0;  // 정상 수신되었으면 재시도 횟수 초기화
	}

	// 엔디안 변환 후 데이터 반환
	*data = ntohl(network_data);
	return 0;
}


// 가변 길이 데이터 수신 함수
int recv_variable_length_data(SOCKET sock, char *buf, int len, int max_retries) {
	int total_received = 0;
	int retry_count = 0;

	while (total_received < len) {
		int received = recv(sock, buf + total_received, len - total_received, 0);
		if (received == -1) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				if (++retry_count > max_retries) {
						printf("Max retries reached, aborting.\n");
						return -1;
				}
				continue;  // 재시도
			} else {
				perror("recv() error");
				return -1;
			}
		}
		total_received += received;
		retry_count = 0;  // 정상 수신되었으면 재시도 횟수 초기화
	}

	return 0;
}





// 이름 변경 기능을 구현하는 함수
int change_name(SOCKET sock, const char *new_name, const char *mmessage, int max_retries) {
    uint32_t len;  // 고정 길이 데이터(가변 길이 데이터의 크기를 나타냄)

    // Message 구조체 생성 및 초기화
    Message *chat_buf = (Message *)malloc(CHAT_SIZE);  
    if (chat_buf == NULL) {
        err_quit("chat_buf Memory allocation error");
				free(chat_buf);
        return -1;
    }
    memset(chat_buf, 0, CHAT_SIZE);

    // 포맷된 메시지를 저장할 버퍼
    char *formatted_msg = (char *)malloc(BUFFER_CHUNK_SIZE);  
    if (formatted_msg == NULL) {
        err_quit("formatted_msg Memory allocation error");
				free(chat_buf);
        free(formatted_msg);
        return -1;
    }
    memset(formatted_msg, 0, BUFFER_CHUNK_SIZE);

    // Message 구조체에 데이터 할당
    strncpy(chat_buf->command, "name", COMMAND_SIZE - 1);         // command 필드에 값 복사
    strncpy(chat_buf->user, new_name, NAME_SIZE - 1);         // 현재 사용자 이름을 user 필드에 복사
    chat_buf->message_length = strlen(mmessage);                  // 변경할 이름의 길이를 message_length에 할당
    strncpy(chat_buf->message, mmessage, MESSAGE_SIZE - 1);       // 변경할 이름을 message 필드에 복사

    // 메시지를 포맷
    format_message(chat_buf, formatted_msg);
    len = (uint32_t)strlen(formatted_msg);

    // 고정 길이 데이터 전송
    if (send_fixed_length_data(sock, len, max_retries) == -1) {
        printf("Failed to send fixed length data.\n");
        free(chat_buf);
        free(formatted_msg);
        return -1;
    }

    // 가변 길이 데이터 전송
    if (send_variable_length_data(sock, formatted_msg, len, max_retries) == -1) {
        printf("Failed to send variable length data.\n");
        free(chat_buf);
        free(formatted_msg);
        return -1;
    }

    // 메모리 해제
    free(chat_buf);
    free(formatted_msg);

    return 0;
}

// 채팅 메시지를 보내는 함수
int send_chat_message(SOCKET sock, const char *username, int max_retries) {
    uint32_t len;

    // Message 구조체 생성 및 초기화
    Message *chat_buf = (Message *)malloc(CHAT_SIZE);
    if (chat_buf == NULL) {
        perror("chat_buf Memory allocation error");
				free(chat_buf);
        return -1;
    }
    memset(chat_buf, 0, CHAT_SIZE);

    // 포맷된 메시지를 저장할 버퍼
    char *formatted_msg = (char *)malloc(BUFFER_CHUNK_SIZE);
    if (formatted_msg == NULL) {
        perror("formatted_msg Memory allocation error");
				free(chat_buf);
        free(formatted_msg);
        return -1;
    }
    memset(formatted_msg, 0, BUFFER_CHUNK_SIZE);

    // 채팅 메시지 입력 받기
    printf("Input a message (max length: %ld): ", MESSAGE_SIZE - 1);
    if (fgets(chat_buf->message, MESSAGE_SIZE - 1, stdin) == NULL) {
        perror("fgets() error");
        free(chat_buf);
        free(formatted_msg);
        return -1;
    }

    // 입력된 메시지가 너무 긴 경우 처리
    if (strchr(chat_buf->message, '\n') == NULL && strchr(chat_buf->message, '\r') == NULL) {
        printf("Error: Input exceeds maximum buffer size of %ld characters.\n", MESSAGE_SIZE - 1);
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF);
        free(chat_buf);
        free(formatted_msg);
        return -1;
    }

    // '\n', '\r' 문자 제거
    len = (uint32_t)strlen(chat_buf->message);
    if (len > 0 && (chat_buf->message[len - 1] == '\n' || chat_buf->message[len - 1] == '\r')) {
        chat_buf->message[len - 1] = '\0';
        len--;
    }

    // Message 구조체에 값 할당
    strncpy(chat_buf->command, "chat", COMMAND_SIZE - 1);
    strncpy(chat_buf->user, username, NAME_SIZE - 1);
    chat_buf->message_length = len;

    // 메시지를 포맷
    format_message(chat_buf, formatted_msg);
    len = (uint32_t)strlen(formatted_msg);

    // 고정 길이 데이터 전송
    if (send_fixed_length_data(sock, len, max_retries) == -1) {
        printf("Failed to send fixed length data.\n");
        free(chat_buf);
        free(formatted_msg);
        return -1;
    }

    // 가변 길이 데이터 전송
    if (send_variable_length_data(sock, formatted_msg, len, max_retries) == -1) {
        printf("Failed to send variable length data.\n");
        free(chat_buf);
        free(formatted_msg);
        return -1;
    }

    // "q"가 입력되면 소켓 종료
    if (!strncmp(chat_buf->message, "q", 1)) {
        free(chat_buf);
        free(formatted_msg);
        return 1;  // "q" 입력 시 종료 신호 반환
    }

    // 메모리 해제
    free(chat_buf);
    free(formatted_msg);

    return 0;  // 정상적으로 메시지를 보냄
}


// 채팅 메시지를 수신하는 함수
int recv_chat_message(SOCKET sock, Message *msg, int max_retries) {

    uint32_t len;

    char *formatted_msg = (char *)malloc(BUFFER_CHUNK_SIZE);
    if (formatted_msg == NULL) {
        perror("formatted_msg Memory allocation error");
        return -1;
    }
    memset(formatted_msg, 0, BUFFER_CHUNK_SIZE);

    // 고정 길이 데이터 수신 (메시지의 전체 길이를 먼저 받음)
    if (recv_fixed_length_data(sock, &len, max_retries) == -1) {
        free(formatted_msg);
        return -1;
    }

    // 가변 길이 데이터 수신 (메시지 내용)
    if (recv_variable_length_data(sock, formatted_msg, len, max_retries) == -1) {
        free(formatted_msg);
        return -1;
    }

    // 메시지 파싱
    parse_message(formatted_msg, msg);

    // 메모리 해제
    free(formatted_msg);

    return 0;
}


void *ChatThread(void *arg)
{

	int retval;  // 소켓 오류시 시도 횟수

	ClientInfo *client_info = (ClientInfo *)arg;
	if (client_info == NULL) {
		err_quit("client_info Memory allocation error");
	}
	Message *msg = (Message *)malloc(CHAT_SIZE);
	if (msg == NULL) {
		err_quit("msg Memory allocation error");
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

			memset(msg, 0, CHAT_SIZE);

			if(recv_chat_message(client_info->sock, msg,5) < 0)
				err_display_msg("recv_chat_message()");

			printf("Received message:\nCommand: %s\nUser: %s\nMessage: %s\n", msg->command, msg->user, msg->message);

			// "q"가 입력되면 소켓 종료
			if (!strcmp(msg->message, "q")) {
				printf("q가 입력되어서 종료합니다!\n");
				break;
			}
	
		}

		// 표준 입력(키보드 입력), 서버로 데이터 전송
		if (FD_ISSET(STDIN_FILENO, &readfds)) {
			if(send_chat_message(client_info->sock, "all", 5)<0)	// 보낼 소켓, 보낼 사람
				err_display_msg("send_chat_message()");
		}
	}


	printf("ChatThread 종료\n");
	return 0;
}


void *VideoStreamThread(void *arg)
{}


