#pragma once

/**
 * @file transcoder.h
 * @brief Full decode → re-encode pipeline from H.264 to H.265 (HEVC).
 *
 * Reads every video frame from the source file, decodes it with the
 * appropriate decoder, then re-encodes it with libx265 at the same
 * resolution, frame rate, and quality target (CRF 18, slow preset).
 * Non-video streams are currently skipped.
 */

/**
 * @brief Transcodes a video from H.264 to H.265 and writes the result to disk.
 *
 * Pipeline:
 *  - Opens @p inp_filename and finds the first video stream.
 *  - Initialises an H.264 decoder whose parameters are copied from the
 *    stream (resolution, frame rate, pixel format, GOP size).
 *  - Initialises a libx265 encoder with the same parameters (CRF 18, slow).
 *  - Reads packets → decodes frames → re-encodes → writes interleaved
 *    packets to the output container.
 *  - Flushes the encoder, writes the trailer, and releases all resources.
 *
 * av_dump_format() is called on the output before teardown so that codec
 * and container metadata is printed to stderr for debugging.
 *
 * @param inp_filename Path to the H.264 source file.
 * @param out_filename Path to the output H.265 .mp4 file (created/overwritten).
 */
void transcode_to_h265(const char *inp_filename, const char *out_filename);
