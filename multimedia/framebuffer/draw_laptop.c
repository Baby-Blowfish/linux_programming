#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define FBDEVICE "/dev/fb0"  // 프레임 버퍼 장치의 경로

typedef unsigned char ubyte;  // unsigned char 타입을 ubyte로 정의

// 프레임 버퍼 정보 처리를 위한 구조체
struct fb_var_screeninfo vinfo;  // 화면 정보 구조체

// 점을 그리는 함수
static void drawpoint(int fd, int x, int y, ubyte r, ubyte g, ubyte b)
{
    // 알파 값 정의 (불투명)
    ubyte a = 0xff;

    // 영상의 깊이 (바이트 단위)
    int depth = (vinfo.bits_per_pixel / 8);

    // x, y 좌표에 해당하는 픽셀 위치 계산
    int offset = (x + y * vinfo.xres) * depth;

    // 파일 포인터를 해당 오프셋으로 이동
    lseek(fd, offset, SEEK_SET);

    // BGRA 순서로 색상 데이터 작성
    write(fd, &b, 1);  // 파랑
    write(fd, &g, 1);  // 초록
    write(fd, &r, 1);  // 빨강
    write(fd, &a, 1);  // 알파 (투명도)
}

// 선을 그리는 함수
static void drawline(int fd, int start_x, int end_x, int y, ubyte r, ubyte g, ubyte b)
{
    ubyte a = 0xFF;  // 알파 값 (불투명)

    // 영상의 깊이 (바이트 단위)
    int depth = (vinfo.bits_per_pixel / 8);

    int offset = 0;

    // 주어진 x 좌표 범위 내에서 선을 그림
    for (int x = start_x; x < end_x; x++)
    {
        // 현재 x, y 좌표에 해당하는 픽셀 위치 계산
        offset = (x + y * vinfo.xres) * depth;

        // 파일 포인터를 해당 오프셋으로 이동
        lseek(fd, offset, SEEK_SET);

        // BGRA 순서로 색상 데이터 작성
        write(fd, &b, 1);  // 파랑
        write(fd, &g, 1);  // 초록
        write(fd, &r, 1);  // 빨강
        write(fd, &a, 1);  // 알파 (투명도)
    }
}

// 중점 원 알고리즘을 이용해 원을 그리는 함수
static void drawcircle(int fd, int center_x, int center_y, int radius, ubyte r, ubyte g, ubyte b)
{
    int x = radius, y = 0;  // 초기 x, y 값
    int radiusError = 1 - x;  // 반지름 오류 초기화

    // 원을 그리기 위한 알고리즘 반복
    while (x >= y)
    {
        // 8대칭성을 활용해 원의 점들을 그린다
        drawpoint(fd, x + center_x, y + center_y, r, g, b);
        drawpoint(fd, y + center_x, x + center_y, r, g, b);
        drawpoint(fd, -x + center_x, y + center_y, r, g, b);
        drawpoint(fd, -y + center_x, x + center_y, r, g, b);
        drawpoint(fd, -x + center_x, -y + center_y, r, g, b);
        drawpoint(fd, -y + center_x, -x + center_y, r, g, b);
        drawpoint(fd, x + center_x, -y + center_y, r, g, b);
        drawpoint(fd, y + center_x, -x + center_y, r, g, b);

        // 반지름 오류에 따라 x와 y 값 조정
        y++;
        if (radiusError < 0)
        {
            radiusError += 2 * y + 1;  // y 증가
        }
        else
        {
            x--;  // x 감소
            radiusError += 2 * (y - x + 1);  // x와 y 조정
        }
    }
}

// 화면 전체를 채우는 함수
static void drawface(int fd, int start_x, int start_y, int end_x, int end_y, ubyte r, ubyte g, ubyte b)
{
    ubyte a = 0xFF;  // 알파 값 (불투명)

    // 끝 좌표가 0일 경우, 화면 해상도 전체를 채움
    if (end_x == 0) end_x = vinfo.xres;
    if (end_y == 0) end_y = vinfo.yres;

    // 주어진 영역을 순회하며 각 픽셀을 색상으로 채움
    for (int x = start_x; x < end_x; x++)
    {
        for (int y = start_y; y < end_y; y++)
        {
            // x, y 좌표에 해당하는 픽셀 위치 계산
            int offset = (x + y * vinfo.xres) * vinfo.bits_per_pixel / 8.;

            // 파일 포인터를 해당 오프셋으로 이동
            lseek(fd, offset, SEEK_SET);

            // BGRA 순서로 색상 데이터 작성
            write(fd, &b, 1);  // 파랑
            write(fd, &g, 1);  // 초록
            write(fd, &r, 1);  // 빨강
            write(fd, &a, 1);  // 알파 (투명도)
        }
    }
}

// 메모리 매핑을 사용하여 화면 전체를 채우는 함수
static void drawfacemmap(int fd, int start_x, int start_y, int end_x, int end_y, ubyte r, ubyte g, ubyte b)
{
    ubyte *pfb;  // 프레임 버퍼 포인터

    ubyte a = 0xFF;  // 알파 값

    int depth = vinfo.bits_per_pixel / 8.;  // 바이트당 비트 수

    // 끝 좌표가 0일 경우, 화면 해상도 전체를 채움
    if (end_x == 0) end_x = vinfo.xres;
    if (end_y == 0) end_y = vinfo.yres;

    // mmap() 함수를 이용해서 메모리 맵을 작성
    pfb = (ubyte *)mmap(NULL, vinfo.xres * vinfo.yres * depth, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		// 오류 처리
    if (pfb == MAP_FAILED) {
        perror("Error: mmap failed");
        close(fd);  // fd를 닫고 종료
        return;
    }

    // 주어진 영역을 순회하며 색상 데이터를 채움
    for (int x = start_x; x < end_x * depth; x += depth)
    {
        for (int y = start_y; y < end_y; y++)
        {
            // 픽셀의 색상 값을 설정
            *(pfb + (x + 0) + y * vinfo.xres * depth) = b;  // 파랑
            *(pfb + (x + 1) + y * vinfo.xres * depth) = g;  // 초록
            *(pfb + (x + 2) + y * vinfo.xres * depth) = r;  // 빨강
            *(pfb + (x + 3) + y * vinfo.xres * depth) = a;  // 알파 (투명도)
        }
    }
    // 메모리 맵 해제
    munmap(pfb, vinfo.xres * vinfo.yres * depth);
}

int main(int argc, char **argv)
{
    int fdfd, status, offset;
    unsigned short pixel;

    // 프레임 버퍼 장치를 열기
    fdfd = open(FBDEVICE, O_RDWR);
    if (fdfd < 0)
    {
        perror("Error: cannot open framebuffer device");  // 오류 메시지 출력
        return -1;
    }

    // 프레임 버퍼의 화면 정보 가져오기
    if (ioctl(fdfd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        perror("Error reading fixed information");  // 오류 메시지 출력
        return -1;
    }

    // 현재 프레임 버퍼의 해상도 및 색상 깊이 정보 출력
    printf("Resolution : %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    printf("Virtual Resolution : %dx%d\n", vinfo.xres_virtual, vinfo.yres_virtual);

    // 각 색상 채널의 오프셋과 길이 출력
    printf("Red: offset = %d, length = %d\n", vinfo.red.offset, vinfo.red.length);
    printf("Green: offset = %d, length = %d\n", vinfo.green.offset, vinfo.green.length);
    printf("Blue: offset = %d, length = %d\n", vinfo.blue.offset, vinfo.blue.length);
		printf("Alpha (transparency): offset = %d, length = %d\n", vinfo.transp.offset, vinfo.transp.length);

	// 테스트를 위한 다양한 도형 그리기
	drawface(fdfd, 0, 0, 0, 0, 255, 255, 0);  // 화면 전체를 노란색으로 채움
	drawcircle(fdfd, 200, 200, 100, 255, 0, 255);  // 자홍색 원 그리기
	drawline(fdfd, 0, 100, 200, 0, 255, 255);  // 청록색 선 그리기


	// 화면 채우기
	//drawfacemmap(fdfd, 0, 0, 0, 0, 255, 255, 0);
	//drawcircle(fdfd, 200, 200, 100, 255, 0, 255);  // 자홍색 원 그리기

	// 프레임 버퍼 장치 닫기
	close(fdfd);

	return 0;
}
