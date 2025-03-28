#ifndef __BMP_FILE_H__  // 헤더 파일 중복 포함을 방지하기 위한 매크로 정의 시작
#define __BMP_FILE_H__

// BMP 파일 헤더 구조체 (BITMAPFILEHEADER)
// __attribute__((__packed__))을 사용하여 패딩 없이 정확하게 메모리에 배치되도록 함
typedef struct __attribute__((__packed__)) 
{
    unsigned short bfType;         // 파일의 종류를 나타내는 필드 (고정 값: "BM" = 0x4D42)
    unsigned int bfSize;           // 파일의 크기를 바이트 단위로 저장
    unsigned short bfReserved1;    // 예약 필드 (항상 0)
    unsigned short bfReserved2;    // 예약 필드 (항상 0)
    unsigned int bfOffBits;        // 실제 픽셀 데이터의 시작 위치 (파일의 시작부터 바이트 오프셋)
} BITMAPFILEHEADER;

// BMP 정보 헤더 구조체 (BITMAPINFOHEADER)
// 이미지의 해상도, 색상 정보 및 압축 방식 등 이미지에 대한 메타데이터를 저장
typedef struct {
    unsigned int biSize;           // 이 구조체의 크기 (헤더의 크기 = 40 bytes)
    unsigned int biWidth;          // 이미지의 가로 크기 (픽셀 단위)
    unsigned int biHeight;         // 이미지의 세로 크기 (픽셀 단위)
    unsigned short biPlanes;       // 사용하는 색상 평면 수 (항상 1로 설정)
    unsigned short biBitCount;     // 픽셀당 비트 수 (1, 4, 8, 16, 24, 32 가능)
    unsigned int biCompresion;     // 압축 방식 (0: 압축 없음, 1: RLE 8비트, 2: RLE 4비트 등)
    unsigned int SizeImage;        // 이미지 데이터의 크기 (압축된 경우 압축 크기)
    unsigned int biXPelsPerMeter;  // 가로 방향 해상도 (미터당 픽셀 수)
    unsigned int biYPelsPerMeter;  // 세로 방향 해상도 (미터당 픽셀 수)
    unsigned int biClrUsed;        // 색상 테이블에서 실제로 사용되는 색상의 수
    unsigned int biClrImportant;   // 중요한 색상 수 (0이면 모든 색상이 중요함)
} BITMAPINFOHEADER;

// BMP 색상 팔레트 구조체 (RGBQUAD)
// 색상 정보 (RGB 값)를 저장하며, 픽셀 데이터를 색상 테이블로 사용할 때 사용
typedef struct
{
    unsigned char rgbBlue;   // 파란색 값 (0~255)
    unsigned char rgbGreen;  // 녹색 값 (0~255)
    unsigned char rgbRed;    // 빨간색 값 (0~255)
    unsigned char rgbReserved; // 예약 필드 (알파 채널이나 미사용, 항상 0)
} RGBQUAD;

#endif  // 헤더 파일 중복 포함 방지를 위한 매크로 정의 끝
