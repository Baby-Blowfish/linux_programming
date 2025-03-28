#include <stdio.h>            // for perror
#include <stdlib.h>           // for malloc, free
#include <string.h>
#include <fcntl.h>            // for open
#include <unistd.h>           // for read, write, close
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT            554   // 표준 RTSP 포트
#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENTS     10

// RTSP 요청 메서드 열거형
typedef enum {
  RTSP_OPTIONS,
  RTSP_DESCRIBE,
  RTSP_SETUP,
  RTSP_PLAY,
  RTSP_PAUSE,
  RTSP_TEARDOWN,
  RTSP_INVALID
} RTSPMethod;

// RTSP 세션 구조체
typedef struct {
  int client_socket;
  int cseq;
  char session_id[32];
  int is_playing;
} RTSPSession;

// RTSP 메서드 파싱 함수
RTSPMethod parse_rtsp_method(const char* request) 
{
  if (strncmp(request, "OPTIONS", 7) == 0) return RTSP_OPTIONS;
  if (strncmp(request, "CSeq", 4) == 0) return RTSP_OPTIONS;
  if (strncmp(request, "DESCRIBE", 8) == 0) return RTSP_DESCRIBE;
  if (strncmp(request, "SETUP", 5) == 0) return RTSP_SETUP;
  if (strncmp(request, "PLAY", 4) == 0) return RTSP_PLAY;
  if (strncmp(request, "PAUSE", 5) == 0) return RTSP_PAUSE;
  if (strncmp(request, "TEARDOWN", 8) == 0) return RTSP_TEARDOWN;
  return RTSP_INVALID;
}

// CSeq 파싱 함수
int parse_cseq(const char* request) 
{
  char* cseq_ptr = strstr(request, "CSeq:");
  if (cseq_ptr) {
    return atoi(cseq_ptr + 5);
  }
  return -1;
}

// RTSP 응답 생성 함수
int create_rtsp_response(RTSPMethod method, int cseq, char* response, RTSPSession* session) 
{
  off_t file_size;

  switch (method) {
    case RTSP_OPTIONS:
      snprintf(response, MAX_BUFFER_SIZE, 
          "RTSP/1.0 200 OK\r\n"
          "CSeq: %d\r\n"
          "Public: OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, TEARDOWN\r\n\r\n", 
          cseq);
      break;

    case RTSP_DESCRIBE:
      snprintf(response, MAX_BUFFER_SIZE, 
          "RTSP/1.0 200 OK\r\n"
          "CSeq: %d\r\n"
          "Content-Type: application/sdp\r\n"
          "Content-Length: %ld\r\n\r\n", 
          cseq, file_size);
      break;

    case RTSP_SETUP:
      snprintf(session->session_id, sizeof(session->session_id), "session_%d", getpid());
      snprintf(response, MAX_BUFFER_SIZE, 
          "RTSP/1.0 200 OK\r\n"
          "CSeq: %d\r\n"
          "Session: %s\r\n"
          "Transport: RTP/UDP;unicast;client_port=8000-8001;server_port=9000-9001\r\n\r\n", 
          cseq, session->session_id);
      break;

    case RTSP_PLAY:
      session->is_playing = 1;
      snprintf(response, MAX_BUFFER_SIZE, 
          "RTSP/1.0 200 OK\r\n"
          "CSeq: %d\r\n"
          "Session: %s\r\n"
          "Range: npt=0.000-\r\n\r\n", 
          cseq, session->session_id);
      break;

    case RTSP_PAUSE:
      session->is_playing = 0;
      snprintf(response, MAX_BUFFER_SIZE, 
          "RTSP/1.0 200 OK\r\n"
          "CSeq: %d\r\n"
          "Session: %s\r\n\r\n", 
          cseq, session->session_id);
      break;

    case RTSP_TEARDOWN:
      snprintf(response, MAX_BUFFER_SIZE, 
          "RTSP/1.0 200 OK\r\n"
          "CSeq: %d\r\n"
          "Session: %s\r\n\r\n", 
          cseq, session->session_id);
      break;

    default:
      snprintf(response, MAX_BUFFER_SIZE, 
          "RTSP/1.0 400 Bad Request\r\n"
          "CSeq: %d\r\n\r\n", 
          cseq);
  }
  return 0;
}

/* 스레드 처리를 위한 함수 */
static void *clnt_connection(void *arg)
{
  /* 스레드를 통해서 넘어온 arg를 int 형의 파일 디스크립터로 변환한다. */
  int client_socket = *((int*)arg);
  FILE *clnt_read, *clnt_write;
  char reg_line[BUFSIZ], reg_buf[BUFSIZ];
  char method[BUFSIZ], type[BUFSIZ];
  char filename[BUFSIZ], *ret;
  char buffer[MAX_BUFFER_SIZE];

  /* 파일 디스크립터를 FILE 스트림으로 변환한다. */
  clnt_read = fdopen(client_socket, "r");
  clnt_write = fdopen(dup(client_socket), "w");

  while(1) {
    // 클라이언트 요청 수신
    int ret;
    memset(buffer, 0, sizeof(buffer));
    if ((ret = recv(client_socket, buffer, sizeof(buffer), 0)) < 0) {
      perror("수신 실패");
      close(client_socket);
      continue;
    } else if(ret == 0) {
      continue;
    }

    // RTSP 세션 초기화
    RTSPSession session = {
      .client_socket = client_socket,
      .is_playing = 0
    };

    // 메서드 파싱
    RTSPMethod method = parse_rtsp_method(buffer);
    int cseq = parse_cseq(buffer);

    // 응답 생성 및 전송
    char response[MAX_BUFFER_SIZE];
    create_rtsp_response(method, cseq, response, &session);

    send(client_socket, response, strlen(response), 0);

    // 디버깅용 출력
    printf("수신된 RTSP 요청:\n%s\n", buffer);
    printf("전송된 RTSP 응답:\n%s\n", response);

    if(method == RTSP_TEARDOWN) break;
  }

  // 클라이언트 소켓 종료
  close(client_socket);
}

int main(int argc, char **argv) 
{
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  pthread_t thread;

  // 소켓 생성
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    perror("소켓 생성 실패");
    exit(1);
  }

  // 서버 주소 설정
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // 소켓 바인딩
  if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("바인딩 실패");
    exit(1);
  }

  // 수신 대기
  if (listen(server_socket, MAX_CLIENTS) < 0) {
    perror("수신 대기 실패");
    exit(1);
  }

  printf("RTSP 서버가 %d 포트에서 대기 중...\n", PORT);

  while (1) {
    char mesg[BUFSIZ];
    // 클라이언트 연결 수락
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_socket < 0) {
      perror("연결 수락 실패");
      continue;
    }

    /* 네트워크 주소를 문자열로 변경 */
    inet_ntop(AF_INET, &client_addr.sin_addr, mesg, BUFSIZ);
    printf("Client IP : %s:%d\n", mesg, ntohs(client_addr.sin_port));

    /* 클라이언트의 요청이 들어오면 스레드를 생성하고 클라이언트의 요청을 처리한다. */
    pthread_create(&thread, NULL, clnt_connection, (void*)&client_socket);
    pthread_detach(thread);
  }

  close(server_socket);
  return 0;
}