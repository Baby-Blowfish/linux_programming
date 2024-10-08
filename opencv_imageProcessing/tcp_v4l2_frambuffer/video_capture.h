#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> 
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/videodev2.h>

#define FBDEV        "/dev/fb0"      // 프레임버퍼 디바이스 경로
#define VIDEODEV     "/dev/video0"   // 비디오 디바이스 경로
#define WIDTH        800             // 화면의 너비
#define HEIGHT       600             // 화면의 높이

/* Video4Linux에서 사용할 영상 저장을 위한 버퍼 구조체 */
struct buffer {
    void * start;    // 버퍼의 시작 주소
    size_t length;   // 버퍼의 크기
};

/* 외부에서 참조할 전역 변수들 */
extern short *fbp;                 // 프레임버퍼의 포인터 (화면 출력용)
extern struct buffer *buffers;     // 비디오 장치에서 사용할 메모리 매핑된 버퍼 배열
extern unsigned int n_buffers;     // 매핑된 버퍼의 개수
extern struct fb_var_screeninfo vinfo;  // 프레임버퍼의 화면 정보
extern int camfd;		/* 카메라의 파일 디스크립터 */


/* 함수 선언 */

/**
 * @brief 오류 메시지를 출력하고 프로그램을 종료합니다.
 * 
 * @param s 오류 메시지
 */
void mesg_exit(const char *s);

/**
 * @brief ioctl 시스템 호출을 처리하는 함수입니다.
 * 
 * @param fd 파일 디스크립터
 * @param request ioctl 명령 요청
 * @param arg 명령에 전달할 인자
 * @return 성공 시 0, 실패 시 -1을 반환합니다.
 */
int xioctl(int fd, int request, void *arg);

/**
 * @brief 주어진 값이 특정 범위 내에 있는지 확인하고, 범위를 초과하면 경계 값으로 클리핑합니다.
 * 
 * @param value 클리핑할 값
 * @param min 최소 값
 * @param max 최대 값
 * @return 클리핑된 값
 */
int clip(int value, int min, int max);

/**
 * @brief YUYV 포맷의 영상을 처리하여 프레임버퍼에 출력합니다.
 * 
 * @param p 처리할 영상 데이터의 시작 주소
 */
void process_image(const void *p);

/**
 * @brief 프레임을 읽어 들이고 처리합니다.
 * 
 * @param fd 비디오 장치의 파일 디스크립터
 * @return 성공 시 1, 실패 시 0을 반환합니다.
 */
int read_frame(int fd);

/**
 * @brief 비디오 프레임을 반복적으로 읽어 처리하는 메인 루프입니다.
 * 
 * @param fd 비디오 장치의 파일 디스크립터
 */
void mainloop(int fd);

/**
 * @brief 비디오 장치에서 영상을 캡처하기 위한 준비 작업을 수행합니다.
 * 
 * @param fd 비디오 장치의 파일 디스크립터
 */
void start_capturing(int fd);

/**
 * @brief 비디오 장치의 메모리를 메모리 맵핑 방식으로 초기화합니다.
 * 
 * @param fd 비디오 장치의 파일 디스크립터
 */
void init_mmap(int fd);

/**
 * @brief 비디오 장치 초기화를 수행하고, 캡처 설정을 구성합니다.
 * 
 * @param fd 비디오 장치의 파일 디스크립터
 */
void init_device(int fd);

#endif /* VIDEO_CAPTURE_H */
