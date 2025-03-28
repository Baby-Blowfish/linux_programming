#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#define TCP_PORT 5100
#define MAX_CLIENTS 8

// TCP 소켓 설정
extern int ssock;              // 서버 소켓 디스크립터
extern int client_count;    // 현재 접속된 클라이언트 수

// 버퍼 설정
char mesg[BUFSIZ];            // 메시지 버퍼, 클라이언트에서 송수신되는 데이터 저장용

// 클라이언트 정보 구조체 정의
typedef struct {
    int csock;                      // 클라이언트 소켓 디스크립터 (클라이언트와 서버가 통신하는 소켓)
    pid_t pid;                      // 자식 프로세스 ID (클라이언트 연결을 처리하는 자식 프로세스 ID)
    int client_id;                  // 클라이언트 고유 ID (서버에서 구분하기 위한 ID)
    int child_to_parent[2];         // 자식 -> 부모로 데이터를 전송하는 파이프
    int parent_to_child[2];         // 부모 -> 자식으로 데이터를 전송하는 파이프
    char mesg[BUFSIZ];              // 클라이언트 메시지 버퍼 (클라이언트에서 수신된 데이터를 저장)
} __attribute__((packed, aligned(4))) Client;  // 4바이트로 정렬

extern Client* clients[MAX_CLIENTS];  // 동적으로 클라이언트를 관리하기 위한 포인터 배열

// 클라이언트 제거 함수 선언
void remove_client(int index); // 클라이언트가 종료될 때 클라이언트를 목록에서 제거하는 함수
// Signal 핸들러 선언
void sigHandlerParent(int signo);    // 서버 측 signal 핸들러 (부모 프로세스가 처리)
void sigHandlerChild(int signo);     // 클라이언트 측 signal 핸들러 (자식 프로세스가 처리)

#endif
