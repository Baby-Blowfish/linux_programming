#include "tcp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int clnt_cnt = 0;  // 현재 접속한 클라이언트 수
int clnt_socks[MAX_CLNT];  // 클라이언트 소켓 파일 디스크립터 배열
pthread_mutex_t mutx;  // 클라이언트 목록 접근을 위한 뮤텍스

// 서버 소켓 초기화 함수
// 지정된 포트로 서버 소켓을 생성하고 바인딩하며, 클라이언트의 접속을 대기하도록 설정
// 성공 시 서버 소켓 파일 디스크립터를 반환, 실패 시 -1 반환
int init_server(int port) {
    int serv_sock;
    struct sockaddr_in serv_adr;

    // 뮤텍스 초기화
    pthread_mutex_init(&mutx, NULL);
    
    // 소켓 생성
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    if (serv_sock == -1) {
        error_handling("socket() error");  // 소켓 생성 오류 시 에러 메시지 출력
        return -1;
    }

    // 주소 구조체 초기화 및 설정
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);  // 모든 IP 주소로부터의 접속 허용
    serv_adr.sin_port = htons(port);  // 지정된 포트로 설정

    // 소켓과 주소 바인딩
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1) {
        error_handling("bind() error");  // 바인딩 오류 시 에러 메시지 출력
        return -1;
    }

    // 클라이언트 접속 대기 상태로 변경
    if (listen(serv_sock, 5) == -1) {
        error_handling("listen() error");  // listen 오류 시 에러 메시지 출력
        return -1;
    }

    return serv_sock;  // 서버 소켓 파일 디스크립터 반환
}



// 에러 발생 시 메시지를 출력하고 프로그램 종료
void error_handling(char *msg) {
    fputs(msg, stderr);  // 에러 메시지 출력
    fputc('\n', stderr);
    exit(1);  // 프로그램 종료
}
