/**
 * @file encoder.cpp
 * @brief Generates and encodes a synthetic YUV420P video to H.264/MP4.
 */

#include "encoder.h"
#include "utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
}

#include <cstdio>
#include <cstdint>

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Fills @p frame with a synthetic animated pattern for frame @p i.
 *
 * Luma (Y):  value = (x + y + i*3) mod 256 — diagonal stripes that scroll
 *            diagonally as i increases.
 * Cb (U):    (128 + x + y + i*3) mod 256
 * Cr (V):    ( 64 + x + y + i*3) mod 256
 *
 * The chroma planes are half-sized in both dimensions (YUV420P).
 *
 * @param frame Writable frame whose format must be AV_PIX_FMT_YUV420P.
 * @param i     Frame index (0-based).
 */
static void fill_yuv_frame(AVFrame *frame, int i)
{
    /* --- Luma (Y plane) --- */
    for (int y = 0; y < frame->height; y++) {
        uint8_t *row = frame->data[0] + y * frame->linesize[0];
        for (int x = 0; x < frame->width; x++)
            row[x] = static_cast<uint8_t>(x + y + i * 3);
    }

    /* --- Chroma (U and V planes, half resolution) --- */
    for (int y = 0; y < frame->height / 2; y++) {
        uint8_t *u_row = frame->data[1] + y * frame->linesize[1];
        uint8_t *v_row = frame->data[2] + y * frame->linesize[2];
        for (int x = 0; x < frame->width / 2; x++) {
            u_row[x] = static_cast<uint8_t>((128 + x + y + i * 3) % 256);
            v_row[x] = static_cast<uint8_t>(( 64 + x + y + i * 3) % 256);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void encode_h264(const char *out_filename)
{
    int ret = 0;

    /* --- Codec --- */
    const AVCodec *enc = avcodec_find_encoder_by_name("libx264");
    if (!enc)
        LOG_ERROR("cannot find encoder 'libx264'");

    AVCodecContext *enc_ctx = avcodec_alloc_context3(enc);
    if (!enc_ctx)
        LOG_ERROR("cannot allocate encoder context");

    /* Quality / speed settings */
    av_opt_set(enc_ctx->priv_data, "preset", "slow", 0);
    av_opt_set(enc_ctx->priv_data, "crf",    "18",   0);

    enc_ctx->width        = 360;
    enc_ctx->height       = 360;
    enc_ctx->framerate    = AVRational{25, 1};
    enc_ctx->time_base    = av_inv_q(enc_ctx->framerate);
    enc_ctx->pix_fmt      = AV_PIX_FMT_YUV420P;
    enc_ctx->gop_size     = 10;
    enc_ctx->max_b_frames = 1;

    ret = avcodec_open2(enc_ctx, enc, nullptr);
    if (ret < 0)
        LOG_ERROR("could not open encoder");

    /* --- Output container --- */
    AVFormatContext *out_fmt_ctx = nullptr;
    ret = avformat_alloc_output_context2(&out_fmt_ctx, nullptr, nullptr,
                                         out_filename);
    if (ret < 0)
        LOG_ERROR("could not allocate output format context");

    AVStream *out_stream = avformat_new_stream(out_fmt_ctx, nullptr);
    if (!out_stream)
        LOG_ERROR("could not create output stream");

    out_stream->time_base = enc_ctx->time_base;
    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if (ret < 0)
        LOG_ERROR("could not copy parameters from encoder context to stream");

    ret = avio_open(&out_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
    if (ret < 0)
        LOG_ERROR("could not open output file '%s'", out_filename);

    ret = avformat_write_header(out_fmt_ctx, nullptr);
    if (ret < 0)
        LOG_ERROR("could not write container header");

    /* --- Frame + packet buffers --- */
    AVFrame *frame = av_frame_alloc();
    if (!frame)
        LOG_ERROR("could not allocate frame");

    frame->width  = enc_ctx->width;
    frame->height = enc_ctx->height;
    frame->format = enc_ctx->pix_fmt;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0)
        LOG_ERROR("could not allocate frame buffer");

    AVPacket *pkt = av_packet_alloc();
    if (!pkt)
        LOG_ERROR("could not allocate packet");

    /* --- Encode loop (2500 frames = 100 seconds at 25 fps) --- */
    for (int i = 0; i < 2500; i++) {
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            LOG_ERROR("could not make frame writable");

        fill_yuv_frame(frame, i);
        frame->pts = i;

        encode_frame(enc_ctx, frame, pkt, out_fmt_ctx);
    }

    /* Flush the encoder */
    encode_frame(enc_ctx, nullptr, pkt, out_fmt_ctx);

    av_write_trailer(out_fmt_ctx);
    av_dump_format(out_fmt_ctx, 0, out_filename, 1);

    /* --- Cleanup --- */
    avio_closep(&out_fmt_ctx->pb);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&enc_ctx);
    avformat_free_context(out_fmt_ctx);
}
