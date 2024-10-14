// tcpclient.c
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

#include <linux/videodev2.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define VIDEODEV    "/dev/video0"
#define WIDTH       800
#define HEIGHT      600

#define SERVER_IP   "127.0.0.1"  // 서버의 IP 주소
#define SERVER_PORT 12345        // 포트 번호

struct buffer {
    void * start;
    size_t length;
};

static struct buffer *buffers = NULL; // 비디오 캡처를 위한 버퍼들의 동적 배열
static unsigned int n_buffers = 0; // 현재 할당된 버퍼 개수
// static int ssock = -1; // Not needed since we're not using TCP/IP

static void xioctl(int fd, int request, void *arg); // ioctl 핸들러
static void init_mmap(int fd); // 프레임버퍼를 위한 mmap 요청
static void init_device(int fd); // 카메라 초기화
static void start_capturing(int fd); // 스트림 open
static void stop_capturing(int fd); // 스트림 close
static int read_frame(int fd, unsigned char *buffer); // 
static void mainloop(int fd);

int main(int argc, char **argv)
{
    int camfd = -1;
    // struct sockaddr_in servaddr; // Not needed

    /*
    // TCP/IP code is not needed
    if((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    if(inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0) {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    // 서버에 연결
    if(connect(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }
    */

    // 카메라 초기화
    if((camfd = open(VIDEODEV, O_RDWR)) == -1) {
        perror("Opening video device");
        exit(EXIT_FAILURE);
    }

    init_device(camfd);
    start_capturing(camfd);
    mainloop(camfd);

    // 종료 처리
    stop_capturing(camfd);
    for(int i = 0; i < n_buffers; ++i)
        munmap(buffers[i].start, buffers[i].length);
    free(buffers);
    close(camfd);
    // close(ssock); // Not needed

    return 0;
}

static void xioctl(int fd, int request, void *arg) // ioctl 핸들러
{
    int r; // ioctl 반환값
    do
        r = ioctl(fd, request, arg); // 디스크립터, 요청, 매개변수
    while (-1 == r && EINTR == errno); // 인터럽트에 의한 중단일 경우, 재시도

    if (r == -1) { // 아니라면 error
        perror("xioctl");
        exit(EXIT_FAILURE);
    }
}

static void init_mmap(int fd) // 프레임버퍼를 위한 mmap 요청
{
    struct v4l2_requestbuffers req; // 버퍼 요청 구조체

    memset(&req, 0, sizeof(req)); //구조체 0으로 초기화

    req.count       = 4; // 요청 버퍼 개수
    req.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE; // 버퍼 타입
    req.memory      = V4L2_MEMORY_MMAP; // 버퍼를 memory mapping I/O로 사용 

    xioctl(fd, VIDIOC_REQBUFS, &req); // 드라이버에 mmap I/O를 위한 버퍼 공간 할당 요청

    if(req.count < 2) {
        fprintf(stderr, "At least 2 buffer memory on %s\n", VIDEODEV);
        exit(EXIT_FAILURE);
    }

    if(!(buffers = calloc(req.count, sizeof(*buffers)))) { // 버퍼 개수만큼 메모리 할당 및 초기화
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    // 할당된 버퍼 공간을 mmap을 통해 메모리에 매핑
    for(n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;
        xioctl(fd, VIDIOC_QUERYBUF, &buf);

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        
        if(MAP_FAILED == buffers[n_buffers].start) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }
}

static void init_device(int fd) // 비디오 캡처 장치(카메라) 초기화
{
    struct v4l2_capability cap;     // 장치의 기능을 저장하는 구조체
    struct v4l2_format fmt;         // 비디오 포맷을 저장하는 구조체

    // 장치의 기능 조회 호출
    xioctl(fd, VIDIOC_QUERYCAP, &cap);

    // 장치가 비디오 캡처 기능을 지원하는지 확인
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is not capturable device.\n", VIDEODEV);
        exit(EXIT_FAILURE);
    }

    // 장치가 스트리밍 I/O를 지원하는지 확인
    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s is not support streaming I/O.\n", VIDEODEV);
        exit(EXIT_FAILURE);
    }

    // 비디오 포맷 설정
    memset(&fmt, 0, sizeof(fmt)); // fmt 구조체를 0으로 초기화
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH; // 캡처 해상도 너비
    fmt.fmt.pix.height = HEIGHT; // 캡처 해상도 높이
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // 픽셀 포맷 설정 (YUYV)
    fmt.fmt.pix.field = V4L2_FIELD_NONE; // 필드 설정

    // 설정한 포맷을 장치에 적용하기 위한 ioctl 호출
    xioctl(fd, VIDIOC_S_FMT, &fmt);

    /* 버퍼 초기화 */
    init_mmap(fd); // 메모리 매핑 초기화 함수 호출
}

static void start_capturing(int fd)
{
    for(int i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;

        // 버퍼 큐에 버퍼를 추가.
        xioctl(fd, VIDIOC_QBUF, &buf);
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // 스트림을 열어 드라이버가 버퍼 큐에서 버퍼를 받아오기 시작함.
    xioctl(fd, VIDIOC_STREAMON, &type); 
}

static void stop_capturing(int fd)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // 스트림을 닫는다.
    xioctl(fd, VIDIOC_STREAMOFF, &type);
}

static int read_frame(int fd, unsigned char *buffer)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    // 버퍼 큐에서 하나의 버퍼를 디큐
    xioctl(fd, VIDIOC_DQBUF, &buf);

    // 디큐한 버퍼를 복사
    memcpy(buffer, buffers[buf.index].start, buf.bytesused);

    // 디큐한 버퍼를 다시 재사용 가능하게 re 큐
    xioctl(fd, VIDIOC_QBUF, &buf);

    return buf.bytesused;
}

static void mainloop(int fd)
{
    const int frame_size = WIDTH * HEIGHT * 2; // YUYV 포맷: 2바이트 per 픽셀

    unsigned char *frame_buffer = malloc(frame_size);
    if(!frame_buffer) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for(int frame_count = 0; frame_count < 10; frame_count++) {
        fd_set fds;
        struct timeval tv;

        FD_ZERO(&fds);
        FD_SET(fd, &fds); // camfd를 파일 디스크립터 집합에 추가

        // Timeout 설정
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        // 비디오 데이터가 올 때까지 대기
        int r = select(fd + 1, &fds, NULL, NULL, &tv);
        if(-1 == r) {
            if(EINTR == errno)
                continue;
            perror("select");
            free(frame_buffer);
            exit(EXIT_FAILURE);
        }

        if(0 == r) {
            fprintf(stderr, "select timeout\n");
            free(frame_buffer);
            exit(EXIT_FAILURE);
        }

        int bytes = read_frame(fd, frame_buffer);

        if(bytes > 0) {
            // 프레임 데이터를 파일로 저장
            char filename[20];
            snprintf(filename, sizeof(filename), "frame%03d.yuv", frame_count);
            FILE *fp = fopen(filename, "wb");
            if(!fp) {
                perror("fopen");
                free(frame_buffer);
                exit(EXIT_FAILURE);
            }
            size_t written = fwrite(frame_buffer, 1, bytes, fp);
            if(written != bytes) {
                perror("fwrite");
                fclose(fp);
                free(frame_buffer);
                exit(EXIT_FAILURE);
            }
            fclose(fp);
            printf("Saved frame to %s\n", filename);
        }
    }

    free(frame_buffer);
}

