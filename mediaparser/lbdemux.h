/*
 * avmuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#pragma once
#include <stdint.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#define AVMEDIA_PRINTF_MAIN_INFO        1
#define AVMEDIA_PRINTF_VIDEO_PACKET     2
#define AVMEDIA_PRINTF_AUDIO_PACKET     4
#define AVMEDIA_PRINTF_VIDEO_SH			8
#define AVMEDIA_PRINTF_AUDIO_SH			16

#define AVMEDIA_WRITE_VIDEO_PACKET		32
#define AVMEDIA_WRITE_AUDIO_PACKET		64
//typedef int (*demux_open_func)(char* purl);

typedef struct
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
	char		sequence_header[512];
	int			nseq_hdr_len;
} stream_info;

typedef int (*demux_read_packet)(void* pdc, AVPacket* pkt);
typedef int (*demux_read_frame)(void* pdc, AVFrame* pframe);
typedef int (*demux_seek)(void* pdc, int64_t pts);
typedef int (*demux_get_stream_info)(void* pdc, int mt, stream_info* psi);
typedef void (*demux_close)(void** ppdc);



typedef struct
{
	void*	powner;

	demux_read_packet		pread_packet;
	demux_read_frame		pread_frame;
	demux_seek				pseek_packet;
	demux_close				pclose_context;
	demux_get_stream_info	pget_stream_info;

} lbdec_ctx;

lbdec_ctx* lbdemux_open_context(char* purl);

void lbdemux_close_contextp(lbdec_ctx** ppdc);

int lbdemux_read_packet(lbdec_ctx* pdc, AVPacket* pkt);

int lbdemux_read_frame(lbdec_ctx* pdc, AVFrame* pframe);

int lbdemux_seek_packet(lbdec_ctx* pdc, int64_t pts);

int lbdemux_get_stream_info(lbdec_ctx* pdc, int mt, stream_info* psi);


/*void avdemux_close_contextp(struct avdemux_context** ppdemux);

int64_t avdemux_get_duration(struct avdemux_context* pdemux, int mediatype);

int avdemux_seek(struct avdemux_context* pdemux, int64_t pts);

int avdemux_read_packet(struct avdemux_context* pdemux, AVPacket* pkt);

int avdemux_read_frame(struct avdemux_context* pdemux, AVFrame* pframe);

int avdemux_get_stream_info(struct avdemux_context* pdemux, int mediatype, struct stream_info* psi);

AVCodecContext* avdemux_open_stream(AVStream* pst);
//int avdemux_open_context(struct avdemux_context* pdemux, AVStream* pst);

int avdemux_decode_packet(struct avdemux_context* pdemux, AVPacket* pkt, AVFrame* pframe);*/

int avdemux_printf_main_info(lbdec_ctx* pdc);

void avdemux_trace_info(lbdec_ctx* pdc, int flag, int64_t start_time, int64_t dur_time);

void avdemux_trace_sequence_header(lbdec_ctx* pdc, int flag);