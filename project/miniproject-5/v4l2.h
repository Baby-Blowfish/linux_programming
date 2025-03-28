#ifndef __V4L2_H__
#define __V4L2_H__
#include "Common.h"

/* Video4Linux2에서 사용할 영상 데이터를 저장할 버퍼 구조체 */
struct buffer {
    void * start;    /* 버퍼 시작 주소 */
    size_t length;   /* 버퍼 길이 */
};

typedef unsigned char ubyte;  // unsigned char에 대한 별칭 정의
extern struct buffer *buffers;          /* 비디오 캡처용 버퍼 */
extern unsigned int n_buffers;      /* 버퍼 개수 */

/* 캡처 시작을 위한 함수 */
void start_capturing(int fd);

/* 메모리 맵핑 방식의 초기화 함수 */
void init_mmap(int fd);

/* V4L2 장치 초기화 함수 */
void init_v4l2(int fd);

#endif