#include <stdio.h>      // 기본 입출력 함수 사용 (printf, fgets 등)
#include <unistd.h>     // UNIX 표준 함수 사용 (close 등)
#include <string.h>     // 문자열 처리 함수 사용 (memset, strlen 등)

#include <sys/socket.h> // 소켓 프로그래밍을 위한 라이브러리
#include <arpa/inet.h>  // 인터넷 프로토콜 주소 변환 함수 (inet_pton 등)

#define TCP_PORT 5100   // 서버와 통신할 TCP 포트 번호 정의

int main(int argc, char** argv)
{
    int ssock;                         // 클라이언트 소켓 디스크립터
    struct sockaddr_in servaddr;       // 서버 주소 정보를 담는 구조체
    char mesg[BUFSIZ];                 // 메시지 버퍼 (클라이언트 -> 서버)

    // 프로그램 실행 시 IP 주소가 입력되었는지 확인
    if (argc < 2)
    {
        printf("Usage : %s IP_ADDRESS\n", argv[0]); // 사용법 출력
        return -1;
    }

    // 소켓 생성 (AF_INET: IPv4, SOCK_STREAM: TCP 통신, 0: 프로토콜 자동 선택)
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() :");          // 소켓 생성 실패 시 오류 출력
        return -1;
    }

    // 서버 주소 구조체 초기화
    memset(&servaddr, 0, sizeof(servaddr)); // 구조체의 모든 필드를 0으로 초기화
    servaddr.sin_family = AF_INET;          // IPv4 주소 체계 설정

    // 사용자가 입력한 IP 주소를 네트워크 바이트 순서로 변환하여 구조체에 설정
    inet_pton(AF_INET, argv[1], &(servaddr.sin_addr.s_addr));
    servaddr.sin_port = htons(TCP_PORT);    // 포트 번호 설정 (호스트 바이트 순서를 네트워크 순서로 변환)

    // 서버에 연결 시도
    if (connect(ssock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect() :");         // 연결 실패 시 오류 출력
        return -1;
    }

    // 표준 입력으로부터 메시지 입력 받기 (사용자가 입력한 메시지)
    fgets(mesg, BUFSIZ, stdin);

    // 입력받은 메시지를 서버에 전송 (비차단 모드로 전송)
    if (send(ssock, mesg, BUFSIZ, MSG_DONTWAIT) <= 0)
    {
        perror("send() :");            // 메시지 전송 실패 시 오류 출력
        return -1;
    }

    // 서버로부터 응답을 받을 때 사용할 메시지 버퍼 초기화
    memset(mesg, 0, BUFSIZ);

    // 서버로부터 응답을 수신
    if (recv(ssock, mesg, BUFSIZ, 0) <= 0)
    {
        perror("recv() :");            // 응답 수신 실패 시 오류 출력
        return -1;
    }

    // 수신한 데이터를 출력
    printf("Received data : %s ", mesg);

    // 통신 종료 후 소켓 닫기
    close(ssock);

    return 0;
}
