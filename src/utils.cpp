/**
 * @file utils.cpp
 * @brief Implementations of encode_frame() and save_gray_frame().
 */

#include "utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <cstdio>

void encode_frame(AVCodecContext *codec_ctx, AVFrame *frame,
                  AVPacket *pkt, AVFormatContext *fmt_ctx)
{
    int response = avcodec_send_frame(codec_ctx, frame);
    if (response < 0)
        LOG_ERROR("could not send frame to the encoder");

    /* Drain all packets the encoder has ready. */
    while (response >= 0) {
        response = avcodec_receive_packet(codec_ctx, pkt);

        /* EAGAIN  → encoder needs more input before producing output.
         * EOF     → encoder was flushed and is done.
         * Both are normal exit conditions for this loop.              */
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
            return;
        else if (response < 0)
            LOG_ERROR("error while receiving packet from encoder");

        av_interleaved_write_frame(fmt_ctx, pkt);
        av_packet_unref(pkt);
    }
}

void save_gray_frame(unsigned char *buf, int wrap,
                     int xsize, int ysize, int nb_frame)
{
    char filename[64];
    snprintf(filename, sizeof(filename), "./frames/frame%d.pgm", nb_frame);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen");
        return;
    }

    /* PGM binary (P5) header: width height max-value */
    fprintf(f, "P5\n%d %d\n255\n", xsize, ysize);

    /* Write one raster line at a time, respecting the plane stride. */
    for (int i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);

    fclose(f);
}
