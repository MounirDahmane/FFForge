/**
 * @file main.cpp
 * @brief FFForge — an FFmpeg-based video pipeline demo.
 *
 * Runs three sequential stages:
 *
 *  1. **Encode**    — Generates a synthetic 360×360 H.264/MP4 video
 *                     (2500 frames, 25 fps, CRF 18).
 *
 *  2. **Extract**   — Decodes the H.264 video and saves the first 10
 *                     frames as grayscale PGM images in ./frames/.
 *
 *  3. **Transcode** — Re-encodes the H.264 source as H.265/HEVC,
 *                     preserving resolution and frame rate.
 *
 * Each stage is self-contained; see encoder.h, decoder.h, and
 * transcoder.h for full API documentation.
 */

#include "encoder.h"
#include "decoder.h"
#include "transcoder.h"

#include <cstdio>

static const char *H264_OUTPUT   = "./output_H.264.mp4";
static const char *H265_OUTPUT   = "./output_H.265.mp4";
static const int   FRAMES_TO_DUMP = 10;

int main()
{
    /* Stage 1 — Synthesise and encode a test video in H.264. */
    printf("[FFForge] Stage 1: encoding H.264 video...\n");
    encode_h264(H264_OUTPUT);

    /* Stage 2 — Extract the first N frames as grayscale PGM files.
     * Requires the ./frames/ directory to exist. */
    printf("[FFForge] Stage 2: extracting %d gray frames...\n", FRAMES_TO_DUMP);
    extract_gray_frames(H264_OUTPUT, FRAMES_TO_DUMP);

    /* Stage 3 — Transcode from H.264 to H.265. */
    printf("[FFForge] Stage 3: transcoding to H.265...\n");
    transcode_to_h265(H264_OUTPUT, H265_OUTPUT);

    printf("[FFForge] Done.\n");
    return 0;
}
