#pragma once

/**
 * @file decoder.h
 * @brief Decodes a video file and extracts its first N frames as PGM images.
 *
 * Only the luma (Y) plane is saved, producing true grayscale snapshots of
 * each decoded frame. This is useful for quick visual inspection of a video
 * without needing a full playback pipeline.
 */

/**
 * @brief Decodes @p max_frames frames from @p inp_filename and saves each as
 *        a grayscale PGM file.
 *
 * Pipeline:
 *  - Opens the input container and locates the first video stream.
 *  - Initialises the appropriate decoder from the stream's codec parameters.
 *  - Reads packets in a loop; for each video packet the decoded frame's Y
 *    plane is written to "./frames/frame<N>.pgm" via save_gray_frame().
 *  - Stops after @p max_frames frames have been saved.
 *
 * The output directory ("./frames/") must exist before calling this function.
 * All FFmpeg resources are freed before returning.
 *
 * @param inp_filename Path to the input video file.
 * @param max_frames   Maximum number of frames to extract.
 */
void extract_gray_frames(const char *inp_filename, int max_frames);
