#include <stdio.h>
#include "bmpHeader.h"  // BMP 헤더 구조체 정의를 포함

/**
 * BMP 파일을 읽고 이미지 데이터를 반환하는 함수.
 * @param filename 읽을 BMP 파일의 경로
 * @param data 이미지 데이터를 저장할 포인터
 * @param cols 이미지의 열(가로) 크기를 저장할 포인터
 * @param rows 이미지의 행(세로) 크기를 저장할 포인터
 * @return 성공 시 0, 실패 시 -1
 */
int readBmp(char *filename, unsigned char **data, int *cols, int *rows) {
    BITMAPFILEHEADER bmpHeader;      // BMP 파일 헤더 구조체
    BITMAPINFOHEADER bmpInfoHeader;  // BMP 정보 헤더 구조체
    FILE *fp;                        // 파일 포인터

    // BMP 파일을 바이너리 모드로 엽니다.
    fp = fopen(filename, "rb");
    if(fp == NULL) {  // 파일 열기 실패 시
        perror("fopen()");  // 오류 메시지를 출력
        return -1;  // 오류 코드 반환
    }

    // BMP 파일 헤더 읽기
    fread(&bmpHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    // BMP 정보 헤더 읽기
    fread(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    // BMP 파일이 24비트 색상 형식인지 확인
    if(bmpInfoHeader.biBitCount != 24) {
        perror("This image file doesn't supports 24bit color\n");
        fclose(fp);  // 파일 닫기
        return -1;  // 오류 코드 반환
    }

    // 열과 행 크기 저장
    *cols = bmpInfoHeader.biWidth;  // 이미지의 가로 크기
    *rows = bmpInfoHeader.biHeight;  // 이미지의 세로 크기

    // 이미지 해상도와 비트 수 출력
    printf("Resolution : %d x %d\n", bmpInfoHeader.biWidth, bmpInfoHeader.biHeight);
    printf("Bit Count : %d\n", bmpInfoHeader.biBitCount);

    // BMP 데이터 섹션으로 파일 포인터 이동
    fseek(fp, bmpHeader.bfOffBits, SEEK_SET);
    // 이미지 데이터를 메모리에 읽기
    fread(*data, 1, bmpHeader.bfSize - bmpHeader.bfOffBits, fp);

    fclose(fp);  // 파일 닫기

    return 0;  // 성공 코드 반환
}
