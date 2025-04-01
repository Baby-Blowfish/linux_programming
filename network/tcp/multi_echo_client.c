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
    char send_buf[BUFSIZ];
    char recv_buf[BUFSIZ];

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

    while (1) {
        printf("[클라이언트] 메시지 입력 (q 입력 시 종료): ");
        fgets(send_buf, BUFSIZ, stdin);

        send(ssock, send_buf, strlen(send_buf), 0);

        if (strncmp(send_buf, "q", 1) == 0)
            break;

        int n = recv(ssock, recv_buf, BUFSIZ - 1, 0);
        if (n <= 0) break;

        recv_buf[n] = '\0';
        printf("[클라이언트] 서버 응답: %s", recv_buf);
    }


    if(n == 0)
        printf("서버가 연결 종료했음\n");

    shutdown(ssock, SHUT_RDWR);
    // 8. 소켓 종료
    close(ssock);

    return 0;
}

