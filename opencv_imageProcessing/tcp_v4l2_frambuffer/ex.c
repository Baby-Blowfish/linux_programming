#include "tcp_server.h"
#include "video_capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

int main(int argc, char *argv[]) {
	
    int serv_sock, clnt_sock;  // 서버 소켓과 클라이언트 소켓 파일 디스크립터
    struct sockaddr_in clnt_adr;  // 클라이언트 주소 구조체
    socklen_t clnt_adr_sz;  // 클라이언트 주소 크기
    pthread_t t_id;  // 스레드 ID

    // 인자가 2개 (프로그램 이름과 포트 번호)가 아닌 경우 사용법 출력 후 종료
    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 서버 소켓 초기화 및 생성
    serv_sock = init_server(atoi(argv[1]));  // argv[1]로 전달된 포트 번호로 서버 초기화

    while (1) {
        clnt_adr_sz = sizeof(clnt_adr);  // 클라이언트 주소 구조체의 크기를 설정
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);  // 클라이언트 연결 대기 및 수락

        // 클라이언트 소켓을 배열에 추가하기 위해 뮤텍스를 사용하여 상호 배제 처리
        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;  // 클라이언트 소켓을 소켓 배열에 저장하고 클라이언트 수 증가
        pthread_mutex_unlock(&mutx);

        // 클라이언트 처리를 위한 스레드 생성
        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);  // 새 스레드에서 클라이언트 처리 함수 실행
        pthread_detach(t_id);  // 생성된 스레드를 분리 (스레드 종료 시 자동으로 자원 해제)
        
        // 연결된 클라이언트의 IP 주소 출력
        printf("Connected client IP: %s\n", inet_ntoa(clnt_adr.sin_addr));
    }
    
    // 서버 소켓 닫기 (프로그램이 종료될 때)
    close(serv_sock);
    return 0;
}
