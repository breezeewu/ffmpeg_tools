/*
 * avmuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#pragma once
#include <stdint.h>
#include <libavformat/avformat.h>
#include "lbdemux.h"
#define AVMEDIA_PRINTF_MAIN_INFO        1
#define AVMEDIA_PRINTF_VIDEO_PACKET     2
#define AVMEDIA_PRINTF_AUDIO_PACKET     4
#define AVMEDIA_PRINTF_VIDEO_SH			8
#define AVMEDIA_PRINTF_AUDIO_SH			16

#define AVMEDIA_WRITE_VIDEO_PACKET		32
#define AVMEDIA_WRITE_AUDIO_PACKET		64

/*typedef struct 
{
    int         nindex;
    int         ncodec_type;
    int         ncodec_id;
    int         nwidth;
    int         nheight;
    int         nchannel;
    int         nsample_rate;
    int         nformat;
    int         nbitrate;
    int         nbit_depth;
    int         nb_frames;
    int64_t     lduration;
    AVRational  frame_rate;
    AVRational  aspect_rate;
    char        codec_name[128];
} STREAM_INFO;*/

struct avdemux_context* avdemux_alloc_context();

struct avdemux_context* avdemux_open_context(struct avdemux_context* pdemux, const char* purl);

lbdec_ctx* avdemux_open(const char* purl);

void avdemux_close_contextp(struct avdemux_context** ppdemux);

int64_t avdemux_get_duration(struct avdemux_context* pdemux, int mediatype);

int avdemux_seek(struct avdemux_context* pdemux, int64_t pts);

int avdemux_read_packet(struct avdemux_context* pdemux, AVPacket* pkt);

int avdemux_read_frame(struct avdemux_context* pdemux, AVFrame* pframe);

int avdemux_get_stream_info(struct avdemux_context* pdemux, int mediatype, struct stream_info* psi);

AVCodecContext* avdemux_open_stream(AVStream* pst);
//int avdemux_open_context(struct avdemux_context* pdemux, AVStream* pst);

int avdemux_decode_packet(struct avdemux_context* pdemux, AVPacket* pkt, AVFrame* pframe);

/*int avdemux_printf_main_info(struct avdemux_context* pdemux);

void avdemux_trace_info(struct avdemux_context* pdemux, int flag, int64_t start_time, int64_t dur_time);

void avdemux_trace_sequence_header(struct avdemux_context* pdemux, int flag);*/