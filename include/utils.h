#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

/**
 * @file utils.h
 * @brief Shared utilities for FFForge: packet flushing, frame I/O helpers,
 *        and a structured error-logging macro.
 */

/**
 * @brief Logs a formatted error message with file/line/function context to
 *        stderr and immediately terminates the process.
 *
 * Wraps fprintf so the call site reads like a single statement.
 * Intended for unrecoverable FFmpeg API failures only.
 */
#define LOG_ERROR(...) do {                                                    \
    fprintf(stderr, "\033[1;31m[ERROR] %s:%d (%s): ", __FILE__, __LINE__,     \
            __func__);                                                          \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\033[0m\n");                                              \
    exit(1);                                                                   \
} while(0)

/**
 * @brief Sends one frame (or a flush signal) to an encoder and writes all
 *        resulting packets to the muxer.
 *
 * Passing @p frame as nullptr flushes the encoder, draining any buffered
 * frames. Each successfully received packet is written via
 * av_interleaved_write_frame and then unreferenced.
 *
 * @param codec_ctx  Open encoder context.
 * @param frame      Frame to encode, or nullptr to flush.
 * @param pkt        Scratch packet — must be allocated by the caller.
 * @param fmt_ctx    Output format context to which packets are written.
 */
void encode_frame(AVCodecContext *codec_ctx, AVFrame *frame,
                  AVPacket *pkt, AVFormatContext *fmt_ctx);

/**
 * @brief Writes a single luminance (Y) plane to a PGM file on disk.
 *
 * The output file is created at "./frames/frame<nb_frame>.pgm". The
 * directory must already exist; no attempt is made to create it.
 *
 * @param buf       Pointer to the first byte of the Y plane.
 * @param wrap      Stride (linesize) of the Y plane in bytes.
 * @param xsize     Frame width in pixels.
 * @param ysize     Frame height in pixels.
 * @param nb_frame  Zero-based frame index used to form the filename.
 */
void save_gray_frame(unsigned char *buf, int wrap,
                     int xsize, int ysize, int nb_frame);
