#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <x264.h>

// Function to initialize the x264 encoder
x264_t* initialize_encoder(int width, int height, int fps, x264_param_t *param, x264_picture_t *pic) {
    // Default parameters for x264
    x264_param_default_preset(param, "fast", "zerolatency");

    // Set up encoding parameters
    param->i_threads = 1;
    param->i_width = width;
    param->i_height = height;
    param->i_fps_num = fps;
    param->i_fps_den = 1;
    param->i_keyint_max = fps;
    param->b_intra_refresh = 1;
    param->rc.i_rc_method = X264_RC_CRF;
    param->rc.i_vbv_buffer_size = 1000;
    param->rc.i_vbv_max_bitrate = 1000;
    param->rc.f_rf_constant = 25;
    param->rc.f_rf_constant_max = 35;

    // Apply some profiles
    x264_param_apply_profile(param, "high");

    // Allocate and initialize x264 encoder
    x264_t *encoder = x264_encoder_open(param);

    // Initialize picture
    x264_picture_alloc(pic, X264_CSP_I420, width, height);

    return encoder;
}

int main() {
    int width = 640;
    int height = 480;
    int fps = 30;
    FILE *output_file = fopen("output.h264", "wb");

    // Check if file is opened successfully
    if (!output_file) {
        printf("Error: Could not open output file.\n");
        return -1;
    }

    // Encoder parameters
    x264_param_t param;
    x264_picture_t pic_in, pic_out;
    x264_t *encoder;

    // Initialize encoder and picture
    encoder = initialize_encoder(width, height, fps, &param, &pic_in);

    // Example loop to simulate raw video frames (YUV420p)
    for (int i = 0; i < 100; i++) {
        // Simulate filling the picture with YUV420 data (raw frame)
        memset(pic_in.img.plane[0], i, width * height); // Y plane
        memset(pic_in.img.plane[1], 128, width * height / 4); // U plane
        memset(pic_in.img.plane[2], 128, width * height / 4); // V plane

        // Encode the frame
        x264_nal_t *nals;
        int i_nals;
        int frame_size = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);

        if (frame_size > 0) {
            // Write the encoded frame to the file
            fwrite(nals->p_payload, 1, frame_size, output_file);
        }
    }

    // Flush encoder
    x264_encoder_close(encoder);
    x264_picture_clean(&pic_in);
    fclose(output_file);

    printf("Finished encoding.\n");
    return 0;
}
// gcc -o encode_h264 encode_h264.c -lx264
// ./encode_h264
