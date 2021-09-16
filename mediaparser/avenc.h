/*
 * avmuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#pragma once
#include <stdint.h>
#include <libavcodec/avcodec.h>
#define ENCODER_PACKET_FROM_FRAME
#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif

struct avencoder_context* avencoder_alloc_context();

void avencoder_free_contextp(struct avencoder_context** ppenc_ctx);

int avencoder_init(struct avencoder_context* penc_ctx, int codec_type, int codec_id, int param1, int param2, int param3, int format, int bitrate);

int avencoder_context_open(struct avencoder_context** ppenc_ctx, int codec_type, int codec_id, int param1, int param2, int praram3, int format, int bitrate);

int avencoder_encode_frame(struct avencoder_context* penc_ctx, AVFrame* pframe, char* pout_buf, int out_len);


int avencoder_encoder_data(struct avencoder_context* penc_ctx, char* pdata, int datalen, char* pout_buf, int out_len, long pts);

int avencoder_get_packet(struct avencoder_context* penc_ctx, char* pdata, int* datalen, long* pts);

int encoder_frame(int codecid, AVFrame* pframe, char* pout_buf, int out_len);

#ifdef ENCODER_PACKET_FROM_FRAME
int avencoder_encode_packet_from_frame(struct avencoder_context* penc_ctx, AVFrame* pframe,  struct AVPacket* pkt);
int encoder_packet_from_frame(int codecid, AVFrame* pframe, struct AVPacket* pkt);
#endif

//avencoder_encode_packet_from_frame
//int encoder_packet_from_frame(int codecid, AVFrame* pframe, char* pout_buf, int out_len);

//int avencoder_encode_packet_from_frame(int codecid, AVFrame* pframe, AVPacket* pkt);

AVFrame* decoder_packet(int mediatype, int codecid, char* ph264, int h264len);

int convert_h264_to_jpg(char* ph264, int h264len, char* pjpg, int jpglen);

int h264_keyframe_to_jpg_file(char* ph264, int h264len, char* pjpgfile);
