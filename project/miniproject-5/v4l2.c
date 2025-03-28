#include "v4l2.h"

struct buffer *buffers = NULL;          /* 비디오 캡처용 버퍼 */
unsigned int n_buffers = 0;      /* 버퍼 개수 */

/* 캡처 시작을 위한 함수 */
void start_capturing(int fd)
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
void init_mmap(int fd)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));  /* req 구조체의 모든 필드를 0으로 초기화 */
    req.count = 4;  /* 4개의 버퍼 요청. 즉, 4개의 프레임을 처리할 수 있도록 버퍼를 할당 */
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  /* 비디오 캡처를 위한 버퍼 타입 지정 */
    req.memory = V4L2_MEMORY_MMAP;  /* 메모리 매핑 방식(MMAP)을 사용하여 버퍼를 관리 */

    /* VIDIOC_REQBUFS를 사용해 메모리 매핑을 요청 */
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {  /* 버퍼 요청이 실패할 경우 */
        if (EINVAL == errno) {  /* 장치가 메모리 매핑을 지원하지 않는 경우 */
            fprintf(stderr, "%d does not support memory mapping\n", fd);
            exit(EXIT_FAILURE);  /* 메모리 매핑을 지원하지 않으면 프로그램 종료 */
        } else {
            mesg_exit("VIDIOC_REQBUFS");  /* 그 외의 다른 오류가 발생한 경우 */
        }
    }

    /* 요청한 버퍼 수가 너무 적으면 오류 처리 */
    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %d\n", fd);  /* 버퍼 메모리가 충분하지 않음을 알림 */
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
void init_v4l2(int fd)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    /* 장치 기능 조회 */
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%d is no V4L2 device\n", fd );
            exit(EXIT_FAILURE);
        } else {
            mesg_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%d is no video capture device\n", fd);
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%d does not support streaming i/o\n", fd);
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

