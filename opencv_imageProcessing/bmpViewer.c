#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "bmpHeader.h"

#define FBDEVFILE "/dev/fb0"
#define LIMIT_UBYTE(n) (n>UCHAR_MAX)?UCHAR_MAX:(n<0)?0:n

typedef unsigned char ubyte;
typedef unsigned short u2byte; 

extern int readBmp(char * filename, ubyte **pData, int *cols, int *rows);

unsigned short makepixel(unsigned char r, unsigned char g, unsigned char b)
{
	return (unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
}

int main(int argc, char **argv)
{
	int cols, rows;
	ubyte r, g, b, a = 255;
	ubyte *pData;
	
	u2byte *pFbMap,*pBmpData;
	struct fb_var_screeninfo vinfo;
	int fbfd;

	if(argc != 2)
	{
		printf("Usage : ./%s xxx.bmp\n", argv[0]);
		return -1;
	}

	fbfd = open(FBDEVFILE, O_RDWR);
	if(fbfd<0)
	{
		perror("open()");
		return -1;
	}

	if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0)
	{
		perror("ioctl() : FBIOGE_VSCREENINFO");
		return -1;
	}

	int color = vinfo.bits_per_pixel/8.;
	
	pBmpData = (u2byte *)malloc(vinfo.xres*vinfo.yres*color);
	
	pData = (ubyte *)malloc(vinfo.xres*vinfo.yres*(24/8));
	
	pFbMap = (u2byte *)mmap(0, vinfo.xres*vinfo.yres*color,PROT_READ|PROT_WRITE,MAP_SHARED,fbfd,0);
	
	if(pFbMap == (u2byte*)-1)
	{
		perror("mmap()");
		return -1;
	}

	if(readBmp(argv[1],&pData,&cols, &rows) < 0)
	{
		perror("readBmp()");
		return -1;
	}

	for(int y = 0, k, total_y; y < rows; y++)
	{
		k = (rows - y - 1)*cols*(24/8);
		total_y = y*cols*(24/8);
		
		for(int x = 0; x < cols; x++)
		{
			//b = LIMIT_UBYTE(pData[k+x*(24/8)+0]);
			//g = LIMIT_UBYTE(pData[k+x*(24/8)+1]);
			//r = LIMIT_UBYTE(pData[k+x*(24/8)+2]);
			
			// b = LIMIT_UBYTE(pData[x*(24/8) + total_y + 0]);

			// bmp is x-axis symmetry
			b = LIMIT_UBYTE(pData[x*(24/8) + k + 0] + 50);
			g = LIMIT_UBYTE(pData[x*(24/8) + k + 1] + 50);
			r = LIMIT_UBYTE(pData[x*(24/8) + k + 2] + 50);
			
			/*
			gray = (r*0.3F)+(g*0.58F)+(b*0.11F);
			histogram[(unsigned char)(gray)] += 1;
			   */
			unsigned short pixel;
			pixel = makepixel(r,g,b);
			*(pBmpData + (x + y*vinfo.xres)) = pixel;
		}
	}

	memcpy(pFbMap, pBmpData, vinfo.xres*vinfo.yres*color);

	munmap(pFbMap, vinfo.xres*vinfo.yres*color);
	free(pBmpData);
	free(pData);

	close(fbfd);

	return 0;
}


