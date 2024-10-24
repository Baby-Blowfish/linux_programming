#include "Common.h"
#define SCREENSIZE  (1920*1080) 								// frame buffer의 크기
#define BUFFER_MAX  (CHAT_SIZE + SCREENSIZE)		// 받을 데이터의 최대 크기



const char* SERVERIP = "127.0.0.1";  // 서버 IP 주소

extern void err_quit(const char *msg);
extern void err_display_msg(const char *msg);
extern void err_display_code(int errcode);
extern void format_message(const Message *msg, char *formatted_message);
extern void parse_message(char *data, Message *msg);
extern int send_fixed_length_data(SOCKET sock	, uint32_t data, int max_retries) ;	// 고정 길이 데이터 송신 
extern int send_variable_length_data(SOCKET sock, const char *buf, int len, int max_retries);	// 가변 길이 데이터 송신 
extern int recv_fixed_length_data(SOCKET sock, uint32_t *data, int max_retries);	// 고정 길이 데이터 수신 
extern int recv_variable_length_data(SOCKET sock, char *buf, int len, int max_retries); // 가변 길이 데이터 수신 함수
extern int change_name(SOCKET sock, const char *new_name, const char *mmessage, int max_retries);extern int send_chat_message(SOCKET sock, const char *username, int max_retries);
int recv_chat_message(SOCKET sock, Message *msg, int max_retries);

// 메인 메뉴 함수
void show_main_menu();
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


