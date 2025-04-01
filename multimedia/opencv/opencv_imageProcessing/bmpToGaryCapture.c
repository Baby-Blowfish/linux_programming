#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <limits.h>  /* USHRT_MAX 상수를 위해 사용한다. */

#include "bmpHeader.h"  /* BMP 파일과 관련된 구조체 정의 */

#define LIMIT_UBYTE(n) ((n)>UCHAR_MAX)?UCHAR_MAX:((n)<0)?0:(n)
/* 이미지 데이터의 경계 검사를 위한 매크로.
   값이 0보다 작거나 UCHAR_MAX보다 큰 경우 범위 내로 제한한다. */

typedef unsigned char ubyte;  /* 간결한 코드를 위해 unsigned char를 ubyte로 정의 */

/* main 함수 */
int main(int argc, char** argv) 
{
    FILE* fp;  /* 파일 포인터 */
    BITMAPFILEHEADER bmpHeader;  /* BMP 파일 헤더 구조체 */
    BITMAPINFOHEADER bmpInfoHeader;  /* BMP 정보 헤더 구조체 */
    RGBQUAD *palrgb;  /* 팔레트 (8비트 그레이스케일 이미지를 위한 색상 팔레트) */
    ubyte *inimg, *outimg;  /* 입력 및 출력 이미지 데이터 포인터 */
    int x, y, z, imageSize;  /* 이미지 처리에 필요한 변수들 */

    /* 프로그램 인자가 부족할 경우 사용법 출력 */
    if(argc != 3) {
        fprintf(stderr, "usage : %s input.bmp output.bmp\n", argv[0]);
        return -1;
    }
    
    /***** BMP 파일 읽기 *****/
    if((fp = fopen(argv[1], "rb")) == NULL) {  // 입력 파일을 바이너리 모드로 열기
        fprintf(stderr, "Error : Failed to open file...\n"); 
        return -1;
    }

    /* BMP 파일 헤더 읽기 */
    fread(&bmpHeader, sizeof(BITMAPFILEHEADER), 1, fp);

    /* BMP 정보 헤더 읽기 */
    fread(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    /* BMP 파일이 24비트 컬러 이미지를 지원하지 않으면 프로그램 종료 */
    if(bmpInfoHeader.biBitCount != 24) {
        perror("This image file doesn't support 24bit color\n");
        fclose(fp);
        return -1;
    }
    
    /* 각 픽셀당 바이트 수 계산 (24비트 컬러 -> 픽셀당 3바이트) */
    int elemSize = bmpInfoHeader.biBitCount / 8;  
    int size = bmpInfoHeader.biWidth * elemSize;  /* 이미지의 가로 크기 * 바이트 수 */
    imageSize = size * bmpInfoHeader.biHeight;  /* 전체 이미지의 크기 계산 */

    /* 이미지 해상도 및 관련 정보 출력 */
    printf("Resolution : %d x %d\n", bmpInfoHeader.biWidth, bmpInfoHeader.biHeight);
    printf("Bit Count : %d(%d)\n", bmpInfoHeader.biBitCount, elemSize);  /* 픽셀당 비트 수 */
    printf("Image Size : %d\n", imageSize);

    /* 입력 이미지 데이터를 저장할 메모리 할당 */
    inimg = (ubyte*)malloc(sizeof(ubyte) * imageSize); 

    /* 그레이스케일 이미지 출력을 위한 메모리 할당 (가로 * 세로 크기) */
    outimg = (ubyte*)malloc(sizeof(ubyte) * bmpInfoHeader.biWidth * bmpInfoHeader.biHeight);

    /* 입력 이미지 데이터를 읽어서 inimg에 저장 */
    fread(inimg, sizeof(ubyte), imageSize, fp); 
    
    fclose(fp);  /* 파일을 닫음 */
    
    /* 24비트 이미지를 그레이스케일로 변환하는 루프 */
    for(y = 0; y < bmpInfoHeader.biHeight; y++) {
        for(x = 0; x < size; x += elemSize) {
            /* B, G, R 값을 각각 추출 (픽셀은 BGR 순서로 저장됨) */
            ubyte b = inimg[x + y * size + 0];
            ubyte g = inimg[x + y * size + 1];
            ubyte r = inimg[x + y * size + 2];

            /* 그레이스케일 값 계산 (가중치 공식 사용) */
            outimg[x / elemSize + y * bmpInfoHeader.biWidth] = (r * 0.299F) + (g * 0.587F) + (b * 0.114F);
        }
    }
     
    /***** BMP 파일 쓰기 *****/
    if((fp = fopen(argv[2], "wb")) == NULL) {  // 출력 파일을 바이너리 모드로 열기
        fprintf(stderr, "Error : Failed to open file...\n"); 
        return -1;
    }

    /* 그레이스케일 이미지를 위한 팔레트 생성 (0~255의 회색조 값) */
    palrgb = (RGBQUAD*)malloc(sizeof(RGBQUAD) * 256);
    for(x = 0; x < 256; x++) {
        palrgb[x].rgbBlue = palrgb[x].rgbGreen = palrgb[x].rgbRed = x;  /* RGB 값 모두 동일하게 설정 (그레이스케일) */
        palrgb[x].rgbReserved = 0;
    }

    /* BMP 정보 헤더를 8비트로 설정 */
    bmpInfoHeader.biBitCount = 8;
    bmpInfoHeader.SizeImage = bmpInfoHeader.biWidth * bmpInfoHeader.biHeight * bmpInfoHeader.biBitCount / 8;
    bmpInfoHeader.biCompression = 0;
    bmpInfoHeader.biClrUsed = 0;
    bmpInfoHeader.biClrImportant = 0;

    /* 파일 헤더의 오프셋과 파일 크기 설정 */
    bmpHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256;
    bmpHeader.bfSize = bmpHeader.bfOffBits + bmpInfoHeader.SizeImage;

    /* BMP 파일 헤더 쓰기 */
    fwrite(&bmpHeader, sizeof(BITMAPFILEHEADER), 1, fp);

    /* BMP 정보 헤더 쓰기 */
    fwrite(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    /* 팔레트 데이터 쓰기 */
    fwrite(palrgb, sizeof(RGBQUAD), 256, fp); 

    /* 변환된 그레이스케일 이미지 데이터 쓰기 */
    fwrite(outimg, sizeof(ubyte), bmpInfoHeader.biWidth * bmpInfoHeader.biHeight, fp);

    fclose(fp);  /* 파일을 닫음 */
    
    /* 할당된 메모리 해제 */
    free(inimg); 
    free(outimg);
    
    return 0;  /* 프로그램 종료 */
}
