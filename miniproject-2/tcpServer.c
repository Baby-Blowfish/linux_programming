#include <stdio.h>      // 표준 입출력 함수 사용 (printf 등)
#include <string.h>     // 문자열 처리 함수 사용 (memset, strncmp 등)
#include <unistd.h>     // 유닉스 표준 함수 사용 (close 등)
#include <sys/socket.h> // 소켓 함수 사용 (socket, bind, listen, accept 등)
#include <arpa/inet.h>  // 인터넷 프로토콜 관련 함수 사용 (inet_ntop 등)

#define TCP_PORT 5100   // 서버가 사용할 TCP 포트 번호

int main(int argc, char **argv) {


    // Socket setting
    int ssock;          // 서버 소켓 디스크립터
    socklen_t clen;      // 클라이언트 주소 구조체 크기
    struct sockaddr_in servaddr, cliaddr;  // 서버 및 클라이언트 주소 구조체

    // Buffer setting
    char mesg[BUFSIZ];   // 메시지 버퍼

    // 1. 서버 소켓 생성 (AF_INET: IPv4, SOCK_STREAM: TCP, 0: 프로토콜 자동 선택)
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() :");    // 소켓 생성 실패 시 오류 출력
        return -1;
    }

    // 2. 서버 주소 구조체 설정
    memset(&servaddr, 0, sizeof(servaddr)); // 구조체를 0으로 초기화
    servaddr.sin_family = AF_INET;          // IPv4 주소 체계 설정
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 모든 IP 주소에서 연결 허용
    servaddr.sin_port = htons(TCP_PORT);    // 포트 번호 설정 (호스트 -> 네트워크 바이트 순서)

    // 3. 서버 소켓을 지정한 주소와 포트에 바인딩
    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind() :");       // 바인딩 실패 시 오류 출력
        return -1;
    }

    // 4. 연결 대기 큐 설정 (최대 8개의 클라이언트 대기 가능)
    if (listen(ssock, 8) < 0) {
        perror("listen() : ");    // 대기 상태 설정 실패 시 오류 출력
        return -1;
    }

    clen = sizeof(cliaddr); // 클라이언트 주소 구조체의 크기를 초기화

    // 5. 클라이언트 연결 수락 (accept)
    int n, csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
    if (csock < 0) {
        perror("accept() :"); // 연결 수락 실패 시 오류 출력
        //continue; // 에러 발생 시 다음 클라이언트 연결 처리로 진행
    }

    // 6. 클라이언트의 IP 주소를 문자열 형식으로 변환하여 출력
    inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
    printf("Client is connected: %s\n", mesg);

    do {

        memset(mesg, 0, BUFSIZ); // 메시지 버퍼 초기화
        // 7. 클라이언트로부터 메시지 수신 (read)
        if ((n = read(csock, mesg, BUFSIZ)) < 0) {
            perror("read() :");   // 메시지 수신 실패 시 오류 출력
            break;
        }

        // 8. 수신한 메시지를 출력 (null 문자로 끝 처리)
        mesg[n] = '\0';           // 수신한 메시지의 끝에 null 문자 추가
        printf("Received data: %s", mesg);

        // 9. 클라이언트로 메시지 에코 (받은 메시지를 다시 클라이언트로 전송)
        if (write(csock, mesg, n) <= 0) {
            perror("write()");    // 메시지 전송 실패 시 오류 출력
        }

    } while (strncmp(mesg, "q", 1) != 0); // 메시지가 'q'로 시작하면 서버 종료

    // 10. 클라이언트와의 연결 종료
    close(csock);             // 클라이언트 소켓 종료
    // 11. 서버 소켓 종료
    close(ssock);                 // 서버 소켓 종료
    return 0;
}
