// transcheck420.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>                  /* low-level I/O */
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/fb.h>

#define FBDEV        "/dev/fb0"
#define WIDTH        800
#define HEIGHT       600

typedef enum {
    YCBCR_JPEG,
    YCBCR_601,
    YCBCR_709
} YCbCrType;

static unsigned char *fbp = NULL;
static struct fb_var_screeninfo vinfo;

// 클리핑 함수
static inline uint8_t clamp(int16_t value)
{
    return value < 0 ? 0 : (value > 255 ? 255 : value);
}

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
        close(fbfd);
        exit(EXIT_FAILURE);
    }

    long screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    fbp = (unsigned char *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if(fbp == MAP_FAILED) {
        perror("mmap");
        close(fbfd);
        exit(EXIT_FAILURE);
    }

    memset(fbp, 0, screensize);

    close(fbfd);
}

// YUV420p를 RGB24로 변환하는 함수
void yuv420_rgb24_std(
    uint32_t width, uint32_t height, 
    const uint8_t *Y, const uint8_t *U, const uint8_t *V, uint32_t Y_stride, uint32_t UV_stride, 
    uint8_t *RGB, uint32_t RGB_stride, 
    YCbCrType yuv_type)
{
    uint32_t x, y;
    int C, D, E, R, G, B;
    for(y = 0; y < height; y++) {
        const uint8_t *y_ptr = Y + y * Y_stride;
        const uint8_t *u_ptr = U + (y / 2) * UV_stride;
        const uint8_t *v_ptr = V + (y / 2) * UV_stride;
        uint8_t *rgb_ptr = RGB + y * RGB_stride;

        for(x = 0; x < width; x++) {
            int Y_val = y_ptr[x];
            int U_val = u_ptr[x / 2];
            int V_val = v_ptr[x / 2];

            C = Y_val - 16;
            D = U_val - 128;
            E = V_val - 128;

            R = (298 * C + 409 * E + 128) >> 8;
            G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            B = (298 * C + 516 * D + 128) >> 8;

            rgb_ptr[3 * x] = clamp(R);
            rgb_ptr[3 * x + 1] = clamp(G);
            rgb_ptr[3 * x + 2] = clamp(B);
        }
    }
}

// RGB24 데이터를 프레임버퍼에 출력하는 함수
static void display_rgb24(unsigned char *rgb_buffer)
{
    int x, y;
    int fb_stride = vinfo.xres * vinfo.bits_per_pixel / 8;
    unsigned char *fb_ptr = fbp;

    for(y = 0; y < HEIGHT; y++) {
        if (y >= vinfo.yres) break; // 화면을 벗어나면 중단
        unsigned char *fb_line_ptr = fb_ptr + y * fb_stride;
        unsigned char *rgb_line_ptr = rgb_buffer + y * WIDTH * 3;

        for(x = 0; x < WIDTH; x++) {
            if (x >= vinfo.xres) break; // 화면을 벗어나면 중단
            int pixel_offset = x * (vinfo.bits_per_pixel / 8);
            uint8_t R = rgb_line_ptr[3 * x];
            uint8_t G = rgb_line_ptr[3 * x + 1];
            uint8_t B = rgb_line_ptr[3 * x + 2];

            if(vinfo.bits_per_pixel == 32) {
                // RGBA8888 포맷
                fb_line_ptr[pixel_offset] = B;
                fb_line_ptr[pixel_offset + 1] = G;
                fb_line_ptr[pixel_offset + 2] = R;
                fb_line_ptr[pixel_offset + 3] = 0xFF; // Alpha 채널
            } else if(vinfo.bits_per_pixel == 24) {
                // RGB888 포맷
                fb_line_ptr[pixel_offset] = B;
                fb_line_ptr[pixel_offset + 1] = G;
                fb_line_ptr[pixel_offset + 2] = R;
            } else if(vinfo.bits_per_pixel == 16) {
                // RGB565 포맷
                unsigned short pixel = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3);
                *((unsigned short*)(fb_line_ptr + pixel_offset)) = pixel;
            } else {
                // 지원하지 않는 비트 깊이
                fprintf(stderr, "Unsupported bits per pixel: %d\n", vinfo.bits_per_pixel);
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char **argv)
{
    if(argc != 2) {
        fprintf(stderr, "Usage: %s trans000.yuv\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    const int y_size = WIDTH * HEIGHT;
    const int chroma_size = (WIDTH / 2) * (HEIGHT / 2);
    const int in_frame_size = y_size + 2 * chroma_size; // YUV420 포맷: Y + U + V

    unsigned char *in_frame_buffer = malloc(in_frame_size);
    if(!in_frame_buffer) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    // 프레임버퍼 초기화
    init_framebuffer();

    // 파일에서 YUV420 프레임 데이터 읽기
    FILE *fp = fopen(filename, "rb");
    if(!fp) {
        perror("fopen");
        free(in_frame_buffer);
        exit(EXIT_FAILURE);
    }

    size_t read_bytes = fread(in_frame_buffer, 1, in_frame_size, fp);
    if(read_bytes != in_frame_size) {
        fprintf(stderr, "Error: read %zu bytes, expected %d bytes\n", read_bytes, in_frame_size);
        fclose(fp);
        free(in_frame_buffer);
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    // YUV420p 데이터를 RGB24로 변환
    unsigned char *rgb_buffer = malloc(WIDTH * HEIGHT * 3);
    if(!rgb_buffer) {
        fprintf(stderr, "Out of memory for RGB buffer\n");
        free(in_frame_buffer);
        exit(EXIT_FAILURE);
    }

    unsigned char *Y_plane = in_frame_buffer;
    unsigned char *U_plane = in_frame_buffer + y_size;
    unsigned char *V_plane = in_frame_buffer + y_size + chroma_size;

    yuv420_rgb24_std(
        WIDTH, HEIGHT,
        Y_plane, U_plane, V_plane,
        WIDTH, WIDTH / 2,
        rgb_buffer, WIDTH * 3,
        YCBCR_601 // BT.601 표준 사용
    );

    // 변환된 RGB24 데이터를 프레임버퍼에 출력
    display_rgb24(rgb_buffer);

    // 리소스 해제
    free(in_frame_buffer);
    free(rgb_buffer);

    // 프로그램을 종료하지 않고 대기
    printf("Press Enter to exit...\n");
    getchar();

    return 0;
}

