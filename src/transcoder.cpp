/**
 * @file transcoder.cpp
 * @brief Full decode-and-re-encode pipeline: H.264 → H.265 (HEVC).
 *
 * Unlike a stream-copy remux, this does a full pixel-level transcode so that
 * encoder-specific parameters (GOP structure, B-frames, CRF) can be set
 * independently on the output side.
 */

#include "transcoder.h"
#include "utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
}

#include <cstdio>

void transcode_to_h265(const char *inp_filename, const char *out_filename)
{
    int ret = 0;

    /* ------------------------------------------------------------------ */
    /* Input: open container and find the video stream                      */
    /* ------------------------------------------------------------------ */

    AVFormatContext *in_fmt = avformat_alloc_context();
    if (!in_fmt)
        LOG_ERROR("could not allocate input format context");

    ret = avformat_open_input(&in_fmt, inp_filename, nullptr, nullptr);
    if (ret < 0)
        LOG_ERROR("could not open input file '%s'", inp_filename);

    ret = avformat_find_stream_info(in_fmt, nullptr);
    if (ret < 0)
        LOG_ERROR("could not find stream info for '%s'", inp_filename);

    /* ------------------------------------------------------------------ */
    /* Output: allocate container                                           */
    /* ------------------------------------------------------------------ */

    AVFormatContext *out_fmt = nullptr;
    ret = avformat_alloc_output_context2(&out_fmt, nullptr, nullptr,
                                          out_filename);
    if (ret < 0)
        LOG_ERROR("could not allocate output format context");

    /* ------------------------------------------------------------------ */
    /* Decoder: initialise from the first video stream's codec parameters  */
    /* ------------------------------------------------------------------ */

    const AVCodec  *dec_codec  = nullptr;
    AVCodecContext *dec_ctx    = nullptr;
    AVStream       *out_stream = nullptr;
    bool            found_video = false;

    for (unsigned i = 0; i < in_fmt->nb_streams; i++) {
        AVStream          *in_stream = in_fmt->streams[i];
        AVCodecParameters *par       = in_stream->codecpar;

        if (par->codec_type != AVMEDIA_TYPE_VIDEO)
            continue;

        found_video = true;

        dec_codec = avcodec_find_decoder(par->codec_id);
        if (!dec_codec)
            LOG_ERROR("no decoder found for codec id %d", par->codec_id);

        dec_ctx = avcodec_alloc_context3(dec_codec);
        if (!dec_ctx)
            LOG_ERROR("could not allocate decoder context");

        ret = avcodec_parameters_to_context(dec_ctx, par);
        if (ret < 0)
            LOG_ERROR("could not copy codec parameters to decoder context");

        ret = avcodec_open2(dec_ctx, dec_codec, nullptr);
        if (ret < 0)
            LOG_ERROR("could not open decoder");

        /* Reserve a stream slot in the output container for this video. */
        out_stream = avformat_new_stream(out_fmt, nullptr);
        if (!out_stream)
            LOG_ERROR("could not create output stream");

        break; /* transcode first video stream only */
    }

    if (!found_video)
        LOG_ERROR("no video stream found in '%s'", inp_filename);

    /* ------------------------------------------------------------------ */
    /* Encoder: libx265, matching the decoded video's geometry             */
    /* ------------------------------------------------------------------ */

    const AVCodec  *enc_codec = avcodec_find_encoder_by_name("libx265");
    if (!enc_codec)
        LOG_ERROR("could not find encoder 'libx265'");

    AVCodecContext *enc_ctx = avcodec_alloc_context3(enc_codec);
    if (!enc_ctx)
        LOG_ERROR("could not allocate encoder context");

    av_opt_set(enc_ctx->priv_data, "preset", "slow", 0);
    av_opt_set(enc_ctx->priv_data, "crf",    "18",   0);

    /* Inherit geometry and timing from the decoder. */
    enc_ctx->width        = dec_ctx->width;
    enc_ctx->height       = dec_ctx->height;
    enc_ctx->framerate    = dec_ctx->framerate;
    enc_ctx->time_base    = av_inv_q(dec_ctx->framerate);
    enc_ctx->gop_size     = dec_ctx->gop_size;
    enc_ctx->pix_fmt      = dec_ctx->pix_fmt;
    enc_ctx->max_b_frames = 1;

    ret = avcodec_open2(enc_ctx, enc_codec, nullptr);
    if (ret < 0)
        LOG_ERROR("could not open encoder");

    /* Wire encoder parameters into the output stream. */
    out_stream->time_base        = enc_ctx->time_base;
    out_stream->codecpar->codec_tag = 0; /* let the muxer fill this in */

    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if (ret < 0)
        LOG_ERROR("could not copy encoder parameters to output stream");

    /* ------------------------------------------------------------------ */
    /* Open output file and write container header                          */
    /* ------------------------------------------------------------------ */

    ret = avio_open(&out_fmt->pb, out_filename, AVIO_FLAG_WRITE);
    if (ret < 0)
        LOG_ERROR("could not open output file '%s'", out_filename);

    ret = avformat_write_header(out_fmt, nullptr);
    if (ret < 0)
        LOG_ERROR("could not write container header");

    /* ------------------------------------------------------------------ */
    /* Transcode loop: read → decode → encode → write                      */
    /* ------------------------------------------------------------------ */

    AVPacket *packet = av_packet_alloc();
    AVFrame  *frame  = av_frame_alloc();
    if (!packet || !frame)
        LOG_ERROR("could not allocate packet/frame");

    while (av_read_frame(in_fmt, packet) >= 0) {
        ret = avcodec_send_packet(dec_ctx, packet);
        if (ret < 0)
            LOG_ERROR("could not send packet to decoder");

        av_packet_unref(packet);

        /* A single compressed packet can yield multiple decoded frames. */
        while (ret >= 0) {
            ret = avcodec_receive_frame(dec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if (ret < 0)
                LOG_ERROR("could not receive frame from decoder");

            encode_frame(enc_ctx, frame, packet, out_fmt);
            av_frame_unref(frame);
        }
    }

    /* Flush the encoder: send nullptr to signal end-of-stream. */
    encode_frame(enc_ctx, nullptr, packet, out_fmt);

    av_write_trailer(out_fmt);
    av_dump_format(out_fmt, 0, out_filename, 1);

    /* ------------------------------------------------------------------ */
    /* Cleanup                                                              */
    /* ------------------------------------------------------------------ */

    avio_closep(&out_fmt->pb);
    avcodec_free_context(&dec_ctx);
    avcodec_free_context(&enc_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avformat_free_context(out_fmt);
    avformat_close_input(&in_fmt);
}
