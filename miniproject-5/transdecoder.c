// transdecoder.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h> // AVFormatContext 사용을 위해 추가
#include <libavutil/imgutils.h>
#include <libavutil/error.h>

int main(int argc, char **argv)
{
    const char *input_filename = "transvideo000.h264";

    AVFormatContext *fmt_ctx = NULL;

    // 입력 파일 열기
    if (avformat_open_input(&fmt_ctx, input_filename, NULL, NULL) < 0) {
        fprintf(stderr, "%s 파일을 열 수 없습니다.\n", input_filename);
        exit(EXIT_FAILURE);
    }

    // 스트림 정보 가져오기
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "스트림 정보를 가져올 수 없습니다.\n");
        avformat_close_input(&fmt_ctx);
        exit(EXIT_FAILURE);
    }

    // 비디오 스트림 인덱스 찾기
    int video_stream_index = -1;
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        fprintf(stderr, "비디오 스트림을 찾을 수 없습니다.\n");
        avformat_close_input(&fmt_ctx);
        exit(EXIT_FAILURE);
    }

    // 디코더 찾기
    AVCodecParameters *codecpar = fmt_ctx->streams[video_stream_index]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "코덱을 찾을 수 없습니다.\n");
        avformat_close_input(&fmt_ctx);
        exit(EXIT_FAILURE);
    }

    // 코덱 컨텍스트 할당 및 초기화
    AVCodecContext *c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "코덱 컨텍스트를 할당할 수 없습니다.\n");
        avformat_close_input(&fmt_ctx);
        exit(EXIT_FAILURE);
    }

    if (avcodec_parameters_to_context(c, codecpar) < 0) {
        fprintf(stderr, "코덱 파라미터를 컨텍스트로 복사하는 데 실패했습니다.\n");
        avcodec_free_context(&c);
        avformat_close_input(&fmt_ctx);
        exit(EXIT_FAILURE);
    }

    // 코덱 열기
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "코덱을 열 수 없습니다.\n");
        avcodec_free_context(&c);
        avformat_close_input(&fmt_ctx);
        exit(EXIT_FAILURE);
    }

    // 프레임 할당
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "프레임을 할당할 수 없습니다.\n");
        avcodec_free_context(&c);
        avformat_close_input(&fmt_ctx);
        exit(EXIT_FAILURE);
    }

    // 패킷 할당
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "패킷을 할당할 수 없습니다.\n");
        av_frame_free(&frame);
        avcodec_free_context(&c);
        avformat_close_input(&fmt_ctx);
        exit(EXIT_FAILURE);
    }

    int frame_count = 0;

    // 패킷을 읽어와 디코딩
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            int ret = avcodec_send_packet(c, pkt);
            if (ret < 0) {
                fprintf(stderr, "디코더에 패킷을 전송하는 중 오류 발생: %s\n", av_err2str(ret));
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(c, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "디코딩 중 오류 발생: %s\n", av_err2str(ret));
                    exit(EXIT_FAILURE);
                }

                // 디코딩된 프레임을 YUV 파일로 저장
                char output_filename[25];
                snprintf(output_filename, sizeof(output_filename), "transdecode%03d.yuv", frame_count);

                FILE *out_file = fopen(output_filename, "wb");
                if (!out_file) {
                    fprintf(stderr, "%s 파일을 열 수 없습니다.\n", output_filename);
                    exit(EXIT_FAILURE);
                }

                // YUV420P 포맷의 데이터를 파일에 저장
                int y_size = frame->width * frame->height;
                int uv_size = y_size / 4;

                // Y 평면
                fwrite(frame->data[0], 1, y_size, out_file);

                // U 평면
                fwrite(frame->data[1], 1, uv_size, out_file);

                // V 평면
                fwrite(frame->data[2], 1, uv_size, out_file);

                fclose(out_file);
                printf("프레임 %d을(를) %s로 저장했습니다.\n", frame_count, output_filename);
                frame_count++;
            }
        }
        av_packet_unref(pkt);
    }

    // 디코더 플러시
    avcodec_send_packet(c, NULL);
    while (1) {
        int ret = avcodec_receive_frame(c, frame);
        if (ret == AVERROR_EOF)
            break;
        else if (ret >= 0) {
            // 디코딩된 프레임을 YUV 파일로 저장
            char output_filename[25];
            snprintf(output_filename, sizeof(output_filename), "transdecode%03d.yuv", frame_count);

            FILE *out_file = fopen(output_filename, "wb");
            if (!out_file) {
                fprintf(stderr, "%s 파일을 열 수 없습니다.\n", output_filename);
                exit(EXIT_FAILURE);
            }

            // YUV420P 포맷의 데이터를 파일에 저장
            int y_size = frame->width * frame->height;
            int uv_size = y_size / 4;

            // Y 평면
            fwrite(frame->data[0], 1, y_size, out_file);

            // U 평면
            fwrite(frame->data[1], 1, uv_size, out_file);

            // V 평면
            fwrite(frame->data[2], 1, uv_size, out_file);

            fclose(out_file);
            printf("프레임 %d을(를) %s로 저장했습니다.\n", frame_count, output_filename);
            frame_count++;
        } else {
            fprintf(stderr, "디코딩 중 오류 발생: %s\n", av_err2str(ret));
            break;
        }
    }

    // 리소스 해제
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&c);
    avformat_close_input(&fmt_ctx);

    return 0;
}

