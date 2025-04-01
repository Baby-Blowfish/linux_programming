#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TCP_PORT 5100

int main(int argc, char**argv)
{
    int ssock, n;
    struct sockaddr_in servaddr;
    char mesg[BUFSIZ];

    // 1. IP 주소 인자 확인
    if(argc<2)
    {
        printf("Usage : %s IP_ADDRESS\n", argv[0]);
        return -1;
    }

    // 2. 소켓 생성
    if((ssock = socket(AF_INET, SOCK_STREAM, 0))<0)
    {
        perror("socket() :");
        return -1;
    }

    // 3. 서버 주소 구조체 설정
    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(servaddr.sin_addr.s_addr));
    servaddr.sin_port = htons(TCP_PORT);

    // 4. 서버에 연결 요청
    if(connect(ssock,(struct sockaddr*)&servaddr, sizeof(servaddr))<0)
    {
        perror("connect() :");
        return -1;
    }

    // 5. 사용자 입력
    printf("[클라이언트] 서버에 보낼 메시지를 입력하세요: ");
    fgets(mesg, BUFSIZ, stdin);

    // 6. 서버에 메시지 전송
    if(send(ssock, mesg, BUFSIZ, MSG_DONTWAIT) <= 0)  // 논블로킹 전송
    {
        perror("send() :");
        return -1;
    }

    // 송신 종료 알림
    shutdown(ssock, SHUT_WR);

    // 7. 서버로부터 에코 메시지 수신
    memset(mesg, 0, BUFSIZ);
    while((n = recv(ssock, mesg, BUFSIZ, 0)) > 0)
    {
        mesg[n] = '\0';
        printf("서버 응답: %s", mesg);
    }


    if(n == 0)
        printf("서버가 연결 종료했음\n");

    shutdown(ssock, SHUT_RDWR);
    // 8. 소켓 종료
    close(ssock);

    return 0;
}

