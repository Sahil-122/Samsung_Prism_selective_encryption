#include <stdio.h>
#include <stdlib.h>


    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>


// Helper function to print pixel values
void print_pixel_values(uint8_t *data, int width, int height, int linesize) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width * 3; x += 3) {  // RGB has 3 channels
            int r = data[y * linesize + x];
            int g = data[y * linesize + x + 1];
            int b = data[y * linesize + x + 2];
            printf("(%d, %d, %d) ", r, g, b);
        }
        printf("\n");
    }
}

// Function to read video frames and print pixel values
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
    AVFrame *rgbFrame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    struct SwsContext *swsContext = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt,
                                                   codecContext->width, codecContext->height, AV_PIX_FMT_RGB24,
                                                   SWS_BILINEAR, NULL, NULL, NULL);

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecContext->width, codecContext->height, 1);
    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24,
                         codecContext->width, codecContext->height, 1);

    int frameCounter = 0;

    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecContext, packet) == 0) {
                while (avcodec_receive_frame(codecContext, frame) == 0) {
                    sws_scale(swsContext, (uint8_t const *const *) frame->data, frame->linesize, 0,
                              codecContext->height, rgbFrame->data, rgbFrame->linesize);

                    printf("Frame %d:\n", frameCounter);
                    print_pixel_values(rgbFrame->data[0], codecContext->width, codecContext->height, rgbFrame->linesize[0]);
                    printf("\n");
                    frameCounter++;
                }
            }
        }
        av_packet_unref(packet);
    }

    // Clean up
    sws_freeContext(swsContext);
    av_free(buffer);
    av_frame_free(&rgbFrame);
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
// gcc -o pixel_extractor pixel_rgb.c -lavformat -lavcodec -lavutil -lswscale -lm
// ./pixel_extractor scene.mp4
