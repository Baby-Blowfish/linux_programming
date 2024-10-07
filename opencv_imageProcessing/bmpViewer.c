#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "bmpHeader.h"  // BMP 파일을 처리하는 헤더파일 포함

#define FBDEVFILE "/dev/fb0"  // 프레임버퍼 장치 파일
#define LIMIT_UBYTE(n) (n>UCHAR_MAX)?UCHAR_MAX:(n<0)?0:n  // ubyte 제한 매크로

typedef unsigned char ubyte;   // 1바이트 크기의 부호 없는 정수형 정의
typedef unsigned short u2byte; // 2바이트 크기의 부호 없는 정수형 정의

// BMP 파일을 읽어들일 함수의 프로토타입 선언 (다른 파일에 정의됨)
extern int readBmp(char * filename, ubyte **pData, int *cols, int *rows);

// RGB 값을 받아 16비트 픽셀로 변환하는 함수
unsigned short makepixel(unsigned char r, unsigned char g, unsigned char b)
{
    // 5비트 R, 6비트 G, 5비트 B로 변환하여 16비트 픽셀 값을 반환
    return (unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
}

int main(int argc, char **argv)
{
    int cols, rows;           // BMP 이미지의 가로, 세로 크기 저장 변수
    ubyte r, g, b, a = 255;   // 픽셀의 R, G, B 및 알파 값(사용 안 함)
    ubyte *pData;             // BMP 파일로부터 읽어들인 픽셀 데이터를 저장할 포인터
    
    u2byte *pFbMap, *pBmpData;        // 프레임버퍼와 BMP 데이터를 저장할 포인터
    struct fb_var_screeninfo vinfo;   // 프레임버퍼의 화면 정보 구조체
    int fbfd;                // 프레임버퍼 장치 파일 디스크립터

    // 입력 파일(BMP 파일)을 지정하지 않았을 경우 사용법 출력 후 종료
    if(argc != 2)
    {
        printf("Usage : ./%s xxx.bmp\n", argv[0]);
        return -1;
    }

    // 프레임버퍼 장치를 읽기-쓰기 모드로 염
    fbfd = open(FBDEVFILE, O_RDWR);
    if(fbfd < 0)
    {
        perror("open()");  // 파일 열기에 실패하면 에러 메시지 출력
        return -1;
    }

    // 프레임버퍼의 화면 정보를 가져옴
    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        perror("ioctl() : FBIOGE_VSCREENINFO");  // ioctl 호출 실패 시 에러 메시지
        return -1;
    }

    int color = vinfo.bits_per_pixel / 8;   // 프레임버퍼의 각 픽셀당 바이트 수 계산
    
    // 프레임버퍼에 저장할 BMP 데이터를 위한 메모리 할당
    pBmpData = (u2byte *)malloc(vinfo.xres * vinfo.yres * color);
    
    // BMP 파일 데이터를 저장할 메모리 할당
    pData = (ubyte *)malloc(vinfo.xres * vinfo.yres * (24 / 8));
    
    // 프레임버퍼 메모리를 맵핑하여 접근할 수 있도록 설정
    pFbMap = (u2byte *)mmap(0, vinfo.xres * vinfo.yres * color, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    
    // mmap 실패 시 에러 메시지 출력
    if(pFbMap == (u2byte*)-1)
    {
        perror("mmap()");
        return -1;
    }

    // BMP 파일을 읽어 pData에 저장하고 이미지의 크기를 cols, rows에 저장
    if(readBmp(argv[1], &pData, &cols, &rows) < 0)
    {
        perror("readBmp()");  // BMP 파일 읽기 실패 시 에러 메시지 출력
        return -1;
    }

    // BMP 파일의 각 픽셀 데이터를 프레임버퍼에 복사하는 루프
    for(int y = 0, k, total_y; y < rows; y++)
    {
        // BMP 파일은 y축이 역순으로 저장되므로 마지막 행부터 시작
        k = (rows - y - 1) * cols * (24 / 8);
        total_y = y * cols * (24 / 8);
        
        // 각 행의 픽셀 데이터를 읽어와서 처리
        for(int x = 0; x < cols; x++)
        {
            // BMP 파일에서 픽셀의 R, G, B 값을 읽어옴
            b = LIMIT_UBYTE(pData[x * (24 / 8) + k + 0] + 50);  // 블루 채널
            g = LIMIT_UBYTE(pData[x * (24 / 8) + k + 1] + 50);  // 그린 채널
            r = LIMIT_UBYTE(pData[x * (24 / 8) + k + 2] + 50);  // 레드 채널
            
            // 픽셀을 회색으로 변환하는 코드 (주석 처리됨)
            /* 
            gray = (r * 0.3F) + (g * 0.58F) + (b * 0.11F); 
            histogram[(unsigned char)(gray)] += 1;
            */
            
            // RGB 값을 16비트로 변환
            unsigned short pixel;
            pixel = makepixel(r, g, b);
            
            // 프레임버퍼 데이터에 픽셀 값을 저장
            *(pBmpData + (x + y * vinfo.xres)) = pixel;
        }
    }

    // BMP 데이터를 프레임버퍼 메모리로 복사하여 화면에 출력
    memcpy(pFbMap, pBmpData, vinfo.xres * vinfo.yres * color);

    // 메모리 맵핑 해제
    munmap(pFbMap, vinfo.xres * vinfo.yres * color);
    
    // 할당된 메모리 해제
    free(pBmpData);
    free(pData);

    // 프레임버퍼 파일 닫기
    close(fbfd);

    return 0;
}
