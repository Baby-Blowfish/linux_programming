#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>     // sockaddr_in, htons, INADDR_ANY
#include <sys/socket.h>     // socket, bind, recvfrom, sendto

#define UDP_PORT 5100       // 서버가 사용할 포트 번호

int main(int argc, char **argv)
{
    int sockfd, n;
    struct sockaddr_in servaddr, cliaddr;  // 서버/클라이언트 주소 구조체
    socklen_t len;
    char mesg[1000];

    // 1. UDP 소켓 생성 (SOCK_DGRAM은 UDP 소켓을 의미함)
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // 2. 서버 주소 구조체 초기화 및 설정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;              // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 모든 IP 주소로부터 수신 허용
    servaddr.sin_port = htons(UDP_PORT);        // 포트 번호 지정 (네트워크 바이트 순서로 변환)

    // 3. 소켓에 주소 바인딩
    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    // 4. 데이터 수신 및 전송 루프
    do
    {
        len = sizeof(cliaddr); // 클라이언트 주소 길이
        // 클라이언트로부터 데이터 수신
        n = recvfrom(sockfd, mesg, 1000, 0, (struct sockaddr*)&cliaddr, &len);

        // 받은 메시지를 다시 클라이언트로 전송 (echo)
        sendto(sockfd, mesg, n, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));

        mesg[n] = '\0'; // 문자열 끝 처리
        printf("Received data : %s\n", mesg); // 메시지 출력
    } while(strncmp(mesg, "q", 1)); // 입력이 "q"로 시작하면 종료

    // 5. 소켓 닫기
    close(sockfd);
    return 0;
}

