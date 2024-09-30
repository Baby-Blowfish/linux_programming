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

#include <asm/types.h>               /* for videodev2.h */
#include <linux/videodev2.h>

#define FBDEV        "/dev/fb0"      /* 프레임 버퍼를 위한 디바이스 파일 */
#define VIDEODEV    "/dev/video0"
#define WIDTH       800               /* 캡쳐받을 영상의 크기 */
#define HEIGHT      600                

/* Video4Linux에서 사용할 영상 저장을 위한 버퍼 */
struct buffer {
    void * start;
    size_t length;
};

static short *fbp   	      = NULL;         /* 프레임버퍼의 MMAP를 위한 변수 */
struct buffer *buffers        = NULL;
static unsigned int n_buffers = 0;
static struct fb_var_screeninfo vinfo;                   /* 프레임버퍼의 정보 저장을 위한 구조체 */

static void mesg_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg)
{
    int r;
    do r = ioctl(fd, request, arg); while(-1 == r && EINTR == errno);
    return r;
}

/* unsigned char의 범위를 넘어가지 않도록 경계 검사를 수행다. */
extern inline int clip(int value, int min, int max)
{
    return(value > max ? max : value < min ? min : value);
}

static void process_image(const void *p)
{
    unsigned char* in =(unsigned char*)p;
    int width = WIDTH;
    int height = HEIGHT;
    int istride = WIDTH*2;          /* 이미지의 폭을 넘어가면 다음 라인으로 내려가도록 설정 */
    int x, y, j;
    int y0, u, y1, v, r, g, b;
    unsigned short pixel;
    long location = 0;
    for(y = 0; y < height; ++y) {
        for(j = 0, x = 0; j < vinfo.xres * 2; j += 4, x += 2) {
            if(j >= width*2) {                 /* 현재의 화면에서 이미지를 넘어서는 빈 공간을 처리 */
                 location++; location++;
                 continue;
            }
            /* YUYV 성분을 분리 */
            y0 = in[j];
            u = in[j + 1] - 128;
            y1 = in[j + 2];
            v = in[j + 3] - 128;

            /* YUV를 RGB로 전환 */
            r = clip((298 * y0 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y0 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y0 + 516 * u + 128) >> 8, 0, 255);
            pixel =((r>>3)<<11)|((g>>2)<<5)|(b>>3);                 /* 16비트 컬러로 전환 */
            fbp[location++] = pixel;

            /* YUV를 RGB로 전환 */
            r = clip((298 * y1 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y1 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y1 + 516 * u + 128) >> 8, 0, 255);
            pixel =((r>>3)<<11)|((g>>2)<<5)|(b>>3);                 /* 16비트 컬러로 전환 */
            fbp[location++] = pixel;
        };
        in += istride;
    };
}

static int read_frame(int fd)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch(errno) {
        case EAGAIN: return 0;
        case EIO:
            /* Could ignore EIO, see spec. */
        default: mesg_exit("VIDIOC_DQBUF");
        }
    }

    process_image(buffers[buf.index].start);

    if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        mesg_exit("VIDIOC_QBUF");

    return 1;
}

static void mainloop(int fd)
{
    unsigned int count = 100;
    while(count-- > 0) {
        for(;;) {
            fd_set fds;
            struct timeval tv;

            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            /* Timeout. */
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            int r = select(fd + 1, &fds, NULL, NULL, &tv);
            if(-1 == r) {
                if(EINTR == errno) continue;
                mesg_exit("select");
            } else if(0 == r) {
                fprintf(stderr, "select timeout\n");
                exit(EXIT_FAILURE);
            }

            if(read_frame(fd)) break;
        }
    }
}

static void start_capturing(int fd)
{
    for(int i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;
	memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;
        if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            mesg_exit("VIDIOC_QBUF");
    }
    
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(fd, VIDIOC_STREAMON, &type))
        mesg_exit("VIDIOC_STREAMON");
}

static void init_mmap(int fd)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count       = 4;
    req.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory      = V4L2_MEMORY_MMAP;
    if(-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if(EINVAL == errno) {
            fprintf(stderr, "%s does not support memory mapping\n", VIDEODEV);
            exit(EXIT_FAILURE);
        } else {
            mesg_exit("VIDIOC_REQBUFS");
        }
    }

    if(req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", VIDEODEV);
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));
    if(!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for(n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;
        if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            mesg_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd, buf.m.offset);
        if(MAP_FAILED == buffers[n_buffers].start)
            mesg_exit("mmap");
    }
}

static void init_device(int fd)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if(EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n", VIDEODEV);
            exit(EXIT_FAILURE);
        } else {
            mesg_exit("VIDIOC_QUERYCAP");
        }
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n",
                         VIDEODEV);
        exit(EXIT_FAILURE);
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n", VIDEODEV);
        exit(EXIT_FAILURE);
    }

    /* Select video input, video standard and tune here. */
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */
        xioctl(fd, VIDIOC_S_CROP, &crop);
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = WIDTH;
    fmt.fmt.pix.height      = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    if(-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        mesg_exit("VIDIOC_S_FMT");

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if(fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if(fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    init_mmap(fd);
} 

int main(int argc, char **argv)
{
    int fbfd = -1;              /* 프레임버퍼의 파일 디스크립터 */
    int camfd = -1;		/* 카메라의 파일 디스크립터 */

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

    /* 캡쳐 중단 */
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(camfd, VIDIOC_STREAMOFF, &type))
        mesg_exit("VIDIOC_STREAMOFF");

    /* 메모리 정리 */
    for(int i = 0; i < n_buffers; ++i)
        if(-1 == munmap(buffers[i].start, buffers[i].length))
            mesg_exit("munmap");
    free(buffers);

    munmap(fbp, screensize);

    /* 장치 닫기 */
    if(-1 == close(camfd) && -1 == close(fbfd))
        mesg_exit("close");

    return EXIT_SUCCESS; 
}
