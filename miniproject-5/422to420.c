#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

int main(int argc, char *argv[])
{
    char *input_file;
    char output_file[256];
    char *basename;
    char number[4];

    if (argc != 2) {
        printf("사용법: %s frameNNN.yuv\n", argv[0]);
        return 1;
    }

    input_file = argv[1];

    // 입력 파일명에서 NNN 추출
    basename = strrchr(input_file, '/');
    if (basename)
        basename++;
    else
        basename = input_file;

    if (strncmp(basename, "frame", 5) != 0) {
        printf("입력 파일명은 frameNNN.yuv 형식이어야 합니다.\n");
        return 1;
    }

    strncpy(number, basename+5, 3);
    number[3] = '\0';

    // 출력 파일명 생성
    sprintf(output_file, "trans%s.yuv", number);

    // 프레임의 가로와 세로 크기 설정
    int width = 800;
    int height = 600;

    // 입력 파일 열기
    FILE *fin = fopen(input_file, "rb");
    if (!fin) {
        perror("입력 파일 열기 실패");
        return 1;
    }

    // 출력 파일 열기
    FILE *fout = fopen(output_file, "wb");
    if (!fout) {
        perror("출력 파일 열기 실패");
        fclose(fin);
        return 1;
    }

    // 입력 파일 크기 계산
    fseek(fin, 0, SEEK_END);
    long file_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    // 한 프레임의 크기 계산
    int frame_size_in = width * height * 2; // YUV422는 16비트 per 픽셀
    int frame_size_out = width * height * 3 / 2; // YUV420는 width*height*1.5

    // 총 프레임 수 계산
    int total_frames = file_size / frame_size_in;
    if (total_frames <= 0) {
        fprintf(stderr, "입력 파일에 프레임이 없습니다.\n");
        fclose(fin);
        fclose(fout);
        return 1;
    }
    printf("총 프레임 수: %d\n", total_frames);

    // 입력용 AVFrame 할당
    AVFrame *pFrameIn = av_frame_alloc();
    if (!pFrameIn) {
        fprintf(stderr, "입력 프레임 할당 실패\n");
        fclose(fin);
        fclose(fout);
        return 1;
    }
    pFrameIn->format = AV_PIX_FMT_YUYV422;
    pFrameIn->width = width;
    pFrameIn->height = height;
    if (av_frame_get_buffer(pFrameIn, 1) < 0) {
        fprintf(stderr, "입력 프레임 버퍼 할당 실패\n");
        av_frame_free(&pFrameIn);
        fclose(fin);
        fclose(fout);
        return 1;
    }

    // 출력용 AVFrame 할당
    AVFrame *pFrameOut = av_frame_alloc();
    if (!pFrameOut) {
        fprintf(stderr, "출력 프레임 할당 실패\n");
        av_frame_free(&pFrameIn);
        fclose(fin);
        fclose(fout);
        return 1;
    }
    pFrameOut->format = AV_PIX_FMT_YUV420P;
    pFrameOut->width = width;
    pFrameOut->height = height;
    if (av_frame_get_buffer(pFrameOut, 1) < 0) {
        fprintf(stderr, "출력 프레임 버퍼 할당 실패\n");
        av_frame_free(&pFrameIn);
        av_frame_free(&pFrameOut);
        fclose(fin);
        fclose(fout);
        return 1;
    }

    // 색상 변환 컨텍스트 생성
    struct SwsContext *img_convert_ctx = sws_getContext(
        width, height, AV_PIX_FMT_YUYV422,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_BICUBIC, NULL, NULL, NULL
    );
    if (!img_convert_ctx) {
        fprintf(stderr, "변환 컨텍스트 초기화 실패\n");
        av_frame_free(&pFrameIn);
        av_frame_free(&pFrameOut);
        fclose(fin);
        fclose(fout);
        return 1;
    }

    // 입력 버퍼 할당
    uint8_t *in_buffer = (uint8_t *)av_malloc(frame_size_in);
    if (!in_buffer) {
        fprintf(stderr, "입력 버퍼 할당 실패\n");
        sws_freeContext(img_convert_ctx);
        av_frame_free(&pFrameIn);
        av_frame_free(&pFrameOut);
        fclose(fin);
        fclose(fout);
        return 1;
    }

    // 각 프레임을 처리
    for (int i = 0; i < total_frames; i++) {
        size_t bytes_read = fread(in_buffer, 1, frame_size_in, fin);
        if (bytes_read != frame_size_in) {
            fprintf(stderr, "프레임 %d 읽기 실패. 읽은 바이트 수: %zu\n", i, bytes_read);
            break;
        }

        // 입력 프레임에 데이터 복사
        if (av_image_fill_arrays(pFrameIn->data, pFrameIn->linesize, in_buffer, AV_PIX_FMT_YUYV422, width, height, 1) < 0) {
            fprintf(stderr, "프레임 %d 입력 데이터 복사 실패\n", i);
            break;
        }

        // YUV420P로 변환
        if (sws_scale(
                img_convert_ctx,
                (const uint8_t * const *)pFrameIn->data,
                pFrameIn->linesize,
                0,
                height,
                pFrameOut->data,
                pFrameOut->linesize
            ) <= 0) {
            fprintf(stderr, "프레임 %d 변환 실패\n", i);
            break;
        }

        // 변환된 데이터 출력 파일에 쓰기
        // Y 평면
        fwrite(pFrameOut->data[0], 1, width * height, fout);
        // U 평면
        fwrite(pFrameOut->data[1], 1, (width / 2) * (height / 2), fout);
        // V 평면
        fwrite(pFrameOut->data[2], 1, (width / 2) * (height / 2), fout);
    }

    // 자원 해제
    av_free(in_buffer);
    sws_freeContext(img_convert_ctx);
    av_frame_free(&pFrameIn);
    av_frame_free(&pFrameOut);
    fclose(fin);
    fclose(fout);

    printf("변환 완료: %s\n", output_file);
    return 0;
}

