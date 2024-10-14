// transencoder.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>

#define WIDTH 800
#define HEIGHT 600
#define FRAME_COUNT 3

int main(int argc, char **argv)
{
    const char *output_filename = "transvideo000.h264";

    // H.264 인코더 찾기
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "코덱을 찾을 수 없습니다.\n");
        exit(EXIT_FAILURE);
    }

    // 코덱 컨텍스트 할당
    AVCodecContext *c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "비디오 코덱 컨텍스트를 할당할 수 없습니다.\n");
        exit(EXIT_FAILURE);
    }

    // 코덱 파라미터 설정
    c->bit_rate = 1000000;  // 비트레이트를 증가하여 품질 개선
    c->width = WIDTH;
    c->height = HEIGHT;
    c->time_base = (AVRational){1, 25};
    c->framerate = (AVRational){25, 1};
    c->gop_size = 10;
    c->max_b_frames = 0;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    // 인코더 설정 (옵션 추가)
    AVDictionary *param = NULL;
    av_dict_set(&param, "preset", "ultrafast", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);

    // 코덱 열기
    if (avcodec_open2(c, codec, &param) < 0) {
        fprintf(stderr, "코덱을 열 수 없습니다.\n");
        avcodec_free_context(&c);
        exit(EXIT_FAILURE);
    }

    // 출력 파일 열기
    FILE *f = fopen(output_filename, "wb");
    if (!f) {
        fprintf(stderr, "%s 파일을 열 수 없습니다.\n", output_filename);
        avcodec_free_context(&c);
        exit(EXIT_FAILURE);
    }

    // 출력용 프레임 할당 (YUV420P)
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "비디오 프레임을 할당할 수 없습니다.\n");
        fclose(f);
        avcodec_free_context(&c);
        exit(EXIT_FAILURE);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    // 프레임 버퍼 할당
    int ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
    if (ret < 0) {
        fprintf(stderr, "프레임 버퍼를 할당할 수 없습니다.\n");
        av_frame_free(&frame);
        fclose(f);
        avcodec_free_context(&c);
        exit(EXIT_FAILURE);
    }

    // 패킷 할당
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "AVPacket을 할당할 수 없습니다.\n");
        av_freep(&frame->data[0]);
        av_frame_free(&frame);
        fclose(f);
        avcodec_free_context(&c);
        exit(EXIT_FAILURE);
    }

    // 프레임 인덱스
    for (int i = 0; i < FRAME_COUNT; i++) {
        char filename[20];
        snprintf(filename, sizeof(filename), "trans%03d.yuv", i);

        // YUV420P 파일 열기
        FILE *fp = fopen(filename, "rb");
        if (!fp) {
            fprintf(stderr, "%s 파일을 열 수 없습니다.\n", filename);
            break;
        }

        const int y_size = WIDTH * HEIGHT;
        const int chroma_size = (WIDTH / 2) * (HEIGHT / 2);
        const int in_frame_size = y_size + 2 * chroma_size; // YUV420 포맷: Y + U + V

        unsigned char *in_frame_buffer = malloc(in_frame_size);
        if(!in_frame_buffer) {
            fprintf(stderr, "메모리를 할당할 수 없습니다.\n");
            fclose(fp);
            break;
        }

        size_t read_bytes = fread(in_frame_buffer, 1, in_frame_size, fp);
        if(read_bytes != in_frame_size) {
            fprintf(stderr, "%s에서 데이터를 모두 읽지 못했습니다. 읽은 바이트 수: %zu\n", filename, read_bytes);
            fclose(fp);
            free(in_frame_buffer);
            break;
        }
        fclose(fp);

        unsigned char *Y_plane = in_frame_buffer;
        unsigned char *U_plane = in_frame_buffer + y_size;
        unsigned char *V_plane = in_frame_buffer + y_size + chroma_size;

        // 프레임 데이터 채우기 (linesize 고려)
        for (int y = 0; y < HEIGHT; y++) {
            memcpy(frame->data[0] + y * frame->linesize[0], Y_plane + y * WIDTH, WIDTH);
        }

        int chroma_height = HEIGHT / 2;
        int chroma_width = WIDTH / 2;
        for (int y = 0; y < chroma_height; y++) {
            memcpy(frame->data[1] + y * frame->linesize[1], U_plane + y * chroma_width, chroma_width);
            memcpy(frame->data[2] + y * frame->linesize[2], V_plane + y * chroma_width, chroma_width);
        }

        free(in_frame_buffer);

        frame->pts = i;

        // 프레임을 인코더로 전송
        ret = avcodec_send_frame(c, frame);
        if (ret < 0) {
            fprintf(stderr, "프레임을 인코더로 전송하는 중 오류 발생: %s\n", av_err2str(ret));
            break;
        }

        // 인코더로부터 패킷 수신 및 파일에 기록
        while (ret >= 0) {
            ret = avcodec_receive_packet(c, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                fprintf(stderr, "패킷을 받는 중 오류 발생: %s\n", av_err2str(ret));
                break;
            }

            fwrite(pkt->data, 1, pkt->size, f);
            av_packet_unref(pkt);
        }
    }

    // 인코더 플러시
    ret = avcodec_send_frame(c, NULL);
    while (ret >= 0) {
        ret = avcodec_receive_packet(c, pkt);
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
            break;
        else if (ret < 0) {
            fprintf(stderr, "인코더 플러시 중 오류 발생: %s\n", av_err2str(ret));
            break;
        }
        fwrite(pkt->data, 1, pkt->size, f);
        av_packet_unref(pkt);
    }

    // 리소스 해제
    fclose(f);
    av_packet_free(&pkt);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    avcodec_free_context(&c);

    printf("비디오가 %s로 저장되었습니다.\n", output_filename);
    return 0;
}
