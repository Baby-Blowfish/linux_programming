#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#define FBDEVICE "/dev/fb0"

typedef unsigned char ubyte;

struct fb_var_screeninfo vinfo;

unsigned short makepixel(unsigned char r, unsigned char g, unsigned char b)
{
	return (unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
}

static void drawpoint(int fd, int x, int y, ubyte r, ubyte g, ubyte b)
{
	
	ubyte a = 0xff;
	unsigned short pixel;
	pixel = makepixel(r,g,b);
	int offset = (x+y*vinfo.xres)*vinfo.bits_per_pixel/8.;
	lseek(fd, offset, SEEK_SET);
	write(fd, &pixel, 2);
}

static void drawline(int fd, int start_x, int end_x, int y, ubyte r, ubyte g, ubyte b)
{
	ubyte a = 0xFF;

	for(int x = start_x; x <end_x; x++)
	{
		int offset = (x+y*vinfo.xres)*vinfo.bits_per_pixel/8.;
		unsigned short pixel;
		pixel = makepixel(r,g,b);
		lseek(fd, offset, SEEK_SET);
		write(fd,&pixel,2);
	}


}
// midpoint circle algorithm
static void drawcircle(int fd, int center_x, int center_y, int radius, ubyte r, ubyte g, ubyte b)
{
	int x = radius, y = 0;
	int radiusError = 1 - x;

	while(x >= y)
	{
		drawpoint(fd, x+center_x, y + center_y, r, g, b);
		drawpoint(fd, y+center_x, x + center_y, r, g, b);
		drawpoint(fd, -x+center_x, y + center_y, r, g, b);
		drawpoint(fd, -y+center_x, x + center_y, r, g, b);
		drawpoint(fd, -x+center_x, -y + center_y, r, g, b);
		drawpoint(fd, -y+center_x, -x + center_y, r, g, b);
		drawpoint(fd, x+center_x, -y + center_y, r, g, b);
		drawpoint(fd, y+center_x, -x + center_y, r, g, b);
		
		y++;
		if(radiusError < 0 )
		{
			radiusError += 2*y + 1;
		}else 
		{
			x--;
			radiusError += 2*(y-x+1);
		}

	}
}

static void drawfacemmap(int fd, int start_x, int start_y, int end_x, int end_y, ubyte r, ubyte g, ubyte b)
{
	unsigned short *pfb;
	int color = vinfo.bits_per_pixel/8.;	// bytes_per_pixel

	if(end_x == 0) end_x = vinfo.xres;
	if(end_y == 0) end_y = vinfo.yres;
	
	pfb = (unsigned short *)mmap(NULL,vinfo.xres*vinfo.yres*color,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

	// 오류 처리
    if (pfb == MAP_FAILED) {
        perror("Error: mmap failed");
        close(fd);  // fd를 닫고 종료
        return;
    }

	for(int x = start_x ; x< end_x; x++)
	{
		for(int y = start_y; y < end_y; y++)
		{
			unsigned short pixel;
			pixel = makepixel(r,g,b);
			//*(pfb + (x + y*vinfo.xres)) = pixel;
			pfb[x + y*vinfo.xres] = pixel;
		}
	}
	munmap(pfb, vinfo.xres * vinfo.yres * color);
}

static void drawface(int fd, int start_x, int start_y, int end_x, int end_y, ubyte r, ubyte g, ubyte b)
{
	ubyte a = 0xFF;

	if(end_x == 0) end_x = vinfo.xres;
	if(end_y == 0) end_y = vinfo.yres;
	
	for(int x = start_x ; x< end_x; x++)
	{
		for(int y = start_y; y < end_y; y++)
		{
			int offset = (x + y*vinfo.xres)*vinfo.bits_per_pixel/8.;
			unsigned short pixel;
			pixel = makepixel(r,g,b);
			lseek(fd, offset, SEEK_SET);
			write(fd,&pixel,2);
		}
	}
}


int main(int argc, char**argv)
{
	int fdfd, status, offset;

	unsigned short pixel;

	fdfd = open(FBDEVICE, O_RDWR);
	if(fdfd<0)
	{
		perror("Error: cannot open framebuffer device");
		return -1;
	}
	if(ioctl(fdfd,FBIOGET_VSCREENINFO,&vinfo) < 0)
	{
		perror("Error reading fixed information");
		return -1;
	}
//	drawpoint(fdfd,50,50,255,0,0);
//	drawpoint(fdfd,100,100,0,255,0);
//	drawpoint(fdfd,150,150,0,0,255);
//	drawline(fdfd, 0, 100, 200, 0, 255, 255);
//	drawcircle(fdfd, 200, 200, 100, 255, 0, 255);//magenta
//	drawcircle(fdfd, 200, 200, 200, 0, 255, 255);//cyan
//	drawface(fdfd, 0, 0, 0, 0, 255, 0, 0);
//	drawcircle(fdfd, 200, 200, 100,255, 0, 255);
//	drawline(fdfd, 0, 100,200, 0, 255, 255);
	
//	drawfacemmap(fdfd, 0, 0, 0, 0, 255,  , 0);
//	drawfacemmap(fdfd, 0, 0, 0, 0, 0, 255 , 0);
	drawfacemmap(fdfd, 0, 0, 533, 0, 0,0, 255); //blue
	drawfacemmap(fdfd, 534, 0, 1066, 0, 255,255, 255); //white
	drawfacemmap(fdfd,1066, 0, 0, 0, 255,0, 0); //red
//	drawflag(fdfd);

	close(fdfd);

	return 0;
}
