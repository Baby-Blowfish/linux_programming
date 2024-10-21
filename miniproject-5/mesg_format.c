#include <stdio.h>
#include <string.h>
#include <stdlib.h>  // atoi 포함

#define BUFFER_CHUNK_SIZE 1024
#define CHAT_SIZE 256
#define COMMAND_SIZE 10
#define NAME_SIZE 20
#define HEADER_SIZE 34
#define MESSAGE_SIZE (CHAT_SIZE - HEADER_SIZE)

// 메시지를 저장할 구조체
typedef struct {
    char command[COMMAND_SIZE];   // 명령어 (예: "chat", "q")
    char user[NAME_SIZE];         // 사용자 이름
    int message_length;           // 메시지 길이
    char message[MESSAGE_SIZE];   // 실제 메시지 내용
} Message;

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

int main() {
    char formatted_message[BUFFER_CHUNK_SIZE];

    // 예시 데이터
    Message msg = {"chat", "user1", 14, "Hello everyone"};

    // 메시지를 포맷
    format_message(&msg, formatted_message);

    // 포맷된 메시지 출력
    printf("Formatted Message: %s\n", formatted_message);

    // 새로운 메시지 구조체로 파싱 테스트
    Message parsed_msg;
    parse_message(formatted_message, &parsed_msg);

    // 파싱된 결과 출력
    printf("Command: %s\n", parsed_msg.command);
    printf("User: %s\n", parsed_msg.user);
    printf("Message Length: %d\n", parsed_msg.message_length);
    printf("Message: %s\n", parsed_msg.message);

    return 0;
}
