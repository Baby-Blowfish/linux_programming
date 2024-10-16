#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>                  /* 저수준 입출력을 위한 헤더 */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/fb.h>               /* 프레임버퍼 장치를 제어하기 위한 헤더 */
#include <asm/types.h>              /* videodev2.h에서 필요한 데이터 타입 정의 */
#include <linux/videodev2.h>        /* V4L2를 제어하기 위한 헤더 */

#define FBDEV        "/dev/fb0"      /* 프레임 버퍼 디바이스 파일 경로 */
#define VIDEODEV     "/dev/video0"   /* 비디오 캡처 디바이스 파일 경로 */
#define WIDTH        800             /* 캡처할 영상의 너비 */
#define HEIGHT       600             /* 캡처할 영상의 높이 */

/* Video4Linux2에서 사용할 영상 데이터를 저장할 버퍼 구조체 */
struct buffer {
    void * start;    /* 버퍼 시작 주소 */
    size_t length;   /* 버퍼 길이 */
};

static short *fbp = NULL;               /* 프레임버퍼의 메모리 맵핑을 위한 변수 */
struct buffer *buffers = NULL;          /* 비디오 캡처용 버퍼 */
static unsigned int n_buffers = 0;      /* 버퍼 개수 */
static struct fb_var_screeninfo vinfo;  /* 프레임버퍼의 정보 저장 구조체 */

/* 에러 발생 시 메시지 출력 후 종료하는 함수 */
static void mesg_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

/* ioctl() 호출을 안전하게 수행하는 함수 */
static int xioctl(int fd, int request, void *arg)
{
    int r;
    do r = ioctl(fd, request, arg); while (-1 == r && EINTR == errno);  /* EINTR 오류 시 재시도 */
    return r;
}

/* 값이 지정된 범위(0~255)를 벗어나지 않도록 경계 검사를 수행하는 함수 */
extern inline int clip(int value, int min, int max)
{
    return (value > max ? max : value < min ? min : value);
}

/* 캡처된 YUYV 형식의 데이터를 처리하여 프레임버퍼에 출력하는 함수 */
static void process_image(const void *p)
{
    unsigned char* in = (unsigned char*)p;  /* 캡처된 비디오 데이터 */
    int width = WIDTH;                      /* 캡처된 영상의 너비 */
    int height = HEIGHT;                    /* 캡처된 영상의 높이 */
    int istride = WIDTH * 2;                /* 한 라인의 데이터 길이 (YUYV는 픽셀당 2바이트) */
    int x, y, j;
    int y0, u, y1, v, r, g, b;
    unsigned short pixel;
    long location = 0;

    /* YUYV 데이터를 RGB로 변환한 후 프레임버퍼에 쓰는 루프 */
    for (y = 0; y < height; ++y) {
        for (j = 0, x = 0; j < vinfo.xres * 2; j += 4, x += 2) {
            /* 프레임버퍼의 크기를 넘어서지 않는지 검사 */
            if (j >= width * 2) {
                 location++; location++;
                 continue;
            }
            /* YUYV 데이터에서 Y0, U, Y1, V 성분을 분리 */
            y0 = in[j];
            u = in[j + 1] - 128;
            y1 = in[j + 2];
            v = in[j + 3] - 128;

            /* YUV를 RGB로 변환 (첫 번째 픽셀) */
            r = clip((298 * y0 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y0 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y0 + 516 * u + 128) >> 8, 0, 255);
            pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);  /* 16비트 RGB로 변환 */
            fbp[location++] = pixel;  /* 첫 번째 픽셀을 프레임버퍼에 기록 */

            /* YUV를 RGB로 변환 (두 번째 픽셀) */
            r = clip((298 * y1 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y1 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y1 + 516 * u + 128) >> 8, 0, 255);
            pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);  /* 16비트 RGB로 변환 */
            fbp[location++] = pixel;  /* 두 번째 픽셀을 프레임버퍼에 기록 */
        }
        in += istride;  /* 한 라인 처리가 끝나면 다음 라인으로 이동 */
    }
}

/* 프레임을 읽어 처리하는 함수 */
static int read_frame(int fd)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  /* 비디오 캡처 타입 */
    buf.memory = V4L2_MEMORY_MMAP;           /* 메모리 매핑 방식 */
    
    /* 큐에서 버퍼를 제거 (비디오 장치에서 한 프레임을 가져옴) */
    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
        case EAGAIN: return 0;
        case EIO:    /* I/O 오류는 무시할 수 있음 */
        default: mesg_exit("VIDIOC_DQBUF");
        }
    }

    /* 가져온 프레임을 처리 */
    process_image(buffers[buf.index].start);

    /* 다시 버퍼를 큐에 넣음 (다음 프레임을 위해) */
    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        mesg_exit("VIDIOC_QBUF");

    return 1;
}

/* 메인 루프: 프레임을 읽고 처리하는 반복 루프 */
static void mainloop(int fd)
{
    unsigned int count = 100;  /* 100프레임 캡처 */
    while (count-- > 0) {
        for (;;) {
            fd_set fds;
            struct timeval tv;

            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            /* 타임아웃 설정 */
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            int r = select(fd + 1, &fds, NULL, NULL, &tv);
            if (-1 == r) {
                if (EINTR == errno) continue;  /* 인터럽트 오류 시 재시도 */
                mesg_exit("select");
            } else if (0 == r) {
                fprintf(stderr, "select timeout\n");
                exit(EXIT_FAILURE);
            }

            /* 프레임을 읽으면 루프를 종료 */
            if (read_frame(fd)) break;
        }
    }
}

/* 캡처 시작을 위한 함수 */
static void start_capturing(int fd)
{
    for (int i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        /* 버퍼를 큐에 넣음 */
        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            mesg_exit("VIDIOC_QBUF");
    }
    
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    /* 스트리밍 시작 */
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
        mesg_exit("VIDIOC_STREAMON");
}

/* 메모리 맵핑 방식의 초기화 함수 */
static void init_mmap(int fd)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;                        /* 4개의 버퍼 요청 */
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    /* 메모리 매핑을 요청 */
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support memory mapping\n", VIDEODEV);
            exit(EXIT_FAILURE);
        } else {
            mesg_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", VIDEODEV);
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));  /* 버퍼를 위한 메모리 할당 */
    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    /* 각 버퍼에 대해 메모리 맵핑 */
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        /* 버퍼 정보 조회 */
        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            mesg_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd, buf.m.offset);
        if (MAP_FAILED == buffers[n_buffers].start)
            mesg_exit("mmap");
    }
}

/* V4L2 장치 초기화 함수 */
static void init_device(int fd)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    /* 장치 기능 조회 */
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n", VIDEODEV);
            exit(EXIT_FAILURE);
        } else {
            mesg_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n", VIDEODEV);
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n", VIDEODEV);
        exit(EXIT_FAILURE);
    }

    /* 비디오 입력 및 크롭 설정 */
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;  /* 기본 크롭 설정 */
        xioctl(fd, VIDIOC_S_CROP, &crop);
    }

    /* 비디오 포맷 설정 */
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  /* YUYV 포맷 */
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        mesg_exit("VIDIOC_S_FMT");

    /* 드라이버의 버그 방지 */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    /* 메모리 맵핑 초기화 */
    init_mmap(fd);
}

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

    /* 프레임버퍼 정보 가져오기 */
    if (-1 == ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information.");
        return EXIT_FAILURE;
    }

    /* 프레임버퍼를 위한 메모리 맵핑 */
    long screensize = vinfo.xres * vinfo.yres * 2;
    fbp = (short *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fbp == (short*)-1) {
        perror("mmap() : framebuffer device to memory");
        return EXIT_FAILURE;
    }

    /* 프레임버퍼 초기화 */
    memset(fbp, 0, screensize);

    /* 카메라 장치 열기 */
    camfd = open(VIDEODEV, O_RDWR | O_NONBLOCK, 0);
    if (-1 == camfd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", VIDEODEV, errno, strerror(errno));
        return EXIT_FAILURE;
    }

    /* V4L2 장치 초기화 */
    init_device(camfd);

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

    munmap(fbp, screensize);

    /* 장치 닫기 */
    if (-1 == close(camfd) && -1 == close(fbfd))
        mesg_exit("close");

    return EXIT_SUCCESS;
}
