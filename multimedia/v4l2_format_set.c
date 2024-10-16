#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>

int main() {
    int fd = open("/dev/video0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open video device");
        return -1;
    }

    // 포맷 설정
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;  // 해상도: 640
    fmt.fmt.pix.height = 360; // 해상도: 360
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // 픽셀 포맷: YUYV
    fmt.fmt.pix.field = V4L2_FIELD_NONE;  // 프로그레시브 스캔 설정

    // 설정된 포맷 적용
    if (-1 == ioctl(fd, VIDIOC_S_FMT, &fmt)) {
        perror("Setting format failed");
        close(fd);
        return -1;
    }

    // 적용된 포맷 확인
    if (-1 == ioctl(fd, VIDIOC_G_FMT, &fmt)) {
        perror("Getting format failed");
        close(fd);
        return -1;
    }

    // 결과 출력
    printf("Format set: Width=%d, Height=%d, Pixel Format=%c%c%c%c\n",
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            fmt.fmt.pix.pixelformat & 0xFF,
            (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
            (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
            (fmt.fmt.pix.pixelformat >> 24) & 0xFF);

    close(fd);
    return 0;
}
