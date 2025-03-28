#include "Common.h"


// 소켓 함수 오류 출력 후 종료
void err_quit(const char *msg)
{
	char *msgbuf = strerror(errno);
	printf("[%s] %s\n", msg, msgbuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display_msg(const char *msg)
{
	char *msgbuf = strerror(errno);
	printf("[%s] %s\n", msg, msgbuf);
}

// 소켓 함수 오류 출력
void err_display_code(int errcode)
{
    char *msgbuf = strerror(errcode);
    printf("[오류] %s\n", msgbuf);
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
        strncpy(msg->command, temp, strlen(temp));
        msg->command[strlen(temp) - 1] = '\0';
    }

    // 두 번째 필드: user
    temp = strtok(NULL, "|");
    if (temp != NULL) {
        // 앞뒤 공백 제거 및 값 복사
        while (*temp == ' ') temp++;
        strncpy(msg->user, temp, strlen(temp));
        msg->user[strlen(temp) - 1] = '\0';
    }

    // 세 번째 필드: message_length
    temp = strtok(NULL, "|");
    if (temp != NULL) {
        while (*temp == ' ') temp++;
        msg->message_length = atoi(temp);  // 문자열을 정수로 변환
    }

    // 네 번째 필드: message
    temp = strtok(NULL, "|");
    if (temp != NULL) {
         while (*temp == ' ') temp++;
        strncpy(msg->message, temp, strlen(temp));
        msg->message[strlen(temp)] = '\0';  // 널 문자 추가
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
			} else if( received == 0 ) {
				perror("recv() error");
				return -1;
			}
		}
        else if( received == 0 ) {
            printf("The client's connection was lost.\n");
            return -1;
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
        else if( received == 0 ) {
            printf("The client's connection was lost.\n");
            return -1;
    }
		total_received += received;
		retry_count = 0;  // 정상 수신되었으면 재시도 횟수 초기화
	}

	return 0;
}



// 채팅 메시지를 보내는 함수
int send_chat_message(SOCKET sock, const char *username, const char * message, int max_retries) {
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


    // Message 구조체에 값 할당
    strncpy(chat_buf->command, "chat", strlen("chat"));
    strncpy(chat_buf->user, username, strlen(username));
    strncpy(chat_buf->message, message, strlen(message));
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

