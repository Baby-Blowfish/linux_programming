#include "video_capture.h"

short *fbp = NULL;         /* 프레임버퍼의 MMAP를 위한 변수 */
struct buffer *buffers = NULL; /* 버퍼 정보를 저장할 구조체 포인터 */
unsigned int n_buffers = 0; /* 버퍼의 수 */
struct fb_var_screeninfo vinfo; /* 프레임버퍼의 정보 저장을 위한 구조체 */
int camfd = -1;		/* 카메라의 파일 디스크립터 */

void mesg_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno)); /* 오류 메시지 출력 */
    exit(EXIT_FAILURE); /* 프로그램 종료 */
}

int xioctl(int fd, int request, void *arg)
{
    int r;
    do r = ioctl(fd, request, arg); while(-1 == r && EINTR == errno); /* ioctl 호출 */
    return r; /* 반환값 */
}

/* unsigned char의 범위를 넘어가지 않도록 경계 검사를 수행 */
inline int clip(int value, int min, int max)
{
    return (value > max ? max : value < min ? min : value); /* 범위 클리핑 */
}

void process_image(const void *p)
{
    unsigned char* in = (unsigned char*)p; /* 입력 이미지 포인터 */
    int width = WIDTH; /* 이미지 너비 */
    int height = HEIGHT; /* 이미지 높이 */
    int istride = WIDTH * 2; /* 행의 스트라이드 */
    int x, y, j;
    int y0, u, y1, v, r, g, b; /* YUV 성분과 RGB 색상 저장 */
    unsigned short pixel; /* 픽셀 데이터 */
    long location = 0; /* 픽셀 위치 */

    for (y = 0; y < height; ++y) {
        for (j = 0, x = 0; j < vinfo.xres * 2; j += 4, x += 2) {
            if (j >= width * 2) { 
                location++;
                location++;
                continue; /* 너비 초과 시 다음 행으로 이동 */
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
            pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3); 
            fbp[location++] = pixel; /* 프레임버퍼에 픽셀 저장 */

            /* YUV를 RGB로 전환 */
            r = clip((298 * y1 + 409 * v + 128) >> 8, 0, 255);
            g = clip((298 * y1 - 100 * u - 208 * v + 128) >> 8, 0, 255);
            b = clip((298 * y1 + 516 * u + 128) >> 8, 0, 255);
            pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            fbp[location++] = pixel; /* 프레임버퍼에 두 번째 픽셀 저장 */
        }
        in += istride; /* 다음 행으로 이동 */
    }
}

int read_frame(int fd)
{
    struct v4l2_buffer buf; /* 비디오 버퍼 구조체 */
    memset(&buf, 0, sizeof(buf)); /* 버퍼 초기화 */
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* 비디오 캡처 타입 설정 */
    buf.memory = V4L2_MEMORY_MMAP; /* 메모리 매핑 방식 설정 */

    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) { /* 버퍼 디큐 */
        switch (errno) {
            case EAGAIN: return 0; /* 다시 시도 */
            case EIO:
            default: mesg_exit("VIDIOC_DQBUF"); /* 오류 발생 시 종료 */
        }
    }

    process_image(buffers[buf.index].start); /* 이미지 처리 */

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) /* 버퍼 인큐 */
        mesg_exit("VIDIOC_QBUF");

    return 1; /* 정상 종료 */
}

void mainloop(int fd)
{
    unsigned int count = 100; /* 루프 카운트 */

    while (count-- > 0) {
        for (;;) {
            fd_set fds; /* 파일 디스크립터 집합 */
            struct timeval tv; /* 타임아웃 구조체 */

            FD_ZERO(&fds); /* 집합 초기화 */
            FD_SET(fd, &fds); /* 파일 디스크립터 추가 */

            /* 타임아웃 설정 */
            tv.tv_sec = 2; /* 초 단위 */
            tv.tv_usec = 0; /* 마이크로초 단위 */

            int r = select(fd + 1, &fds, NULL, NULL, &tv); /* 선택 호출 */
            if (-1 == r) {
                if (EINTR == errno) continue; /* 인터럽트 발생 시 계속 */
                mesg_exit("select"); /* 오류 발생 시 종료 */
            } else if (0 == r) {
                fprintf(stderr, "select timeout\n"); /* 타임아웃 메시지 출력 */
                exit(EXIT_FAILURE); /* 프로그램 종료 */
            }

            if (read_frame(fd)) break; /* 프레임 읽기 */
        }
    }
}

void start_capturing(int fd)
{
    for (int i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf; /* 비디오 버퍼 구조체 */
        memset(&buf, 0, sizeof(buf)); /* 버퍼 초기화 */
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* 비디오 캡처 타입 설정 */
        buf.memory = V4L2_MEMORY_MMAP; /* 메모리 매핑 방식 설정 */
        buf.index = i; /* 인덱스 설정 */

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) /* 버퍼 인큐 */
            mesg_exit("VIDIOC_QBUF");
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* 비디오 캡처 타입 설정 */

    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) /* 스트리밍 시작 */
        mesg_exit("VIDIOC_STREAMON");
        
}

void init_mmap(int fd)
{
    struct v4l2_requestbuffers req; /* 버퍼 요청 구조체 */
    memset(&req, 0, sizeof(req)); /* 요청 구조체 초기화 */
    req.count = 4; /* 요청할 버퍼 수 */
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* 비디오 캡처 타입 설정 */
    req.memory = V4L2_MEMORY_MMAP; /* 메모리 매핑 방식 설정 */

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) { /* 버퍼 요청 */
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support memory mapping\n", VIDEODEV); /* 메모리 매핑 미지원 메시지 출력 */
            exit(EXIT_FAILURE); /* 프로그램 종료 */
        } else {
            mesg_exit("VIDIOC_REQBUFS"); /* 오류 발생 시 종료 */
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", VIDEODEV); /* 버퍼 메모리 부족 메시지 출력 */
        exit(EXIT_FAILURE); /* 프로그램 종료 */
    }

    buffers = calloc(req.count, sizeof(*buffers)); /* 버퍼 메모리 할당 */
    if (!buffers) {
        fprintf(stderr, "Out of memory\n"); /* 메모리 부족 메시지 출력 */
        exit(EXIT_FAILURE); /* 프로그램 종료 */
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf; /* 비디오 버퍼 구조체 */
        memset(&buf, 0, sizeof(buf)); /* 버퍼 초기화 */
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* 비디오 캡처 타입 설정 */
        buf.memory = V4L2_MEMORY_MMAP; /* 메모리 매핑 방식 설정 */
        buf.index = n_buffers; /* 인덱스 설정 */

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) /* 버퍼 정보 쿼리 */
            mesg_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length; /* 버퍼 길이 저장 */
        buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, /* 메모리 매핑 */
                                        MAP_SHARED, fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start) {
            mesg_exit("mmap"); /* 오류 발생 시 종료 */
        }
    }
}


void init_device(int fd)
{
    struct v4l2_capability cap; /* V4L2 장치의 기능 정보를 저장할 구조체 */
    struct v4l2_cropcap cropcap; /* 크롭 가능성을 저장할 구조체 */
    struct v4l2_crop crop; /* 실제 크롭을 설정할 구조체 */
    struct v4l2_format fmt; /* 비디오 형식을 설정할 구조체 */
    unsigned int min; /* 최소 크기 저장 변수 */

    /* 장치의 기능 쿼리 */
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n", VIDEODEV); /* V4L2 장치가 아님을 알림 */
            exit(EXIT_FAILURE); /* 프로그램 종료 */
        } else {
            mesg_exit("VIDIOC_QUERYCAP"); /* 오류 발생 시 종료 */
        }
    }

    /* 캡처 기능 확인 */
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n", VIDEODEV); /* 비디오 캡처 장치가 아님 */
        exit(EXIT_FAILURE); /* 프로그램 종료 */
    }

    /* 스트리밍 지원 확인 */
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n", VIDEODEV); /* 스트리밍 미지원 메시지 출력 */
        exit(EXIT_FAILURE); /* 프로그램 종료 */
    }

    /* 크롭 가능성 설정 */
    memset(&cropcap, 0, sizeof(cropcap)); /* 크롭 가능성 구조체 초기화 */
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* 비디오 캡처 타입 설정 */

    /* 크롭 가능성 쿼리 */
    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* 비디오 캡처 타입 설정 */
        crop.c = cropcap.defrect; /* 기본 크롭 설정 */

        /* 크롭 설정 */
        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            if (EINVAL == errno) {
                // 크롭 설정이 지원되지 않을 수 있음 (예: 해당 장치가 크롭을 지원하지 않음)
            }
        }
    }

    /* 비디오 형식 설정 */
    memset(&fmt, 0, sizeof(fmt)); /* 비디오 형식 구조체 초기화 */
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* 비디오 캡처 타입 설정 */
    fmt.fmt.pix.width = WIDTH; /* 너비 설정 */
    fmt.fmt.pix.height = HEIGHT; /* 높이 설정 */
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; /* 픽셀 포맷 설정 */
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED; /* 필드 설정 */

    /* 형식 설정 호출 */
    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        mesg_exit("VIDIOC_S_FMT"); /* 오류 발생 시 종료 */

    /* 최소 크기 계산 */
    min = fmt.fmt.pix.width * 2; /* 너비에 따른 최소 바이트 수 계산 */
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min; /* 최소 바이트 수로 설정 */

    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height; /* 전체 이미지 크기 계산 */
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min; /* 최소 이미지 크기로 설정 */

    init_mmap(fd); /* 메모리 매핑 초기화 호출 */
}
