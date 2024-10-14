#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/videodev2.h>

#define WIDTH 		    800
#define HEIGHT 		    600  
#define BUFFER_COUNT	4  	// 버퍼의 개수
#define VIDEO_DEV     "/dev/video0"
#define FB_DEV        "/dev/fb0"

typedef unsigned char uchar;

struct buffer {
    void   *start;
    size_t length;
};

static int cond = 1;
static struct fb_var_screeninfo vinfo;

static void sigHandler(int signo)
{
    cond = 0;
}

static inline int clip(int value, int min, int max)
{
    return (value > max ? max : (value < min ? min : value));
}

// YUYV 데이터를 RGB565로 변환하는 함수
void yuyv2Rgb565(uchar *yuyv, unsigned short *rgb, int width, int height) 
{
    uchar* in = (uchar*)yuyv;
    unsigned short pixel;
    int istride = width*2;     /* 이미지의 폭을 넘어가면 다음 라인으로 내려가도록 설정 */
    int x, y, j;
    int y0, u, y1, v, r, g, b;
    long loc = 0;
    for (y = 0; y < height; ++y) {
        for (j = 0, x = 0; j < vinfo.xres * 2; j += 4, x += 2) {
            if (j >= width*2) {                 /* 현재의 화면에서 이미지를 넘어서는 빈 공간을 처리 */
                 loc++; loc++;
                 continue;
            }
            /* YUYV 성분을 분리 */
            y0 = in[j];
            u = in[j + 1] - 128;
            y1 = in[j + 2];
            v = in[j + 3] - 128;

            /* YUV를 RGB로 전환: Y0 + U + V */
            r = clip((298 * y0 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y0 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y0 + 516 * u + 128) >> 8, 0, 255);
            pixel = ((r>>3)<<11)|((g>>2)<<5)|(b>>3);      /* 16비트 컬러로 전환 */
            rgb[loc++] = pixel;

            /* YUV를 RGB로 전환 : Y1 + U + V */
            r = clip((298 * y1 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y1 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y1 + 516 * u + 128) >> 8, 0, 255);
            pixel = ((r>>3)<<11)|((g>>2)<<5)|(b>>3);      /* 16비트 컬러로 전환 */
            rgb[loc++] = pixel;
        }
        in += istride;
    }
}

// 프레임버퍼를 설정하는 함수
int initFramebuffer(unsigned short **fbPtr, int *size) 
{
    int fd = open(FB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open framebuffer device");
        return -1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fd);
        return -1;
    }

    *size = vinfo.yres_virtual * vinfo.xres_virtual * vinfo.bits_per_pixel / 8;
    *fbPtr = (unsigned short *)mmap(0, *size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*fbPtr == MAP_FAILED) {
        perror("Failed to mmap framebuffer");
        close(fd);
        return -1;
    }

    return fd;
}

// V4L2 설정
static int init_v4l2(int *fd, struct buffer *buffers) 
{
    struct v4l2_format format;
    struct v4l2_requestbuffers reqbuf;
    struct v4l2_buffer buf;
    int i;

    *fd = open(VIDEO_DEV, O_RDWR);
    if (*fd < 0) {
        perror("Failed to open video device");
        return -1;
    }

    // 포맷 설정 (YUYV)
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = WIDTH;
    format.fmt.pix.height = HEIGHT;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(*fd, VIDIOC_S_FMT, &format) < 0) {
        perror("Failed to set format");
        close(*fd);
        return -1;
    }
    printf("영상의 해상도 : %d x %d\n", format.fmt.pix.width, format.fmt.pix.height);

    // 버퍼 요청
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = BUFFER_COUNT;

    // 더블 버퍼링(Double Buffering)
    if (ioctl(*fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
        perror("Failed to request buffers");
        close(*fd);
        return -1;
    }

    // 버퍼 매핑
    for (i = 0; i < BUFFER_COUNT; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(*fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("Failed to query buffer");
            close(*fd);
            return -1;
        }

        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            perror("Failed to mmap buffer");
            close(*fd);
            return -1;
        }

        // 큐에 버퍼를 넣음
        if (ioctl(*fd, VIDIOC_QBUF, &buf) < 0) {
            perror("Failed to queue buffer");
            close(*fd);
            return -1;
        }
    }

    // 캡쳐 시작
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(*fd, VIDIOC_STREAMON, &type) < 0) {
        perror("Failed to start capturing");
        close(*fd);
        return -1;
    }

    return 0;
}

int main(int argc, char** argv) 
{
    unsigned short *rgbBuffer, *fbPtr;
    int cam_fd, fb_fd;
    int fbSize, buffer_size = WIDTH * HEIGHT * 2; // YUYV는 픽셀당 2바이트 
    struct buffer buffers[BUFFER_COUNT];
    struct v4l2_buffer buf;                     // V4L2에서 사용할 메모리 버퍼

    signal(SIGINT, sigHandler);

    // V4L2 초기화
    if (init_v4l2(&cam_fd, buffers) < 0) {
        fprintf(stderr, "V4L2 initialization failed\n");
        return -1;
    }

    // 프레임버퍼 초기화
    fb_fd = initFramebuffer(&fbPtr, &fbSize);
    if (fb_fd < 0) {
        fprintf(stderr, "Failed to initialize framebuffer\n"); 
        return -1;
    }

    // 영상을 저장할 메모리 할당 
    rgbBuffer = (unsigned short *)malloc(fbSize);
    if (!rgbBuffer) {
        perror("Failed to allocate buffers");
        close(fb_fd);
        return -1;
    }

    // V4L2를 이용한 영상의 캡쳐 및 표시
    while (cond) {
        // 버퍼 초기화
        memset(&buf, 0, sizeof(buf));

        // MMAP 기반으로 영상 캡쳐
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(cam_fd, VIDIOC_DQBUF, &buf) < 0) {
            perror("Failed to dequeue buffer");
            break;
        }
        
        // YUYV 데이터를 RGB565로 변환
        yuyv2Rgb565(buffers[buf.index].start, rgbBuffer, WIDTH, HEIGHT);

        // 프레임버퍼에 RGB565 데이터 쓰기
        memcpy(fbPtr, rgbBuffer, fbSize); 

        // 버퍼를 다시 큐에 넣기
        if (ioctl(cam_fd, VIDIOC_QBUF, &buf) < 0) {
            perror("Failed to queue buffer");
            break;
        }
    }

    printf("\nGood Bye!!!\n");

    // 캡쳐 종료
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(cam_fd, VIDIOC_STREAMOFF, &type);

    // 메모리 정리
    for (int i = 0; i < BUFFER_COUNT; i++) {
        munmap(buffers[i].start, buffers[i].length);
    }
    munmap(fbPtr, fbSize);
    free(rgbBuffer);

    // 파일디스크립터 정리
    close(cam_fd);
    close(fb_fd);

    return 0;
}
