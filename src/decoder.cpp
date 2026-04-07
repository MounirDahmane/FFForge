/**
 * @file decoder.cpp
 * @brief Decodes a video and dumps its first N frames as grayscale PGM files.
 */

#include "decoder.h"
#include "utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}

#include <cinttypes>
#include <cstdio>
#include <cstdint>

void extract_gray_frames(const char *inp_filename, int max_frames)
{
    int ret = 0;

    /* --- Open container --- */
    AVFormatContext *inp_fmt_ctx = avformat_alloc_context();
    if (!inp_fmt_ctx)
        LOG_ERROR("could not allocate input format context");

    ret = avformat_open_input(&inp_fmt_ctx, inp_filename, nullptr, nullptr);
    if (ret < 0)
        LOG_ERROR("could not open input file '%s'", inp_filename);

    ret = avformat_find_stream_info(inp_fmt_ctx, nullptr);
    if (ret < 0)
        LOG_ERROR("could not find stream information");

    /* --- Locate the first video stream --- */
    int video_stream_idx       = -1;
    const AVCodec   *dec       = nullptr;
    AVCodecParameters *dec_par = nullptr;

    for (unsigned i = 0; i < inp_fmt_ctx->nb_streams; i++) {
        AVCodecParameters *par = inp_fmt_ctx->streams[i]->codecpar;
        if (par->codec_type != AVMEDIA_TYPE_VIDEO)
            continue;

        video_stream_idx = static_cast<int>(i);
        dec_par          = par;
        dec              = avcodec_find_decoder(par->codec_id);
        if (!dec)
            LOG_ERROR("no decoder found for codec id %d", par->codec_id);

        break; /* use first video stream only */
    }

    if (video_stream_idx < 0)
        LOG_ERROR("no video stream found in '%s'", inp_filename);

    /* --- Initialise decoder --- */
    AVCodecContext *dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx)
        LOG_ERROR("could not allocate decoder context");

    ret = avcodec_parameters_to_context(dec_ctx, dec_par);
    if (ret < 0)
        LOG_ERROR("could not copy codec parameters to decoder context");

    ret = avcodec_open2(dec_ctx, dec, nullptr);
    if (ret < 0)
        LOG_ERROR("could not open decoder");

    /* --- Allocate reusable buffers --- */
    AVFrame  *frame = av_frame_alloc();
    AVPacket *pkt   = av_packet_alloc();
    if (!frame || !pkt)
        LOG_ERROR("could not allocate frame/packet");

    /* --- Decode loop --- */
    int nb_frame = 0;

    while (av_read_frame(inp_fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index != video_stream_idx) {
            av_packet_unref(pkt);
            continue;
        }

        printf("AVPacket->pts %" PRId64 "\n", pkt->pts);

        ret = avcodec_send_packet(dec_ctx, pkt);
        if (ret < 0)
            LOG_ERROR("could not send packet to decoder");

        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_unref(pkt);
            continue;
        } else if (ret < 0) {
            LOG_ERROR("error while receiving frame from decoder");
        }

        /* Save the luma (Y) plane only → grayscale PGM */
        save_gray_frame(frame->data[0], frame->linesize[0],
                        frame->width, frame->height, nb_frame);

        av_packet_unref(pkt);

        if (++nb_frame >= max_frames)
            break;
    }

    /* --- Cleanup --- */
    avformat_close_input(&inp_fmt_ctx);
    avcodec_free_context(&dec_ctx);
    av_packet_free(&pkt);
    av_frame_free(&frame);
}
