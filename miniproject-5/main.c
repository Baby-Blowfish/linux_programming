
#include "v4l2.h"
#include "framebuffer.h"
#include "Common.h"

#define FBDEV        "/dev/fb0"      /* 프레임 버퍼 디바이스 파일 경로 */
#define VIDEODEV     "/dev/video0"   /* 비디오 캡처 디바이스 파일 경로 */

extern inline int clip(int value, int min, int max);
static void process_image(const void *p);
static int read_frame(int fd);
static void mainloop(int fd);

// framebuffer.h
extern int screensize_Frambuffer; 

/* 메인 함수 */
int main(int argc, char **argv)
{
    int fbfd = -1;     /* 프레임버퍼의 파일 디스크립터 */
    int camfd = -1;    /* 카메라의 파일 디스크립터 */

    /* 프레임버퍼 장치 열기 */
    fbfd = open(FBDEV, O_RDWR);
    if (-1 == fbfd) {
        perror("open() : framebuffer device");
        return EXIT_FAILURE;
    }

    init_frambuffer(fbfd);


    /* 카메라 장치 열기 */
    camfd = open(VIDEODEV, O_RDWR | O_NONBLOCK, 0);
    if (-1 == camfd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", VIDEODEV, errno, strerror(errno));
        return EXIT_FAILURE;
    }

    /* V4L2 장치 초기화 */
    init_v4l2(camfd);

    /* 비디오 캡처 시작 */
    start_capturing(camfd);

    /* 캡처 루프 */
    mainloop(camfd);

    /* 스트리밍 중지 */
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(camfd, VIDIOC_STREAMOFF, &type))
        mesg_exit("VIDIOC_STREAMOFF");

    /* 메모리 해제 */
    for (int i = 0; i < n_buffers; ++i)
        if (-1 == munmap(buffers[i].start, buffers[i].length))
            mesg_exit("munmap");
    free(buffers);

    munmap(fbp, screensize_Frambuffer);

    /* 장치 닫기 */
    if (-1 == close(camfd) && -1 == close(fbfd))
        mesg_exit("close");

    return EXIT_SUCCESS;
}



/* 값이 지정된 범위(0~255)를 벗어나지 않도록 경계 검사를 수행하는 함수 */
extern inline int clip(int value, int min, int max)
{
    return (value > max ? max : value < min ? min : value);
}

/* 캡처된 YUYV 형식의 데이터를 처리하여 프레임버퍼에 출력하는 함수 */
static void process_image(const void *p)
{
    unsigned char* in = (unsigned char*)p;  /* 캡처된 비디오 데이터 포인터 (YUYV 형식) */
    int width = WIDTH;                      /* 비디오 데이터의 너비 (픽셀 수) */
    int height = HEIGHT;                    /* 비디오 데이터의 높이 (픽셀 수) */
    int istride = WIDTH * 2;                /* 한 라인의 데이터 크기 (YUYV 형식은 픽셀당 2바이트 사용) */
    int x, y, j;                            /* 반복문에서 사용할 변수 */
    int y0, u, y1, v, r, g, b, a = 0xff, depth_fb = vinfo.bits_per_pixel/8;    /* YUYV 데이터를 분리한 후 RGBA 변환에 사용할 변수 */
    long location = 0;                      /* 프레임버퍼에서 현재 위치를 가리킬 인덱스 */

    /* YUYV 데이터를 RGBA로 변환한 후 프레임버퍼에 쓰는 루프 */
    for (y = 0; y < height; ++y) {  /* 각 라인을 반복 */
        for (j = 0, x = 0; x < vinfo.xres; j += 4, x += 2) {  /* 한 라인 내에서 YUYV 데이터를 2픽셀씩 처리 */
            /* 프레임버퍼의 크기를 넘어서지 않는지 검사 */
            if (j >= istride) {  /* 비디오 데이터의 유효 범위를 넘으면 넘어가서 빈 공간을 채움 */
                 location += 2*depth_fb;
                 continue;
            }


            /* YUYV 데이터에서 Y0, U, Y1, V 성분을 분리 */
            y0 = in[j];              /* 첫 번째 픽셀의 밝기 값 (Y0) */
            u = in[j + 1] - 128;     /* 색차 성분 U, 중앙값 128을 기준으로 보정 */
            y1 = in[j+ 2];          /* 두 번째 픽셀의 밝기 값 (Y1) */
            v = in[j + 3] - 128;     /* 색차 성분 V, 중앙값 128을 기준으로 보정 */

            /* YUV를 RGB로 변환 (첫 번째 픽셀) */
            r = clip((298 * y0 + 409 * v + 128) >> 8, 0, 255);  /* 밝기(Y)와 색차(V)를 사용해 빨강(R) 값 계산 */
            g = clip((298 * y0 - 100 * u - 208 * v + 128) >> 8, 0, 255);  /* Y, U, V를 사용해 초록(G) 값 계산 */
            b = clip((298 * y0 + 516 * u + 128) >> 8, 0, 255);  /* 밝기(Y)와 색차(U)를 사용해 파랑(B) 값 계산 */

            /* 32비트 BMP 저장을 위한 RGB 값 저장 */
            *(fbp + location++) = b;  /* 파란색 값 저장 */
            *(fbp + location++) = g;  /* 초록색 값 저장 */
            *(fbp + location++) = r;  /* 빨간색 값 저장 */
            *(fbp + location++) = a;  /* 알파 값 (투명도) 저장 */


            /* YUV를 RGB로 변환 (두 번째 픽셀) */
            r = clip((298 * y1 + 409 * v + 128) >> 8, 0, 255);  /* 두 번째 픽셀의 R 값 계산 */
            g = clip((298 * y1 - 100 * u - 208 * v + 128) >> 8, 0, 255);  /* 두 번째 픽셀의 G 값 계산 */
            b = clip((298 * y1 + 516 * u + 128) >> 8, 0, 255);  /* 두 번째 픽셀의 B 값 계산 */

            /* BMP 저장을 위한 두 번째 픽셀의 RGB 값 저장 */
            *(fbp + location++) = b;  /* 두 번째 픽셀의 파란색 값 */
            *(fbp + location++) = g;  /* 두 번째 픽셀의 초록색 값 */
            *(fbp + location++) = r;  /* 두 번째 픽셀의 빨간색 값 */
            *(fbp + location++) = a;  /* 두 번째 픽셀의 알파 값 (투명도) */
        }
        in += istride;  /* 한 라인 처리가 끝나면 다음 라인으로 이동 */
    }
}


/* 프레임을 읽어 처리하는 함수 */
static int read_frame(int fd)
{
    struct v4l2_buffer buf;  /* V4L2에서 사용할 버퍼 구조체 선언 */
    memset(&buf, 0, sizeof(buf));  /* buf 구조체를 0으로 초기화 (안전한 사용을 위해) */
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  /* 버퍼 타입을 비디오 캡처로 설정 */
    buf.memory = V4L2_MEMORY_MMAP;           /* 메모리 매핑 방식으로 설정 (MMAP 사용) */
    
    /* 큐에서 버퍼를 제거 (VIDIOC_DQBUF 요청: 비디오 장치에서 한 프레임의 데이터를 가져옴) */
    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {  /* VIDIOC_DQBUF 시스템 호출: 큐에서 버퍼를 반환받음 */
        switch (errno) {  /* 시스템 호출 오류 시, errno를 통해 오류 유형을 확인 */
        case EAGAIN: return 0;  /* EAGAIN: 큐에 버퍼가 아직 준비되지 않은 경우, 0을 반환하고 다시 대기 */
        case EIO:    /* I/O 오류가 발생할 경우, 이를 무시하고 계속 진행 */
        default: mesg_exit("VIDIOC_DQBUF");  /* 그 외의 오류가 발생하면 프로그램 종료 */
        }
    }

    /* 가져온 프레임을 처리 (process_image 함수에서 처리) */
    process_image(buffers[buf.index].start);  /* 가져온 버퍼 데이터를 처리 (버퍼의 시작 주소를 사용) */

    /* 처리한 버퍼를 다시 큐에 넣음 (VIDIOC_QBUF 요청: 장치에서 다시 사용될 수 있도록 버퍼를 큐에 반환) */
    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))  /* 프레임 처리가 끝난 후 다시 큐에 버퍼를 넣음 */
        mesg_exit("VIDIOC_QBUF");  /* 큐에 넣는 데 실패하면 프로그램 종료 */

    return 1;  /* 프레임을 성공적으로 처리했음을 나타내기 위해 1을 반환 */
}


/* 메인 루프: 프레임을 읽고 처리하는 반복 루프 */
static void mainloop(int fd)
{
    unsigned int count = 100;  /* 100프레임 캡처: 총 100개의 프레임을 읽기 위한 카운터 설정 */
    while (count-- > 0) {  /* 100번 반복하며 프레임을 읽음 */
        for (;;) {  /* 내부 무한 루프: 성공적으로 프레임을 읽을 때까지 반복 */
            fd_set fds;  /* 파일 디스크립터 셋을 선언: select()로 이벤트를 감시할 파일 디스크립터 */
            struct timeval tv;  /* 타임아웃을 설정하기 위한 구조체 */

            FD_ZERO(&fds);  /* fd_set을 초기화: 모든 비트가 0으로 설정됨 */
            FD_SET(fd, &fds);  /* 비디오 장치 파일 디스크립터(fd)를 fd_set에 추가 */

            /* 타임아웃 설정: 최대 2초 동안 대기 */
            tv.tv_sec = 2;  /* 초 단위 타임아웃 설정 (2초 대기) */
            tv.tv_usec = 0;  /* 마이크로초 단위 타임아웃 설정 (0마이크로초) */

            /* select() 호출: 파일 디스크립터에서 이벤트가 발생할 때까지 대기 */
            int r = select(fd + 1, &fds, NULL, NULL, &tv);  
            /* select()는 파일 디스크립터에서 읽기 가능 상태가 될 때까지 대기 (또는 타임아웃) */
            if (-1 == r) {  /* select() 호출이 실패한 경우 */
                if (EINTR == errno) continue;  /* 인터럽트로 인해 중단된 경우, 루프를 재시작 */
                mesg_exit("select");  /* 그 외 오류 발생 시 프로그램 종료 */
            } else if (0 == r) {  /* select() 호출이 타임아웃으로 인해 반환된 경우 */
                fprintf(stderr, "select timeout\n");  /* 타임아웃 메시지 출력 */
                exit(EXIT_FAILURE);  /* 타임아웃 발생 시 프로그램 종료 */
            }

            /* 프레임을 읽음: read_frame() 함수를 호출해 프레임을 읽음 */
            if (read_frame(fd)) break;  /* 프레임이 성공적으로 읽히면 무한 루프를 종료하고 다음 프레임 처리로 이동 */
        }
    }
}