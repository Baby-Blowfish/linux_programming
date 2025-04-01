#include <stdio.h>  // 표준 입출력 함수 사용을 위한 헤더 파일 포함
#include "bmpHeader.h"  // 사용자 정의 BMP 헤더 파일을 포함 (구조체 정의 포함)

// Bmp 파일 헤더 정보
int readBmpHeader(const char * filename, int *cols, int *rows, int *depth_bmp)
{
    BITMAPFILEHEADER bmpHeader;  // BMP 파일 헤더 구조체 선언 (파일 메타데이터 저장)
    BITMAPINFOHEADER bmpInfoHeader;  // BMP 정보 헤더 구조체 선언 (이미지 정보 저장)
    FILE *fp;  // 파일 포인터 선언 (이미지 파일을 읽기 위한 변수)

    // BMP 파일을 읽기 모드("rb")로 열기 (이진 파일로 열림)
    fp = fopen(filename, "rb");
    if(fp == NULL)
    {
        // 파일 열기에 실패한 경우 에러 메시지 출력
        perror("fopen()");
        return -1;  // 실패 시 -1 반환
    }

    // BMP 파일 헤더를 읽어와서 bmpHeader 구조체에 저장 (파일의 메타데이터)
    fread(&bmpHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    // BMP 정보 헤더를 읽어와서 bmpInfoHeader 구조체에 저장 (이미지의 크기, 색상 정보 등)
    fread(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    // BMP 파일이 24비트 컬러가 아닌 경우 에러 처리 (24비트만 지원)
    if(bmpInfoHeader.biBitCount != 24)
    {
        // 에러 메시지 출력 후 파일 닫기
        fprintf(stderr, "This image file doesn't support 24-bit color\n");
        fclose(fp);
        return -1;  // 실패 시 -1 반환
    }

    // BMP 파일의 Depth를 포인터에 저장
    *depth_bmp = bmpInfoHeader.biBitCount/8;

    // 이미지의 가로, 세로 크기를 전달받은 포인터에 저장
    *cols = bmpInfoHeader.biWidth;   // 이미지의 가로 크기
    *rows = bmpInfoHeader.biHeight;  // 이미지의 세로 크기

    // 이미지의 해상도(가로, 세로)와 비트 수 출력
    printf("Resolution : %d x %d\n", bmpInfoHeader.biWidth, bmpInfoHeader.biHeight);
    printf("Bit Count : %d\n", bmpInfoHeader.biBitCount);

    printf("%d %s\n",__LINE__,__func__);
    // 파일 닫기 (더 이상 파일 작업이 필요 없으므로)
    fclose(fp);

    return 0;
}

// Bmp 파일 데이터 읽기
int readBmpData(const char *filename, unsigned char **pData, int imageSize)
{
    BITMAPFILEHEADER bmpHeader;  // BMP 파일 헤더 구조체 선언 (파일 메타데이터 저장)
    BITMAPINFOHEADER bmpInfoHeader;  // BMP 정보 헤더 구조체 선언 (이미지 정보 저장)

    FILE *fp;  // 파일 포인터 선언 (이미지 파일을 읽기 위한 변수)

    // BMP 파일을 읽기 모드("rb")로 열기 (이진 파일로 열림)
    fp = fopen(filename, "rb");
    if(fp == NULL)
    {
        // 파일 열기에 실패한 경우 에러 메시지 출력
        perror("fopen()");
        return -1;  // 실패 시 -1 반환
    }

    // BMP 파일 헤더를 읽어와서 bmpHeader 구조체에 저장 (파일의 메타데이터)
    fread(&bmpHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    // BMP 정보 헤더를 읽어와서 bmpInfoHeader 구조체에 저장 (이미지의 크기, 색상 정보 등)
    fread(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    // BMP 파일이 24비트 컬러가 아닌 경우 에러 처리 (24비트만 지원)
    if(bmpInfoHeader.biBitCount != 24)
    {
        // 에러 메시지 출력 후 파일 닫기
        fprintf(stderr, "This image file doesn't support 24-bit color\n");
        fclose(fp);
        return -1;  // 실패 시 -1 반환
    }

    // 파일 포인터를 픽셀 데이터의 시작 위치로 이동
    fseek(fp, bmpHeader.bfOffBits, SEEK_SET);

    // 픽셀 데이터를 읽어와서 data에 저장 (이미지 데이터만 읽음)
    fread(*pData, 1, imageSize, fp);

    printf("%d %s\n",__LINE__,__func__);
    // 파일 닫기 (더 이상 파일 작업이 필요 없으므로)
    fclose(fp);

    return 0;
}
