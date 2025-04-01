#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>                  /* 저수준 입출력 처리를 위한 헤더 파일 */
#include <limits.h>
#include <unistd.h>                 /* POSIX 운영체제 API */
#include <errno.h>                  /* 오류 코드 정의 */
#include <malloc.h>                 /* 동적 메모리 할당 관련 함수들 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>               /* 메모리 맵핑 관련 함수들 */
#include <sys/ioctl.h>              /* 입출력 제어 함수 (ioctl) 관련 헤더 */

#include <linux/fb.h>               /* 프레임버퍼 관련 구조체 및 함수 */
#include <asm/types.h>              /* 비디오 장치를 제어하기 위한 데이터 타입들 */
#include <linux/videodev2.h>        /* Video4Linux2 API를 위한 헤더 */

#include "bmpHeader.h"              /* BMP 파일 저장을 위한 헤더 파일 */

#define NUMCOLOR	3               /* BMP 파 일 RGB 색상 성분의 개수 */
#define FBDEV       "/dev/fb0"		/* 프레임 버퍼 장치 파일 경로 */
#define VIDEODEV    "/dev/video0"   /* 비디오 캡처 장치 파일 경로 */
#define WIDTH       800             /* 캡처할 이미지의 너비 */
#define HEIGHT      600             /* 캡처할 이미지의 높이 */

typedef unsigned char u2byte;

/* 영상 데이터 버퍼 구조체 */
struct buffer {
    void * start;    /* 버퍼 시작 주소 */
    size_t length;   /* 버퍼 길이 */
};

static short *fbp = NULL;                  /* 프레임버퍼 메모리 맵핑을 위한 변수 */
struct buffer *buffers = NULL;             /* 비디오 캡처용 버퍼 */
static unsigned int n_buffers = 0;         /* 버퍼 개수 */
static struct fb_var_screeninfo vinfo;     /* 프레임버퍼 정보 구조체 */
#define NO_OF_LOOP 1


/* BMP 이미지를 저장하는 함수 */
void saveImage(unsigned char *inimg)
{
    RGBQUAD palrgb[256];                   /* 색상 테이블 (팔레트) */
    FILE *fp;                              /* 파일 포인터 */
    BITMAPFILEHEADER bmpFileHeader;        /* BMP 파일 헤더 */
    BITMAPINFOHEADER bmpInfoHeader;        /* BMP 정보 헤더 */

    memset(&bmpFileHeader, 0, sizeof(BITMAPFILEHEADER));   /* 파일 헤더 초기화 */
    bmpFileHeader.bfType = 0x4d42;         /* 파일 형식 ('BM') */
    bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);  /* 비트맵 데이터의 시작 위치 */
    bmpFileHeader.bfOffBits += sizeof(RGBQUAD)*256;        /* 팔레트 크기 포함 */
    bmpFileHeader.bfSize = bmpFileHeader.bfOffBits;        /* 파일 크기 계산 */
    bmpFileHeader.bfSize += WIDTH*HEIGHT*NUMCOLOR;         /* 이미지 데이터 크기 포함 */

    memset(&bmpInfoHeader, 0, sizeof(BITMAPINFOHEADER));   /* 정보 헤더 초기화 */
    bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);       /* 정보 헤더 크기 */
    bmpInfoHeader.biWidth = WIDTH;                         /* 이미지의 너비 */
    bmpInfoHeader.biHeight = HEIGHT;                       /* 이미지의 높이 */
    bmpInfoHeader.biPlanes = 1;                            /* 색상 평면 수 (항상 1) */
    bmpInfoHeader.biBitCount = NUMCOLOR*8;                 /* 픽셀당 비트 수 (RGB의 경우 24비트) */
    bmpInfoHeader.SizeImage = WIDTH * HEIGHT * bmpInfoHeader.biBitCount / 8;  /* 이미지 크기 */
    bmpInfoHeader.biXPelsPerMeter = 0x0B12;                /* 가로 해상도 */
    bmpInfoHeader.biYPelsPerMeter = 0x0B12;                /* 세로 해상도 */
    
    /* BMP 파일 생성 */
    if((fp = fopen("capture.bmp", "wb")) == NULL) {
        fprintf(stderr, "Error : Failed to open file..\n");
        exit(EXIT_FAILURE);
    }

    /* BMP 헤더 및 이미지 데이터 쓰기 */
    fwrite((void*)&bmpFileHeader, sizeof(bmpFileHeader), 1, fp);
    fwrite((void*)&bmpInfoHeader, sizeof(bmpInfoHeader), 1, fp);
    fwrite(palrgb, sizeof(RGBQUAD), 256, fp);
    fwrite(inimg, sizeof(unsigned char), WIDTH*HEIGHT*NUMCOLOR, fp);

    fclose(fp);  /* 파일 닫기 */
}

/* 에러 메시지 출력 후 종료 */
static void mesg_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

/* ioctl 함수 호출을 안전하게 수행 */
static int xioctl(int fd, int request, void *arg)
{
    int r;
    do r = ioctl(fd, request, arg); while(-1 == r && EINTR == errno);  /* 인터럽트 시 재시도 */
    return r;
}

/* 값이 지정된 범위(0~255)를 벗어나지 않도록 경계 검사 */
extern inline int clip(int value, int min, int max)
{
    return (value > max ? max : value < min ? min : value);
}

/* 캡처된 YUYV 데이터를 RGB로 변환하여 프레임버퍼에 출력하는 함수 */
static void process_image(const void *p)
{
    unsigned char* in = (unsigned char*)p;  /* YUYV 데이터 */
    int width = WIDTH;                      /* 이미지 너비 */
    int height = HEIGHT;                    /* 이미지 높이 */
    int istride = WIDTH * 2;                /* 한 라인 데이터 크기 (YUYV는 픽셀당 2바이트 사용) */
    int x, y, j;                            /* 반복문에 사용할 변수 */
    int y0, u, y1, v, r, g, b;              /* YUV -> RGB 변환을 위한 변수 */
    unsigned short pixel;                   /* 16비트 색상 값 */
    long location = 0, count = 0;                      /* 프레임버퍼 위치 */
    unsigned char inimg[NUMCOLOR * WIDTH * HEIGHT];  /* 이미지 데이터를 저장할 배열 */

    /* YUYV -> RGB 변환 후 프레임버퍼에 쓰기 */
    for (y = 0; y < height; ++y, count = 0) {
        for (j = 0, x = 0; j < vinfo.xres*2; j += 4, x += 2) {
            if (j >= width*2) {  /* 빈 공간을 처리 */
                location++;
                location++;
                continue;
            }

            /* YUYV 성분 분리 */
            y0 = in[j];
            u = in[j + 1] - 128;
            y1 = in[j + 2];
            v = in[j + 3] - 128;

            /* 첫 번째 픽셀 YUV -> RGB 변환 */
            r = clip((298 * y0 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y0 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y0 + 516 * u + 128) >> 8, 0, 255);
            
			pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);  /* 16비트 색상으로 변환 */
            fbp[location++] = pixel;  /* 2byte 단위 프레임버퍼에 저장 */

			/*Bmp 이미지 데이터 저장*/
            inimg[(height-y-1)*width*NUMCOLOR + count++] = b;
            inimg[(height-y-1)*width*NUMCOLOR + count++] = g;
            inimg[(height-y-1)*width*NUMCOLOR + count++] = r;


            /* 두 번째 픽셀 YUV -> RGB 변환 */
            r = clip((298 * y1 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y1 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y1 + 516 * u + 128) >> 8, 0, 255);
            
			pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);  /* 16비트 색상 변환 */
            fbp[location++] = pixel;  /* 프레임버퍼에 저장 */

			/*Bmp 이미지 데이터 저장*/
            inimg[(height-y-1)*width*NUMCOLOR + count++] = b;
            inimg[(height-y-1)*width*NUMCOLOR + count++] = g;
            inimg[(height-y-1)*width*NUMCOLOR + count++] = r;

        }
        in += istride;  /* 다음 라인으로 이동 */
    }

	saveImage(inimg);
}

/* 프레임을 읽어 처리하는 함수 */
static int read_frame(int fd)
{
    struct v4l2_buffer buf;                /* V4L2 버퍼 구조체 */
    memset(&buf, 0, sizeof(buf));          /* 버퍼 초기화 */
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  /* 비디오 캡처 타입 */
    buf.memory = V4L2_MEMORY_MMAP;         /* 메모리 매핑 방식 */

    /* 큐에서 버퍼를 제거하고 비디오 데이터를 읽음 */
    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
            case EAGAIN: return 0;         /* 버퍼가 준비되지 않았을 때 */
            case EIO:                     /* I/O 오류 */
            default: mesg_exit("VIDIOC_DQBUF");
        }
    }

    process_image(buffers[buf.index].start);  /* 가져온 프레임을 처리 */

    /* 버퍼를 다시 큐에 넣음 */
    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        mesg_exit("VIDIOC_QBUF");

    return 1;
}

/* 메인 루프: 프레임을 반복해서 읽고 처리 */
static void mainloop(int fd)
{
    unsigned int count = 100;              /* 100프레임을 읽을 때까지 반복 */
    while (count-- > 0) {
        for (;;) {
            fd_set fds;                    /* 파일 디스크립터 셋 */
            struct timeval tv;             /* 타임아웃 설정 */

            FD_ZERO(&fds);                 /* fd_set 초기화 */
            FD_SET(fd, &fds);              /* fd_set에 비디오 디바이스 추가 */

            /* 타임아웃 설정 (2초) */
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            /* 비디오 장치가 준비될 때까지 대기 (select) */
            int r = select(fd + 1, &fds, NULL, NULL, &tv);
            if (-1 == r) {
                if (EINTR == errno) continue;  /* 인터럽트 발생 시 다시 대기 */
                mesg_exit("select");
            } else if (0 == r) {             /* 타임아웃 발생 시 */
                fprintf(stderr, "select timeout\n");
                exit(EXIT_FAILURE);
            }

            /* 프레임을 성공적으로 읽으면 종료 */
            if (read_frame(fd)) break;
        }
    }
}

/* 비디오 캡처 시작을 위한 함수 */
static void start_capturing(int fd)
{
    for (int i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));      /* 버퍼 초기화 */
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        /* 버퍼를 큐에 넣음 */
        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            mesg_exit("VIDIOC_QBUF");
    }

    /* 스트리밍 시작 */
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
        mesg_exit("VIDIOC_STREAMON");
}

/* 메모리 맵핑 방식의 초기화 */
static void init_mmap(int fd)
{
    struct v4l2_requestbuffers req;        /* 버퍼 요청 구조체 */
    memset(&req, 0, sizeof(req));          /* 구조체 초기화 */
    req.count = 4;                         /* 4개의 버퍼 요청 */
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    /* 메모리 매핑 요청 */
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

    /* 버퍼 메모리 할당 */
    buffers = calloc(req.count, sizeof(*buffers));
    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    /* 각 버퍼에 대해 메모리 매핑 */
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));      /* 버퍼 초기화 */
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        /* 버퍼 정보 조회 */
        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            mesg_exit("VIDIOC_QUERYBUF");

        /* 버퍼 메모리 매핑 */
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd, buf.m.offset);
        if (MAP_FAILED == buffers[n_buffers].start)
            mesg_exit("mmap");
    }
}

/* V4L2 장치 초기화 */
static void init_device(int fd)
{
    struct v4l2_capability cap;           /* 장치 기능 조회 구조체 */
    struct v4l2_cropcap cropcap;          /* 크롭 기능 관련 구조체 */
    struct v4l2_crop crop;                /* 크롭 정보 구조체 */
    struct v4l2_format fmt;               /* 비디오 포맷 설정 구조체 */
    unsigned int min;

    /* 장치의 기능 확인 */
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n", VIDEODEV);
            exit(EXIT_FAILURE);
        } else {
            mesg_exit("VIDIOC_QUERYCAP");
        }
    }

    /* 비디오 캡처 장치가 맞는지 확인 */
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n", VIDEODEV);
        exit(EXIT_FAILURE);
    }

    /* 스트리밍 기능 지원 여부 확인 */
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n", VIDEODEV);
        exit(EXIT_FAILURE);
    }

    /* 비디오 입력, 크롭 설정 */
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* 기본 크롭 영역으로 설정 */
        xioctl(fd, VIDIOC_S_CROP, &crop);
    }

    /* 비디오 포맷 설정 */
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;      /* 이미지 너비 설정 */
    fmt.fmt.pix.height = HEIGHT;    /* 이미지 높이 설정 */
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  /* 픽셀 포맷 설정 (YUYV) */
    fmt.fmt.pix.field = V4L2_FIELD_NONE;  /* 인터레이스 필드 없음 (프로그레시브 스캔) */
    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        mesg_exit("VIDIOC_S_FMT");

    /* 드라이버 버그 방지 */
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
    int fbfd = -1;                  /* 프레임버퍼 파일 디스크립터 */
    int camfd = -1;                 /* 카메라 파일 디스크립터 */

    /* 프레임버퍼 열기 */
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

    /* 프레임버퍼 메모리 맵핑 */
    long screensize = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel/8);

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
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(camfd, VIDIOC_STREAMOFF, &type))
        mesg_exit("VIDIOC_STREAMOFF");

    //* 메모리 정리 */
    for (int i = 0; i < n_buffers; ++i)
        if (-1 == munmap(buffers[i].start, buffers[i].length))
            mesg_exit("munmap");
    free(buffers);

    /* 프레임버퍼 메모리 해제 */
    munmap(fbp, screensize);

    /* 장치 닫기 */
    if (-1 == close(camfd) && -1 == close(fbfd))
        mesg_exit("close");

    return EXIT_SUCCESS;
}
