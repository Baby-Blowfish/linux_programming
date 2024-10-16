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
typedef unsigned short u2byte;  // unsigned short에 대한 별칭 정의

extern int readBmp(char *filename, ubyte **pData, int *cols, int *rows);  // BMP 파일을 읽는 외부 함수 선언

// RGB 값을 16비트 픽셀 형식으로 변환하는 함수 (5-6-5 비트 구조로 변환)
unsigned short makepixel(unsigned char r, unsigned char g, unsigned char b)
{
    return (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));  // RGB 값을 5비트, 6비트, 5비트로 변환
}

int main(int argc, char **argv)
{
    int cols, rows;  // BMP 이미지의 가로(cols)와 세로(rows) 크기
    ubyte r, g, b, a = 255;  // RGB와 알파 값을 저장할 변수
    ubyte *pData = NULL;  // BMP 이미지의 픽셀 데이터를 저장할 포인터
    u2byte *pFbMap = NULL, *pBmpData = NULL;  // 프레임버퍼 메모리 맵핑 포인터와 BMP 데이터를 위한 포인터
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

    int color = vinfo.bits_per_pixel / 8.;  // 프레임버퍼의 픽셀당 바이트 수

    // BMP 파일에서 이미지 데이터를 읽어오는 함수 호출
    if (readBmp(argv[1], &pData, &cols, &rows) < 0)
    {
        perror("readBmp()");
        close(fbfd);
        return -1;
    }

    // BMP 데이터를 저장할 메모리 할당 (BMP 이미지 크기 기준)
    pBmpData = (u2byte *)malloc(cols * rows * color);
    if (pBmpData == NULL)
    {
        perror("Failed to allocate memory for BMP data");
        free(pData);
        close(fbfd);
        return -1;
    }

    // 프레임버퍼 메모리 맵핑
    pFbMap = (u2byte *)mmap(0, vinfo.xres * vinfo.yres * color, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (pFbMap == (u2byte *)-1)
    {
        perror("mmap()");
        free(pBmpData);
        free(pData);
        close(fbfd);
        return -1;
    }

    // BMP 데이터를 프레임버퍼로 변환하여 복사
    for (int y = 0, k, total_y; y < rows && y < vinfo.yres; y++)  // BMP 이미지 크기와 프레임버퍼 크기를 비교하여 제한
    {
        k = (rows - y - 1) * cols * (24 / 8);  // BMP 파일에서 아래쪽 픽셀부터 읽기 위해 역순으로 처리
        total_y = y * cols * (24 / 8);  // 현재 y축의 시작 위치 계산

        for (int x = 0; x < cols && x < vinfo.xres; x++)  // 가로 방향에서도 프레임버퍼 크기만큼 제한
        {
            // RGB 값을 BMP 데이터에서 가져와서 클램핑 후 픽셀값으로 변환
            b = LIMIT_UBYTE(pData[x * (24 / 8) + k + 0]);  // 파란색 값
            g = LIMIT_UBYTE(pData[x * (24 / 8) + k + 1]);  // 초록색 값
            r = LIMIT_UBYTE(pData[x * (24 / 8) + k + 2]);  // 빨간색 값

            unsigned short pixel;
            pixel = makepixel(r, g, b);  // 픽셀 값을 16비트 형식으로 변환
            *(pBmpData + (x + y * vinfo.xres)) = pixel;  // BMP 데이터를 메모리에 저장
        }
    }

    // 변환한 BMP 데이터를 프레임버퍼로 복사
    memcpy(pFbMap, pBmpData, vinfo.xres * vinfo.yres * color);

    // 메모리 맵핑 해제 및 동적 할당 메모리 해제
    munmap(pFbMap, vinfo.xres * vinfo.yres * color);
    free(pBmpData);
    free(pData);
    close(fbfd);  // 프레임버퍼 파일 디스크립터 닫기

    return 0;
}
