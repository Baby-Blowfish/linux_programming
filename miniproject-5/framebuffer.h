#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__
#include "Common.h"


typedef unsigned char ubyte;  // unsigned char에 대한 별칭 정의
extern ubyte *fbp;           /* 프레임버퍼의 메모리 맵핑을 위한 변수 */
extern struct fb_var_screeninfo vinfo;  /* 프레임버퍼의 정보 저장 구조체 */
extern int screensize_Frambuffer;

int init_frambuffer(int fd);

#endif
