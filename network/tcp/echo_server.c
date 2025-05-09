#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TCP_PORT 5100

int main(int argc, char**argv)
{
    int ssock;                         // 서버 소켓 디스크립터
    socklen_t clen;
    struct sockaddr_in servaddr, cliaddr; // 서버, 클라이언트 주소 구조체
    char mesg[BUFSIZ];                // 데이터 송수신 버퍼

    // 1. TCP 소켓 생성
    if((ssock = socket(AF_INET,SOCK_STREAM,0))<0)
    {
        perror("socket() :");
        return -1;
    }

    // 2. 포트 재사용 설정
    int optval = 1;
    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // 2. 서버 주소 구조체 초기화 및 설정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;                 // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 모든 IP에서의 접속 허용
    servaddr.sin_port = htons(TCP_PORT);           // 포트 번호

    // 3. IP/포트 바인딩
    if(bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr))<0)
    {
        perror("bind() :");
        return -1;
    }

    // 4. 클라이언트 연결 대기 상태 진입
    if((listen(ssock, 8) < 0))
    {
        perror("listen() : ");
        return -1;
    }

    clen = sizeof(cliaddr);

    printf("[서버] 연결 대기 중...\n");

    do
    {
        // 5. 클라이언트 연결 수락
        int n, csock=accept(ssock, (struct sockaddr *)&cliaddr, &clen);
        if(csock < 0)
        {
          perror("accept()");
          return -1;
        }

        // 6. 클라이언트 IP 주소 출력
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
        printf("Client is connected: %s\n",mesg);

        // 7. 클라이언트로부터 데이터 수신
        while((n=read(csock, mesg, BUFSIZ)) > 0)
        {
            mesg[n] = '\0';
            printf("Received data : %s", mesg);

            // 8. 받은 데이터를 다시 전송 (에코)
            send(csock,  mesg, n, 0);
        }

        if( n == 0)
            printf("클라이언트 종료\n");
        else
          perror("recv()");

        // 수신도 끝났으면 완전히 종료
        shutdown(csock, SHUT_RDWR);
        close(csock);

    }while(strncmp(mesg,"q",1));  // "q" 입력 시 종료

    printf("  [서버] 정상 종료 \n");

    close(ssock); // 서버 소켓 종료
    return 0;
}

