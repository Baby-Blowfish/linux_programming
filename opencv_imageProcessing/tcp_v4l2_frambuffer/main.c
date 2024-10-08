#include "tcp_server.h"
#include "video_capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>


// Function to handle communication with a connected client
// This function will be run as a separate thread for each client
void *handle_clnt(void *arg);

// Function to send a message to all connected clients
// Takes the message and its length as arguments
void send_msg(char *msg, int len);


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
    
    
    
    
    int fbfd = -1;              /* 프레임버퍼의 파일 디스크립터 */
    
    /* 프레임버퍼 열기 */
    fbfd = open(FBDEV, O_RDWR);
    if(-1 == fbfd) {
        perror("open( ) : framebuffer device");
        return EXIT_FAILURE;
    }

    /* 프레임버퍼의 정보 가져오기 */
    if(-1 == ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information.");
        return EXIT_FAILURE;
    }

    /* mmap( ) : 프레임버퍼를 위한 메모리 공간 확보 */
    long screensize = vinfo.xres * vinfo.yres * 2;
    fbp = (short *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if(fbp == (short*)-1) {
        perror("mmap() : framebuffer device to memory");
        return EXIT_FAILURE;
    }
    
    memset(fbp, 0, screensize);
    
    /* 카메라 장치 열기 */
    camfd = open(VIDEODEV, O_RDWR | O_NONBLOCK, 0);
    if(-1 == camfd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         VIDEODEV, errno, strerror(errno));
        return EXIT_FAILURE;
    }
    
    
    init_device(camfd);

    start_capturing(camfd);
    
    mainloop(camfd);
    
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


// 클라이언트 연결을 처리하는 함수
// 별도의 스레드에서 실행되며, 클라이언트의 메시지를 수신하고 이를 모든 클라이언트에게 전달
void *handle_clnt(void *arg) {
    int clnt_sock = *((int *)arg);  // 클라이언트 소켓 파일 디스크립터
    int str_len = 0, i;
    char msg[BUF_SIZE];

    // 클라이언트로부터 메시지를 반복해서 읽어들임
    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0) {
        send_msg(msg, str_len);  // 받은 메시지를 모든 클라이언트에게 전송
    }

    // 클라이언트 연결이 끊겼을 때
    pthread_mutex_lock(&mutx);  // 클라이언트 리스트 접근을 위한 뮤텍스 잠금
    for (i = 0; i < clnt_cnt; i++) {  // 연결이 끊긴 클라이언트 제거
        if (clnt_sock == clnt_socks[i]) {
            while (i < clnt_cnt - 1) {
                clnt_socks[i] = clnt_socks[i + 1];
                i++;
            }
            break;
        }
    }
    clnt_cnt--;  // 클라이언트 수 감소
    pthread_mutex_unlock(&mutx);  // 뮤텍스 해제
    close(clnt_sock);  // 클라이언트 소켓 닫기
    return NULL;
}

// 모든 클라이언트에게 메시지를 전송하는 함수
void send_msg(char *msg, int len) {  // 모든 클라이언트에게 메시지 전송
    int i;
    pthread_mutex_lock(&mutx);  // 클라이언트 리스트 접근을 위한 뮤텍스 잠금
    for (i = 0; i < clnt_cnt; i++) {
        write(clnt_socks[i], msg, len);  // 각 클라이언트에 메시지 전송
    }
    pthread_mutex_unlock(&mutx);  // 뮤텍스 해제
}
