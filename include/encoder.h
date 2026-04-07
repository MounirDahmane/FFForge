#pragma once

/**
 * @file encoder.h
 * @brief H.264 encoder that generates a synthetic test video.
 *
 * Produces a 360×360 YUV420P video whose luma and chroma values are derived
 * from the frame index, yielding a smoothly scrolling colour pattern.
 * Output is written as an MP4 container using libx264 at CRF 18, slow preset.
 */

/**
 * @brief Encodes a synthetic 2500-frame H.264 video to disk.
 *
 * Pipeline:
 *  - Opens a libx264 encoder (360×360, 25 fps, CRF 18, slow preset).
 *  - Allocates an output MP4 container at @p out_filename.
 *  - For each frame, fills the Y/U/V planes with a function of (x, y, i)
 *    that produces an animated diagonal colour pattern.
 *  - Flushes the encoder and writes the container trailer on completion.
 *
 * All FFmpeg resources are freed before returning.
 *
 * @param out_filename Path to the output .mp4 file (created/overwritten).
 */
void encode_h264(const char *out_filename);
