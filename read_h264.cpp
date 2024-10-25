#include <iostream>
#include <memory>
#include <cstdio>  // For error printing

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

// Helper function to handle errors
void check_error(int err) {
    if (err < 0) {
        char errbuf[128];
        av_strerror(err, errbuf, sizeof(errbuf));
        std::cerr << "Error: " << errbuf << std::endl;
        exit(1);
    }
}

// Function to read H.264 video data
void read_h264_video(const char* filename) {
    // Open video file and allocate format context
    AVFormatContext* formatContext = nullptr;
    check_error(avformat_open_input(&formatContext, filename, nullptr, nullptr));

    // Retrieve stream information
    check_error(avformat_find_stream_info(formatContext, nullptr));

    // Print basic information about the video file
    std::cout << "Video file: " << filename << std::endl;
    std::cout << "Format: " << formatContext->iformat->name << std::endl;
    std::cout << "Duration: " << formatContext->duration / AV_TIME_BASE << " seconds" << std::endl;
    std::cout << "Bitrate: " << formatContext->bit_rate << " bps" << std::endl;

    // Find the video stream
    int videoStreamIndex = -1;
    for (unsigned i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "Didn't find a video stream" << std::endl;
        avformat_close_input(&formatContext);
        return;
    }

    // Find the decoder for the video stream
    AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
    if (!codec) {
        std::cerr << "Unsupported codec!" << std::endl;
        avformat_close_input(&formatContext);
        return;
    }

    // Print codec information
    std::cout << "Codec: " << codec->long_name << std::endl;
    std::cout << "Resolution: " << codecParameters->width << "x" << codecParameters->height << std::endl;

    // Allocate codec context and copy codec parameters to context
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    check_error(avcodec_parameters_to_context(codecContext, codecParameters));

    // Open the codec
    check_error(avcodec_open2(codecContext, codec, nullptr));

    // Allocate frame and packet structures
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    if (!frame || !packet) {
        std::cerr << "Could not allocate frame or packet" << std::endl;
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return;
    }

    // Track frame number manually
    int frameCounter = 0;

    // Read frames from the file
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            check_error(avcodec_send_packet(codecContext, packet));

            // Get all the available frames from the decoder
            while (true) {
                int ret = avcodec_receive_frame(codecContext, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;  // Need more data or finished
                } else if (ret < 0) {
                    std::cerr << "Error during decoding" << std::endl;
                    break;
                }

                // Increment the frame counter
                frameCounter++;
                std::cout << "\nDecoded frame " << frameCounter << ":" << std::endl;
                // std::cout << "  Width: " << frame->width << std::endl;
                // std::cout << "  Height: " << frame->height << std::endl;
                // std::cout << "  Pixel Format: " << av_get_pix_fmt_name((AVPixelFormat)frame->format) << std::endl;
                // std::cout << "  Presentation Timestamp (PTS): " << frame->pts << std::endl;

                // Print frame type
                switch (frame->pict_type) {
                    case AV_PICTURE_TYPE_I:
                        std::cout << "  Frame Type: I Frame" << std::endl;
                        break;
                    case AV_PICTURE_TYPE_P:
                        std::cout << "  Frame Type: P Frame" << std::endl;
                        break;
                    case AV_PICTURE_TYPE_B:
                        std::cout << "  Frame Type: B Frame" << std::endl;
                        break;
                    default:
                        std::cout << "  Frame Type: Other" << std::endl;
                        break;
                }

                // Process the frame (e.g., display, save, etc.)
            }
        }

        av_packet_unref(packet);
    }

    // Print summary
    std::cout << "\nTotal frames decoded: " << frameCounter << std::endl;

    // Clean up
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
}

// Main function
int main() {
    const char* filename = "scene.mp4";  // Change this to your H.264 video file path
    // const char* filename = "output.h264";
    read_h264_video(filename);
    return 0;
}

// g++ -o read_h264 read_h264.cpp -lavformat -lavcodec -lavutil -lswscale
// ./read_h264
// for qp table - ffmpeg -i scene.mp4 -debug qp -f null -

// 1. General Video Information ->  ffmpeg -i scene.mp4 out.h264
// 2. detailed metadata -> ffprobe -v error -show_format -show_streams scene.mp4
// 3. frame by frame details -> ffprobe -show_frames scene.mp4
// 4. To see bitrate variability over time, GOP(group of picture) structure, and other statistics -> ffmpeg -i scene.mp4 -vf "showinfo" -f null - 
// 5. To show only codec and stream-specific details -> ffprobe -v error -select_streams v:0 -show_entries stream=codec_name,codec_type,width,height,r_frame_rate,bit_rate,duration scene.mp4
// 5. extracting tranform coefficient -> ffmpeg -i scene.mp4 -c:v libx264 -psy-rd 0.4:0 -f null - 2>coefficients.txt


 