#include "framebuffer.h"

ubyte *fbp = NULL;               /* 프레임버퍼의 메모리 맵핑을 위한 변수 */
struct fb_var_screeninfo vinfo;  /* 프레임버퍼의 정보 저장 구조체 */
int screensize_Frambuffer;

int init_frambuffer(int fd)
{

    /* 프레임버퍼 정보 가져오기 */
    if (-1 == ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("ioctl() : FBIOGET_VSCREENINFO");
        return -1;
    }


    /* 프레임버퍼를 위한 메모리 맵핑 */
    int depth_fb = vinfo.bits_per_pixel / 8.;  // 프레임버퍼의 픽셀당 바이트 수 : 4byte
    screensize_Frambuffer = vinfo.yres * vinfo.xres * depth_fb;  // 프레임버퍼 전체 크기 계산
    fbp = (ubyte *)mmap(NULL, screensize_Frambuffer, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fbp == (ubyte*)-1) {
        perror("mmap() : framebuffer device to memory");
        return -1;
    }

    /* 프레임버퍼 초기화 */
    memset(fbp, 0, screensize_Frambuffer);

    return 0;

}