#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>                  /* low-level i/o */
#include <limits.h>
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

#include "bmpHeader.h"  // BMP 파일 헤더를 처리하기 위한 사용자 정의 헤더 파일

#define NUMCOLOR 3
#define FBDEV        "/dev/fb0"      /* 프레임 버퍼를 위한 디바이스 파일 */
#define VIDEODEV    "/dev/video0"
#define WIDTH        640             /* 캡처할 영상의 너비 */
#define HEIGHT       360             /* 캡처할 영상의 높이 */

typedef unsigned char ubyte;  // unsigned char에 대한 별칭 정의

extern int readBmpHeader(const char *filename, int *cols, int *rows, int *depth_bmp);  // BMP 헤더만 읽는 함수 선언
extern int readBmpData(const char *filename, unsigned char **pData, int imageSize);  // BMP 데이터만 읽는 함수 선언

/* Video4Linux2에서 사용할 영상 데이터를 저장할 버퍼 구조체 */
struct buffer {
    void * start;    /* 버퍼 시작 주소 */
    size_t length;   /* 버퍼 길이 */
};
static ubyte *fbp = NULL;               /* 프레임버퍼의 메모리 맵핑을 위한 변수 */
struct buffer *buffers = NULL;          /* 비디오 캡처용 버퍼 */
static unsigned int n_buffers = 0;      /* 버퍼 개수 */
static struct fb_var_screeninfo vinfo;  /* 프레임버퍼의 정보 저장 구조체 */
#define NO_OF_LOOP 1


void saveImage(unsigned char *inimg)
{
	RGBQUAD palrgb[256];
	FILE *fp;
	BITMAPFILEHEADER bmpFileHeader;
	BITMAPINFOHEADER bmpInfoHeader;

	memset(&bmpFileHeader, 0, sizeof(BITMAPFILEHEADER));
	bmpFileHeader.bfType = 0x4d42;		// (unsigned short)('B'|'M' << 8)
	bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpFileHeader.bfOffBits += sizeof(RGBQUAD)*256;
	bmpFileHeader.bfSize = bmpFileHeader.bfOffBits;
	bmpFileHeader.bfSize += WIDTH*HEIGHT*NUMCOLOR;

	memset(&bmpInfoHeader, 0, sizeof(BITMAPINFOHEADER));
	bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfoHeader.biWidth = WIDTH;
	bmpInfoHeader.biHeight = HEIGHT;
	bmpInfoHeader.biPlanes = 1;
	bmpInfoHeader.biBitCount = NUMCOLOR*8;
	bmpInfoHeader.SizeImage = WIDTH*HEIGHT*bmpInfoHeader.biBitCount/8;
	bmpInfoHeader.biXPelsPerMeter = 0x0B12;
	bmpInfoHeader.biYPelsPerMeter = 0x0B12;
	
	if((fp= fopen("capture.bmp","wb"))==NULL)
	{
		fprintf(stderr,"Error : Failed to open file..\n");
		exit(EXIT_FAILURE);
	}

	fwrite((void*)&bmpFileHeader, sizeof(bmpFileHeader),1,fp);
	fwrite((void*)&bmpInfoHeader, sizeof(bmpInfoHeader),1,fp);
	fwrite(palrgb, sizeof(RGBQUAD),256,fp);
	fwrite(inimg, sizeof(unsigned char),WIDTH*HEIGHT*NUMCOLOR,fp);

	fclose(fp);

}





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
    unsigned char* in = (unsigned char*)p;  /* 캡처된 비디오 데이터 포인터 (YUYV 형식) */
    int width = WIDTH;                      /* 비디오 데이터의 너비 (픽셀 수) */
    int height = HEIGHT;                    /* 비디오 데이터의 높이 (픽셀 수) */
    int istride = WIDTH * 2;                /* 한 라인의 데이터 크기 (YUYV 형식은 픽셀당 2바이트 사용) */
    int x, y, j;                            /* 반복문에서 사용할 변수 */
    int y0, u, y1, v, r, g, b, a = 0xff, depth_fb = vinfo.bits_per_pixel/8;    /* YUYV 데이터를 분리한 후 RGBA 변환에 사용할 변수 */
    long location = 0, count =0;                      /* 프레임버퍼에서 현재 위치를 가리킬 인덱스 */
    ubyte inimg[NUMCOLOR*WIDTH*HEIGHT];     // Bmp 이미지 저장을 위한 변수


    /* YUYV 데이터를 RGBA로 변환한 후 프레임버퍼에 쓰는 루프 */
    for (y = 0; y < height; ++y, count = 0) {  /* 각 라인을 반복 */
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

            /* 32비트 Frame Buffer 저장을 위한 RGB 값 저장 */
            *(fbp + location++) = b;  /* 파란색 값 저장 */
            *(fbp + location++) = g;  /* 초록색 값 저장 */
            *(fbp + location++) = r;  /* 빨간색 값 저장 */
            *(fbp + location++) = a;  /* 알파 값 (투명도) 저장 */

            /*Bmp 이미지 데이터 저장*/
            inimg[(height-y-1)*width*NUMCOLOR + count++] = b;
            inimg[(height-y-1)*width*NUMCOLOR + count++] = g;
            inimg[(height-y-1)*width*NUMCOLOR + count++] = r;

            /* YUV를 RGB로 변환 (두 번째 픽셀) */
            r = clip((298 * y1 + 409 * v + 128) >> 8, 0, 255);  /* 두 번째 픽셀의 R 값 계산 */
            g = clip((298 * y1 - 100 * u - 208 * v + 128) >> 8, 0, 255);  /* 두 번째 픽셀의 G 값 계산 */
            b = clip((298 * y1 + 516 * u + 128) >> 8, 0, 255);  /* 두 번째 픽셀의 B 값 계산 */

            /* Frame Buffer 저장을 위한 두 번째 픽셀의 RGB 값 저장 */
            *(fbp + location++) = b;  /* 두 번째 픽셀의 파란색 값 */
            *(fbp + location++) = g;  /* 두 번째 픽셀의 초록색 값 */
            *(fbp + location++) = r;  /* 두 번째 픽셀의 빨간색 값 */
            *(fbp + location++) = a;  /* 두 번째 픽셀의 알파 값 (투명도) */

            /*Bmp 이미지 데이터 저장*/
            inimg[(height-y-1)*width*NUMCOLOR + count++] = b;
            inimg[(height-y-1)*width*NUMCOLOR + count++] = g;
            inimg[(height-y-1)*width*NUMCOLOR + count++] = r;
        }
        in += istride;  /* 한 라인 처리가 끝나면 다음 라인으로 이동 */
    }

    /*이미지 데이터를 BMP 파일로 저장*/
    saveImage(inimg);
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
    memset(&req, 0, sizeof(req));  /* req 구조체의 모든 필드를 0으로 초기화 */
    req.count = 4;  /* 4개의 버퍼 요청. 즉, 4개의 프레임을 처리할 수 있도록 버퍼를 할당 */
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  /* 비디오 캡처를 위한 버퍼 타입 지정 */
    req.memory = V4L2_MEMORY_MMAP;  /* 메모리 매핑 방식(MMAP)을 사용하여 버퍼를 관리 */

    /* VIDIOC_REQBUFS를 사용해 메모리 매핑을 요청 */
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {  /* 버퍼 요청이 실패할 경우 */
        if (EINVAL == errno) {  /* 장치가 메모리 매핑을 지원하지 않는 경우 */
            fprintf(stderr, "%s does not support memory mapping\n", VIDEODEV);
            exit(EXIT_FAILURE);  /* 메모리 매핑을 지원하지 않으면 프로그램 종료 */
        } else {
            mesg_exit("VIDIOC_REQBUFS");  /* 그 외의 다른 오류가 발생한 경우 */
        }
    }

    /* 요청한 버퍼 수가 너무 적으면 오류 처리 */
    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", VIDEODEV);  /* 버퍼 메모리가 충분하지 않음을 알림 */
        exit(EXIT_FAILURE);  /* 적절한 버퍼 수가 확보되지 않으면 프로그램 종료 */
    }

    /* 요청한 버퍼 수만큼의 메모리를 할당 (calloc을 사용해 메모리를 0으로 초기화) */
    buffers = calloc(req.count, sizeof(*buffers));  /* 버퍼 정보를 저장할 메모리를 동적으로 할당 */
    if (!buffers) {
        fprintf(stderr, "Out of memory\n");  /* 메모리 할당 실패 시 오류 메시지 출력 */
        exit(EXIT_FAILURE);  /* 메모리 부족 시 프로그램 종료 */
    }

    /* 각 버퍼에 대해 메모리 맵핑 */
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));  /* buf 구조체 초기화 */
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  /* 비디오 캡처 타입으로 버퍼 타입 설정 */
        buf.memory = V4L2_MEMORY_MMAP;  /* 메모리 매핑 방식 설정 */
        buf.index = n_buffers;  /* 버퍼의 인덱스 설정 (0부터 시작) */

        /* 버퍼 정보를 조회 (VIDIOC_QUERYBUF 호출) */
        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            mesg_exit("VIDIOC_QUERYBUF");  /* 버퍼 조회 실패 시 오류 처리 */

        buffers[n_buffers].length = buf.length;  /* 각 버퍼의 길이를 저장 (버퍼의 크기) */
        buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, 
                                        MAP_SHARED, fd, buf.m.offset);  /* 메모리 매핑 수행 */
        if (MAP_FAILED == buffers[n_buffers].start)
            mesg_exit("mmap");  /* 메모리 맵핑이 실패하면 오류 처리 */
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
    memset(&cropcap, 0, sizeof(cropcap));  /* cropcap 구조체의 모든 필드를 0으로 초기화 */
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  /* 크롭 설정을 위한 버퍼 타입을 비디오 캡처로 설정 */
        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {  /* VIDIOC_CROPCAP: 장치의 크롭 기능 지원 여부 확인 */
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;      /* 크롭 버퍼 타입을 비디오 캡처로 설정 */
        crop.c = cropcap.defrect;  /* 기본 크롭 영역을 장치의 기본값으로 설정 (defrect는 장치의 기본 크롭 영역) */
        xioctl(fd, VIDIOC_S_CROP, &crop);  /* VIDIOC_S_CROP: 크롭 영역을 설정 (드라이버에 따라 지원되지 않을 수 있음) */
    }

    /* 비디오 포맷 설정 */
    memset(&fmt, 0, sizeof(fmt));  /* fmt 구조체의 모든 필드를 0으로 초기화 */
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  /* 포맷 설정을 위한 버퍼 타입을 비디오 캡처로 설정 */
    fmt.fmt.pix.width = WIDTH;  /* 캡처할 이미지의 너비 설정 (정의된 WIDTH 값 사용) */
    fmt.fmt.pix.height = HEIGHT;  /* 캡처할 이미지의 높이 설정 (정의된 HEIGHT 값 사용) */
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  /* 픽셀 포맷을 YUYV(YUV422)로 설정 (2바이트당 2개의 픽셀) */
    fmt.fmt.pix.field = V4L2_FIELD_NONE;  /* 필드 타입을 설정 (V4L2_FIELD_NONE: 인터레이스 필드 없음, 즉 프로그레시브 스캔) */
    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))  /* VIDIOC_S_FMT: 지정한 포맷으로 비디오 장치 설정 */
        mesg_exit("VIDIOC_S_FMT");  /* 포맷 설정 실패 시 오류 처리 */

    /* 드라이버의 버그 방지 */
    min = fmt.fmt.pix.width * 2;  /* 최소 바이트 라인 너비 계산 (YUYV 포맷의 경우 한 픽셀당 2바이트 사용) */
    if (fmt.fmt.pix.bytesperline < min)  /* 드라이버가 설정한 바이트 라인이 계산된 최소값보다 작다면 */
        fmt.fmt.pix.bytesperline = min;  /* 드라이버 버그를 방지하기 위해 바이트 라인 크기를 최소값으로 설정 */

    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;  /* 이미지 전체 크기를 계산 (바이트 라인 * 이미지 높이) */
    if (fmt.fmt.pix.sizeimage < min)  /* 드라이버가 설정한 이미지 크기가 최소값보다 작다면 */
        fmt.fmt.pix.sizeimage = min;  /* 버그를 방지하기 위해 이미지 크기를 최소값으로 설정 */


    /* 메모리 맵핑 초기화 */
    init_mmap(fd);
}

/* 메인 함수 */
int main(int argc, char **argv)
{
    int fbfd = -1;      /* 프레임버퍼의 파일 디스크립터 */
    int camfd = -1;		/* 카메라의 파일 디스크립터 */

    /* 프레임버퍼 장치 열기 */
    fbfd = open(FBDEV, O_RDWR);
    if (-1 == fbfd) {
        perror("open() : framebuffer device");
        return EXIT_FAILURE;
    }

    /* 프레임버퍼 정보 가져오기 */
    if (-1 == ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("ioctl() : FBIOGET_VSCREENINFO");
        return EXIT_FAILURE;
    }

    /* 프레임버퍼를 위한 메모리 맵핑 */
    int depth_fb = vinfo.bits_per_pixel / 8.;  // 프레임버퍼의 픽셀당 바이트 수 : 4byte
    int screensize = vinfo.yres * vinfo.xres * depth_fb;  // 프레임버퍼 전체 크기 계산
    fbp = (ubyte *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fbp == (ubyte*)-1) {
        perror("mmap() : framebuffer device to memory");
        return EXIT_FAILURE;
    }

    /* 프레임버퍼 초기화 */
    memset(fbp, 0, screensize);
    
    /* 카메라 장치 열기 */
    camfd = open(VIDEODEV, O_RDWR | O_NONBLOCK, 0);
    if(-1 == camfd) {
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
    if(-1 == close(camfd) && -1 == close(fbfd))
        mesg_exit("close");

    return EXIT_SUCCESS; 
}
