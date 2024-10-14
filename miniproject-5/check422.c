// check422.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>                  /* low-level I/O */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/fb.h>

#define FBDEV        "/dev/fb0"
#define WIDTH        800
#define HEIGHT       600

static short *fbp = NULL;
static struct fb_var_screeninfo vinfo;

// 프레임버퍼 초기화 함수
static void init_framebuffer()
{
    int fbfd = open(FBDEV, O_RDWR);
    if(fbfd == -1) {
        perror("open framebuffer device");
        exit(EXIT_FAILURE);
    }

    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information.");
        exit(EXIT_FAILURE);
    }

    long screensize = vinfo.xres * vinfo.yres * 2; // 16비트 컬러
    fbp = (short *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if(fbp == (short*)-1) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    memset(fbp, 0, screensize);

    close(fbfd);
}

// YUYV 데이터를 프레임버퍼에 출력하는 함수
static inline int clip(int value, int min, int max)
{
    return(value > max ? max : value < min ? min : value);
}

static void process_image(const unsigned char *in)
{
    int x, y, j;
    int y0, u, y1, v, r, g, b;
    unsigned short pixel;
    long location = 0;
    int istride = WIDTH * 2;

    for(y = 0; y < HEIGHT; ++y) {
        for(j = 0, x = 0; j < vinfo.xres * 2; j += 4, x += 2) {
            if(j >= WIDTH * 2) {
                location++;
                location++;
                continue;
            }
            /* YUYV 성분을 분리 */
            y0 = in[j] - 16;
            u = in[j + 1] - 128;
            y1 = in[j + 2] - 16;
            v = in[j + 3] - 128;

            /* YUV를 RGB로 전환 */
            r = clip((298 * y0 + 409 * v) >> 8, 0, 255);
            g = clip((298 * y0 - 100 * u - 208 * v) >> 8, 0, 255);
            b = clip((298 * y0 + 516 * u) >> 8, 0, 255);
            pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3); // 16비트 컬러
            fbp[location++] = pixel;

            /* YUV를 RGB로 전환 */
            r = clip((298 * y1 + 409 * v) >> 8, 0, 255);
            g = clip((298 * y1 - 100 * u - 208 * v) >> 8, 0, 255);
            b = clip((298 * y1 + 516 * u) >> 8, 0, 255);
            pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3); // 16비트 컬러
            fbp[location++] = pixel;
        }
        in += istride;
    }
}

int main(int argc, char **argv)
{
    if(argc != 2) {
        fprintf(stderr, "Usage: %s frame000.yuv\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    const int frame_size = WIDTH * HEIGHT * 2; // YUYV 포맷: 2바이트 per 픽셀
    unsigned char *frame_buffer = malloc(frame_size);
    if(!frame_buffer) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    // 프레임버퍼 초기화
    init_framebuffer();

    // 파일에서 프레임 데이터 읽기
    FILE *fp = fopen(filename, "rb");
    if(!fp) {
        perror("fopen");
        free(frame_buffer);
        exit(EXIT_FAILURE);
    }

    size_t read_bytes = fread(frame_buffer, 1, frame_size, fp);
    if(read_bytes != frame_size) {
        fprintf(stderr, "Error: read %zu bytes, expected %d bytes\n", read_bytes, frame_size);
        fclose(fp);
        free(frame_buffer);
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    // 프레임버퍼에 출력
    process_image(frame_buffer);

    // 프로그램을 종료하지 않고 대기
    printf("Press Enter to exit...\n");
    getchar();

    // 정리
    free(frame_buffer);

    return 0;
}

