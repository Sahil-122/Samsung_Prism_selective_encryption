#include <stdio.h>
#include <stdlib.h>


    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>


// Function to print YUV pixel values
void print_yuv_values(AVFrame *frame, int width, int height) {
    printf("Y Component:\n");
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int y_value = frame->data[0][y * frame->linesize[0] + x];
            printf("%d ", y_value);
        }
        printf("\n");
    }

    printf("U Component:\n");
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            int u_value = frame->data[1][y * frame->linesize[1] + x];
            printf("%d ", u_value);
        }
        printf("\n");
    }

    printf("V Component:\n");
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            int v_value = frame->data[2][y * frame->linesize[2] + x];
            printf("%d ", v_value);
        }
        printf("\n");
    }
}

// Function to read video frames and print YUV pixel values
void read_video_frames(const char *filename) {
    avformat_network_init();

    AVFormatContext *formatContext = NULL;
    if (avformat_open_input(&formatContext, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open video file: %s\n", filename);
        return;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        fprintf(stderr, "Could not retrieve stream info\n");
        avformat_close_input(&formatContext);
        return;
    }

    int videoStreamIndex = -1;
    for (unsigned i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        fprintf(stderr, "Could not find a video stream\n");
        avformat_close_input(&formatContext);
        return;
    }

    AVCodecParameters *codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
    if (!codec) {
        fprintf(stderr, "Unsupported codec\n");
        avformat_close_input(&formatContext);
        return;
    }

    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecContext, codecParameters);

    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return;
    }

    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    int frameCounter = 0;

    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecContext, packet) == 0) {
                while (avcodec_receive_frame(codecContext, frame) == 0) {
                    printf("Frame %d:\n", frameCounter);
                    print_yuv_values(frame, codecContext->width, codecContext->height);
                    frameCounter++;
                }
            }
        }
        av_packet_unref(packet);
    }

    // Clean up
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <video_file>\n", argv[0]);
        return -1;
    }

    read_video_frames(argv[1]);
    return 0;
}
// gcc -o yuv_extractor pixel_yuv.c -lavformat -lavcodec -lavutil -lswscale -lm
// ./yuv_extractor scene.mp4
