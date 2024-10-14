// tcpserver.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>                  /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/fb.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define FBDEV        "/dev/fb0"
#define WIDTH        800
#define HEIGHT       600

#define SERVER_PORT  12345  // 클라이언트와 동일한 포트 번호

static short *fbp = NULL; // short 16bit, 프레임버퍼 메모리 공간
static struct fb_var_screeninfo vinfo; // 가변 화면 정보를 저장할 구조체

static void init_framebuffer(); // 프레임버퍼 초기화 함수
static inline int clip(int value, int min, int max); // 상하한 설정 함수
static void process_image(const unsigned char *in); // YUV422 -> RGB565로 바꿔 프레임버퍼에 출력

int main(int argc, char **argv)
{
    int ssock, csock;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    const int frame_size = WIDTH * HEIGHT * 2; // YUYV 포맷: 2바이트 per 픽셀
    // https://blog.naver.com/wndrlf2003/220253497246
    // https://docs.kernel.org/userspace-api/media/v4l/pixfmt-packed-yuv.html
    // https://docs.kernel.org/userspace-api/media/v4l/yuv-formats.html#yuv-chroma-centered
    // https://en.wikipedia.org/wiki/YCbCr#4:2:2

    unsigned char *frame_buffer = malloc(frame_size); // 프레임버퍼 할당
    if(!frame_buffer) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    // 프레임버퍼 초기화
    init_framebuffer();

    // 소켓 초기화
    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if(ssock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 주소 구조체 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SERVER_PORT);

    // 포트 재사용 설정 - 빠르게 재사용하려면 설정
    int opt = 1;
    if(setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 바인드
    if(bind(ssock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // 리스닝
    if(listen(ssock, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is Waiting on PORT  %d...\n", SERVER_PORT);

    clilen = sizeof(cli_addr);
    csock = accept(ssock, (struct sockaddr *)&cli_addr, &clilen);
    if(csock < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("Client is Connected!: %s\n", inet_ntoa(cli_addr.sin_addr));

    // 영상 수신 및 출력
    while(1) {
        int total_received = 0; // 읽어온 바이트 수
        while(total_received < frame_size) { // 한 프레임을 전부 읽으면 탈출
            int received = recv(csock, frame_buffer + total_received, frame_size - total_received, 0);
            if(received < 0) {
                perror("recv");
                free(frame_buffer);
                close(csock);
                close(ssock);
                exit(EXIT_FAILURE);
            } else if(received == 0) {
                printf("Client Quit\n");
                free(frame_buffer);
                close(csock);
                close(ssock);
                exit(EXIT_SUCCESS);
            }
            total_received += received;
        }

        // 프레임버퍼에 출력
        process_image(frame_buffer);
    }

    // 정리
    free(frame_buffer);
    close(csock);
    close(ssock);

    return 0;
}

static void init_framebuffer() // 프레임버퍼 초기화
{
    int fbfd = open(FBDEV, O_RDWR); // 프레임버퍼 Open
    if(fbfd == -1) {
        perror("open framebuffer device");
        exit(EXIT_FAILURE);
    }

    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information.");
        exit(EXIT_FAILURE);
    }
    // https://docs.kernel.org/fb/api.html
    // 3 참조.
    // FBIOGET_VSCREENINFO : 가변 화면 정보
    // FBIOGET_FSCREENINFO : Fixed 화면 정보

    long screensize = vinfo.xres * vinfo.yres * 2; //RGB565, 16비트 컬러 == 1픽셀 당 2바이트

    fbp = (short*)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    // client의 mmap과 동일.
    // client와 다르게 framebuffer를 위한 mmap

    if(fbp == (short*)-1) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    memset(fbp, 0, screensize);

    close(fbfd);
}

static inline int clip(int value, int min, int max) // 상하한 세팅
{
    return(value > max ? max : value < min ? min : value);
}


/*
 * Keypoint
 * Remind : Client에서 YUYV(YUV422)로 비디오 프레임을 보냈음을 기억.
 * YUYV 포맷은 개념적 컨테이너로 2픽셀을 가로로, 1개로 그룹핑한다.
 * YUYV 포맷은 YUY2 라고도 불리며 포맷은 다음과 같다. : y0, u, y1, v.
 * 여기서 2픽셀이 나오게 된다.
 */
static void process_image(const unsigned char *in)
{
    int x, y, j;
    int y0, u, y1, v, r, g, b; // YUYV 성분, 변환할 RGB 성분
    unsigned short pixel; // RGB565 포맷에서의 한 픽셀의 값
    long location = 0; // 프레임버퍼 현 위치
    int istride = WIDTH * 2; // 한 프레임의 가로 한 줄의 데이터 길이, 16 bit per 1 pixel 이므로 X2

    for(y = 0; y < HEIGHT; ++y) { // 별 특별할 것 없는 column 루프
        for(j = 0, x = 0; j < vinfo.xres * 2; j += 4, x += 2) { // row 루프
            // vinfo.xres * 2는 무엇을 의미하나요?
            // 한 행의 총 바이트 수. 1 픽셀 당 16비트
            // YUYV 한 컨테이너는 2픽셀의 정보를 담은 4바이트 컨테이너
            // 즉 vinfo.xres*2와 직접적인 비교를 하는 j는 바이트를 의미하고,
            // x는 픽셀 수를 의미함을 알 수 있다.

			// 가변 프레임버퍼의 끝에 다다르지 않았지만, 내가 설정한 비디오 너비를 초과한 경우
            // 픽셀에 출력을 하지 않고, 다음 루프로 계속 점프
            if(j>=WIDTH*2) {
				location++;
				location++;
				continue;
			}

            // YUYV 성분을 분리한다.
            y0 = in[j] - 16;
			u = in[j + 1] - 128;
            y1 = in[j + 2] - 16;
            v = in[j + 3] - 128;
            // u와 v는 왜 128을 빼주나요?
            // https://en.wikipedia.org/wiki/Y%E2%80%B2U:wqV
            // https://en.wikipedia.org/wiki/Chrominance
            // https://en.wikipedia.org/wiki/Luma_(video)
            // https://en.wikipedia.org/wiki/Rec._601
            // https://en.wikipedia.org/wiki/Chroma_subsampling
            // 기본적으로 YUV 색상 체계는 1개의 luma와 2개의 색차로 이루어진 색상 체계다.
            // U = B′ − Y′ (blue − luma), V = R′ − Y′ (red − luma)
            // 색차는 위와 같이 정의되며 -127~128의 범위을 가진다.
            // luma는 밝기 차를 나타내며 16~235의 범위를 가진다.
            // 다만 색차는 디지털 데이터에서 unsigned 0~256으로 정의되며, 128을 neutral로 잡는다.
            // 즉 pixel에 띄워주려면 -128을 해줘야 본래 색 값을 알 수 있다.
            // 자세한 내용은 Rec.601의 Signal Format을 참조하라.


            // 이를 기반으로 0-255 스케일에서 YUV(YCrCb) -> RGB 변환식은 다음과 같다.
            // R = 1.164(Y - 16) + 1.596(Cr - 128)
            // G = 1.164(Y - 16) - 0.813(Cr - 128) - 0.392(Cb - 128)
            // B = 1.164(Y - 16) + 2.017(Cb - 128)
            // -16, -128 등의 연산은 위에서 처리했으니
            // 빠른 연산을 위해 계수에 256을 곱하고 8비트 밀어 위 변환식과 비슷하게 만들어준다.
            // https://sisoog.com/wp-content/uploads/2020/05/ycbcr2rgb.pdf
            // +128은 128/256==0.5 이므로 반올림의 효과를 내 절단 효과를 예방한다.

            // YUV를 RGB로 전환
            r = clip((298 * y0 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y0 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y0 + 516 * u + 128) >> 8, 0, 255);
            pixel = ((r>>3)<<11)|((g>>2)<<5)|(b>>3); // 24비트 컬러 -> 16비트 컬러
            fbp[location++] = pixel;

            // YUV를 RGB로 전환
            r = clip((298 * y1 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y1 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y1 + 516 * u + 128) >> 8, 0, 255);
            pixel = ((r>>3)<<11)|((g>>2)<<5)|(b>>3); // 24비트 컬러 -> 16비트 컬러
            fbp[location++] = pixel;

            // YUYV의 컨테이너에 기반해 한 번의 j루프에서 2개의 픽셀을 처리했다.
            // 2개의 픽셀을 처리했으므로 x는 +=2
            // 한 개의 4바이트 YUYV 컨테이너를 처리했으므로 j += 4
        }
        in += istride;
        // 한 개의 행이 끝났다면 다음 행의 데이터의 시작점으로 포인터를 이동
        // istride = 한 프레임의 가로 한 행의 데이터의 길이 
    }
}