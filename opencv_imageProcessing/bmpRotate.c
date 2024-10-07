/*
	gcc -o bmpRotate bmpRotate.c -lm
	./bmpRotate mandrill.bmp rotate.bmp  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "bmpHeader.h"

#define LIMIT_UBYTE(n) ((n)>UCHAR_MAX)?UCHAR_MAX:((n)<0)? 0:(n)

typedef unsigned char ubyte;

#define widthbytes(bits)   (((bits)+31)/32*4)

#ifndef M_PI
#define M_PI	3.141592654
#endif


int main(int argc, char** argv)
{
	FILE *fp;
	BITMAPFILEHEADER bmpHeader;
	BITMAPINFOHEADER bmpInfoHeader;
	RGBQUAD palrgb[256];
	ubyte *inimg, *outimg;

	int imageSize = 0;
	char input[128], output[128];
	int i, j, size, index;
	double radius, cos_value, sin_value;
	int centerX, centerY;
	int degree = 45;

	if(argc != 3)
	{
		fprintf(stderr, "usage : %s input.bmp output.bmp\n",argv[0]);
		return -1;
	}


	if((fp = fopen(argv[1], "rb")) == NULL){
			fprintf(stderr, "Error : Failde to open file...\n");
			return -1;
	}

	fread(&bmpHeader, sizeof(BITMAPFILEHEADER), 1, fp);
	fread(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

	if(bmpInfoHeader.biBitCount != 24)
	{
		perror("this image file doesn't support 24bit color\n");
		fclose(fp);
		return -1;
	}

	int elemSize = bmpInfoHeader.biBitCount / 8;
	
	int height = bmpInfoHeader.biHeight;
	int width = bmpInfoHeader.biWidth;
	
	size = widthbytes(width*bmpInfoHeader.biBitCount);
	imageSize = height * size;
	

	printf("Resolution : %d x %d \n", bmpInfoHeader.biWidth, bmpInfoHeader.biHeight);
	printf("Bit Count : %d(%d)\n",bmpInfoHeader.biBitCount,elemSize);
	printf("Image Size : %d\n", imageSize);

	inimg = (ubyte*)malloc(sizeof(ubyte) * imageSize);
	outimg = (ubyte*)malloc(sizeof(ubyte) * imageSize);

	fread(inimg, sizeof(ubyte), imageSize, fp);
	fclose(fp);
	
	radius = degree*(M_PI/180.0f);
	sin_value = sin(radius);
	cos_value = cos(radius);
	centerX = height/2;
	centerY = width/2;
	
	for(i = 0 ; i < height; i++) { 
		index = (height-i-1) * size; 
		for(j = 0 ; j < width; j++) { 
			double new_x;
			double new_y;
			new_x = (i-centerX)*cos_value - (j-centerY)*sin_value + centerX;
			new_y = (i-centerX)*sin_value + (j-centerY)*cos_value + centerY; 
			
			if(new_x <0 || new_x > height) {
				outimg[index+3*j+0] = 0;
				outimg[index+3*j+1] = 0;
				outimg[index+3*j+2] = 0;
			} else if (new_y <0 || new_y > width) {
				outimg[index+3*j+0] = 0;
				outimg[index+3*j+1] = 0;
				outimg[index+3*j+2] = 0;
			} else {
				outimg[index+3*j+0] = inimg[(int)(height-new_x-1)*size+(int)new_y*3+0];
				outimg[index+3*j+1] = inimg[(int)(height-new_x-1)*size+(int)new_y*3+1];
				outimg[index+3*j+2] = inimg[(int)(height-new_x-1)*size+(int)new_y*3+2];
			}
		};
	};

	bmpHeader.bfOffBits += 256*sizeof(RGBQUAD); 
	
	if((fp = fopen(argv[2], "wb")) == NULL) {
		fprintf(stderr, "Error : Failed to open file...\n");
		return -1;
	}

 

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
	fwrite(outimg, sizeof(ubyte), imageSize, fp);
	 
	free(inimg);
	free(outimg);
	
	fclose(fp);
	
	return -1;
}
