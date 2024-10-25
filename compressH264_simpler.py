import ffmpeg

# Define input and output paths
input_path = 'San Andreas/gtaSA_og.h264'
output_path = 'San Andreas/compressed_gtaSA.h264'  # Output H.264 file

# Use ffmpeg to re-encode the compressed video with higher quality settings
ffmpeg.input(input_path).output(
    output_path,
    vcodec='libx264',          # Use x264 codec for re-encoding
    video_bitrate='1000k',   
    preset='slow',           # Adjust the preset for quality vs. speed
    crf=28                     # Constant Rate Factor (lower is better quality)
).overwrite_output().run()
