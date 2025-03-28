#ifndef __COMMON_H__  // 헤더 파일 중복 포함을 방지하기 위한 매크로 정의 시작
#define __COMMON_H__

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
#include <malloc.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>              /* videodev2.h에서 필요한 데이터 타입 정의 */
#include <linux/videodev2.h>        /* V4L2를 제어하기 위한 헤더 */
#include <linux/fb.h>

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

#define WIDTH        640             /* 캡처할 영상의 너비 */
#define HEIGHT       360             /* 캡처할 영상의 높이 */

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



void mesg_exit(const char *s);
int xioctl(int fd, int request, void *arg);
void err_quit(const char *msg);
void err_display_msg(const char *msg);
void err_display_code(int errcode);
void format_message(const Message *msg, char *formatted_message);
void parse_message(char *data, Message *msg);
int send_fixed_length_data(SOCKET sock	, uint32_t data, int max_retries) ;	// 고정 길이 데이터 송신 
int send_variable_length_data(SOCKET sock, const char *buf, int len, int max_retries);	// 가변 길이 데이터 송신 
int recv_fixed_length_data(SOCKET sock, uint32_t *data, int max_retries);	// 고정 길이 데이터 수신 
int recv_variable_length_data(SOCKET sock, char *buf, int len, int max_retries); // 가변 길이 데이터 수신 함수
int send_chat_message(SOCKET sock, const char *username, const char * message, int max_retries);
int recv_chat_message(SOCKET sock, Message *msg, int max_retries);

#endif  // 헤더 파일 중복 포함 방지를 위한 매크로 정의 끝