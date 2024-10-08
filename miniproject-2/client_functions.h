#ifndef CLIENT_FUNCTIONS_H
#define CLIENT_FUNCTIONS_H

#include <stdio.h>      // 표준 입출력 함수 사용
#include <string.h>     // 문자열 처리 함수 사용 (memset, strncmp 등)
#include <unistd.h>     // 유닉스 표준 함수 사용 (fork, pipe, close 등)
#include <stdlib.h>     // 표준 라이브러리 함수 사용 (exit 등)
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // signal()
#include <sys/socket.h> // 소켓 함수 사용 (socket, connect, send, recv 등)
#include <arpa/inet.h>  // 인터넷 프로토콜 관련 함수 (inet_pton 등)



// Signal setting
void sigHandler(int signo);

// Pipe setting
int child_to_parent[2];         // 파이프를 전역으로 선언

// Buffer setting
char buffer[BUFSIZ];            // 전역으로 버퍼 선언

// TCP Socket setting
#define TCP_PORT 5100   // 서버와 연결할 포트 번호
int ssock;       // 소켓 디스크립터


#endif // CLIENT_FUNCTIONS_H
