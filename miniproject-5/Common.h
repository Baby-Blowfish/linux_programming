


#include <sys/types.h> // basic type definitions
#include <sys/socket.h> // socket(), AF_INET, ...
#include <arpa/inet.h> // htons(), htonl(), ...
#include <netdb.h> // gethostbyname(), ...
#include <unistd.h> // close(), ...
#include <fcntl.h> // fcntl(), ...
#include <pthread.h> // pthread_create(), ...
#include <poll.h> // poll()
#include <sys/epoll.h> // epoll()
#include <sys/select.h>

#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strerror(), ...
#include <stdbool.h> // bool variable
#include <errno.h> // errno
#include <ctype.h> // isspace()..
#include <signal.h>	// signal()...

// Windows 소켓 코드와 호환성을 위한 정의
typedef int SOCKET;
#define SOCKET_ERROR   -1
#define INVALID_SOCKET -1

#define SERVERPORT 9000
#define BUFFER_CHUNK_SIZE 1024  // 청크 데이터 크기
#define CHAT_SIZE  256
#define COMMAND_SIZE 10
#define NAME_SIZE  20
#define HEADER_SIZE (COMMAND_SIZE + NAME_SIZE + sizeof(uint32_t))
#define MESSAGE_SIZE (CHAT_SIZE - HEADER_SIZE)

// 메시지를 저장할 구조체
typedef struct {
    char command[COMMAND_SIZE];   // 명령어 (예: "chat", "q")
    char user[NAME_SIZE];         // 사용자 이름
    uint32_t message_length;      // 메시지 길이 (uint32_t로 정의)
    char message[MESSAGE_SIZE];   // 실제 메시지 내용
} Message;

// 클라이언트 정보를 저장할 구조체
typedef struct {
	SOCKET sock;
	char name[NAME_SIZE];
} ClientInfo;

char *SERVERIP = (char *)"127.0.0.1";

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
