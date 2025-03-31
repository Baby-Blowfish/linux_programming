#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>          // inet_pton
#define UDP_PORT 5100

int main(int argc, char **argv)
{
    int sockfd, n;
    socklen_t clisize;
    struct sockaddr_in servaddr, cliaddr;
    char mesg[BUFSIZ]; // 시스템 정의 버퍼 크기

    // 1. IP 주소 인자가 없으면 사용법 출력
    if (argc != 2) {
        printf("usage : %s <IP address>\n", argv[0]);
        return -1;
    }

    // 2. UDP 소켓 생성
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // 3. 서버 주소 설정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;               // IPv4
    inet_pton(AF_INET, argv[1], &(servaddr.sin_addr.s_addr)); // 문자열 IP → 네트워크 주소 변환
    servaddr.sin_port = htons(UDP_PORT);         // 포트 번호 설정

    // 4. 데이터 송수신 루프
    do
    {
        // 표준 입력으로부터 문자열 읽기
        fgets(mesg, BUFSIZ, stdin);

        // 서버로 메시지 전송
        sendto(sockfd, mesg, strlen(mesg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

        clisize = sizeof(cliaddr); // 응답 받을 클라이언트 구조체 크기

        // 서버로부터 응답 수신
        n = recvfrom(sockfd, mesg, BUFSIZ, 0, (struct sockaddr*)&cliaddr, &clisize);

        mesg[n] = '\0';   // 문자열 끝 처리
        fputs(mesg, stdout); // 화면에 출력
    } while (strncmp(mesg, "q", 1)); // 입력이 "q"로 시작하면 종료

    // 5. 소켓 닫기
    close(sockfd);
    return 0;
}

