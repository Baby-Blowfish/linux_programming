#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "bmpHeader.h"  // BMP 파일 헤더를 처리하기 위한 사용자 정의 헤더 파일

#define FBDEVFILE "/dev/fb0"  // 프레임버퍼 장치 파일 경로 정의
#define LIMIT_UBYTE(n) (n > UCHAR_MAX) ? UCHAR_MAX : (n < 0) ? 0 : n  // UCHAR_MAX를 넘거나 0보다 작은 값 처리

typedef unsigned char ubyte;  // unsigned char에 대한 별칭 정의

extern int readBmpHeader(const char *filename, int *cols, int *rows, int *depth_bmp);  // BMP 헤더만 읽는 함수 선언
extern int readBmpData(const char *filename, unsigned char **pData, int imageSize);  // BMP 데이터만 읽는 함수 선언


int main(int argc, char **argv)
{
    int cols, rows, depth_bmp;  // BMP 이미지의 가로(cols)와 세로(rows) 크기, depth
    ubyte r, g, b, a = 255;  // RGB와 알파 값을 저장할 변수
    ubyte *pData = NULL;  // BMP 이미지의 픽셀 데이터를 저장할 포인터
    ubyte *pFbMap = NULL, *pBmpData = NULL;  // 프레임버퍼 메모리 맵핑 포인터와 BMP 데이터를 위한 포인터
    struct fb_var_screeninfo vinfo;  // 프레임버퍼의 가변 화면 정보 구조체
    int fbfd;  // 프레임버퍼 파일 디스크립터

    // 실행 시 파일명이 주어졌는지 확인
    if (argc != 2)
    {
        printf("Usage: ./%s xxx.bmp\n", argv[0]);
        return -1;
    }

    // 프레임버퍼 장치를 읽고 쓰기 모드로 열기
    fbfd = open(FBDEVFILE, O_RDWR);
    if (fbfd < 0)
    {
        perror("open()");
        return -1;
    }

    // 프레임버퍼의 화면 정보를 가져오기 위한 IOCTL 호출
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        perror("ioctl() : FBIOGET_VSCREENINFO");
        close(fbfd);
        return -1;
    }

    int depth_fb = vinfo.bits_per_pixel / 8.;  // 프레임버퍼의 픽셀당 바이트 수 : 4byte

    // 1. BMP 헤더 정보를 먼저 읽어서 cols, rows, depth_bmp 값을 얻음
    if (readBmpHeader(argv[1], &cols, &rows, &depth_bmp) < 0)
    {
        perror("readBmpHeader()");
        close(fbfd);
        return -1;
    }

    // 이미지 크기 계산 (cols * rows * 픽셀당 바이트 수)
    int imageSize = cols * rows * depth_bmp;

    // 2. BMP 데이터를 저장할 메모리 동적 할당
    pData = (ubyte *)malloc(imageSize);
    if (pData == NULL)
    {
        perror("Failed to allocate memory for BMP data");
        free(pData);
        close(fbfd);
        return -1;
    }

    // 3. BMP 데이터를 읽어오기
    if (readBmpData(argv[1], &pData, imageSize) < 0)
    {
        perror("readBmpData()");
        free(pData);
        close(fbfd);
        return -1;
    }

    // BMP 데이터를 프레임버퍼로 바꿀 메모리 할당
    pBmpData = (ubyte *)malloc(vinfo.xres * vinfo.yres * depth_fb);
	if (pBmpData == NULL)
    {
        perror("Failed to allocate memory for pBmpData data");
        free(pData);
        free(pBmpData);
        close(fbfd);
        return -1;
    }

    // 프레임버퍼 메모리 맵핑
    pFbMap = (ubyte *)mmap(0, vinfo.xres * vinfo.yres * depth_fb, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if (pFbMap == (ubyte *)-1)
    {
        perror("mmap()");
        free(pData);
        free(pBmpData);
        free(pFbMap);
        close(fbfd);
        return -1;
    }
printf("%d %s\n",__LINE__,__func__);
    // BMP 데이터를 프레임버퍼의 포멧으로 변환하여 복사
    for (int y = 0, k, total_y; y < rows; y++)  // BMP 이미지 크기와 프레임버퍼 크기를 비교하여 제한
    {
        k = (rows - y - 1) * cols * depth_bmp;  // BMP 파일에서 아래쪽 픽셀부터 읽기 위해 역순으로 처리
        total_y = y*vinfo.xres * depth_fb;  // 현재 y축의 시작 위치 계산

        for (int x = 0; x < cols; x++)  // 가로 방향에서도 프레임버퍼 크기만큼 제한
        {
            // RGB 값을 BMP 데이터에서 가져와서 클램핑 후 픽셀값으로 변환
            b = LIMIT_UBYTE(pData[k + x*depth_bmp + 0]);  // 파란색 값
            g = LIMIT_UBYTE(pData[k + x*depth_bmp + 1]);  // 초록색 값
            r = LIMIT_UBYTE(pData[k + x*depth_bmp + 2]);  // 빨간색 값

            *(pBmpData + x*depth_fb + total_y + 0) = b;
            *(pBmpData + x*depth_fb + total_y + 1) = g;  
            *(pBmpData + x*depth_fb + total_y + 2) = r;  
            *(pBmpData + x*depth_fb + total_y + 3) = a;  
        }
    }

printf("%d %s\n",__LINE__,__func__);
    // 변환한 BMP 데이터를 프레임버퍼로 복사
    memcpy(pFbMap, pBmpData, vinfo.xres * vinfo.yres * depth_fb);

printf("%d %s\n",__LINE__,__func__);
    // 메모리 맵핑 해제 및 동적 할당 메모리 해제
    munmap(pFbMap, vinfo.xres * vinfo.yres * depth_fb);
    free(pBmpData);
    free(pData);
    close(fbfd);  // 프레임버퍼 파일 디스크립터 닫기

    return 0;
}
