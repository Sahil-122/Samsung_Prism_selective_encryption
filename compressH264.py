import ffmpeg
import numpy as np

# Define input and output paths
input_path = 'San Andreas/gtaSA.h264'  
output_path = 'San Andreas/compressed_SA.h264'  

# Probe the input video to get metadata
probe = ffmpeg.probe(input_path)
video_info = next(stream for stream in probe['streams'] if stream['codec_type'] == 'video')
width = int(video_info['width'])
height = int(video_info['height'])
fps = eval(video_info['r_frame_rate'])  # Convert FPS to a number
print(video_info)
print(f"Video width: {width}, height: {height}, fps: {fps}")

# Calculate the expected frame size for YUV420p format
expected_frame_size = width * height * 3 // 2  # YUV420p has 1.5 bytes per pixel

# Set up the input process to read raw video frames in YUV420p format
process_in = (
    ffmpeg
    .input(input_path)
    .output('pipe:', format='rawvideo', pix_fmt='yuv420p')
    .run_async(pipe_stdout=True)
)

# Set up the output process for writing H.264 with more aggressive compression
process_out = (
    ffmpeg
    .input('pipe:', format='rawvideo', s=f'{width}x{height}', pix_fmt='yuv420p', framerate=fps)
    .output(output_path, vcodec='libx264', video_bitrate='300k', preset='slow', crf=28)  # Increased compression
    .overwrite_output()
    .run_async(pipe_stdin=True)
)

# Process each frame
while True:
    in_bytes = process_in.stdout.read(expected_frame_size)

    # If the frame is incomplete, break the loop
    if len(in_bytes) != expected_frame_size:
        break

    # Convert bytes to a numpy array and reshape for YUV420p (no manipulation, direct passthrough)
    frame = np.frombuffer(in_bytes, np.uint8)
    
    # Write frame to output
    process_out.stdin.write(in_bytes)

# Close processes properly
process_in.wait()  # Wait for input process to finish
process_out.stdin.close()  # Close the output pipe
process_out.wait()  # Wait for output process to finish

print(f'Compressed video saved as {output_path}')
