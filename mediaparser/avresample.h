/*
 * avmuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#pragma once
#include <stdint.h>
#include <libavcodec/avcodec.h>

struct avresample_context* avresample_alloc_context();

void avresample_free_contextp(struct avresample_context** ppresample);

//struct avresample_context* avresample_init(struct avresample_context* presample, int src_channel, int src_samplerate, int src_format, int dst_channel, int dst_samplerate, int dst_format);
struct avresample_context* avresample_init(struct avresample_context* presample, int dst_channel, int dst_samplerate, int dst_format);
int avresample_resample(struct avresample_context* presample, AVFrame* pdstframe, AVFrame* psrcframe);

int confirm_swr_avaiable(struct avresample_context* presample, AVFrame* psrcframe);
//int resample(AVFrame* pdstframe, AVFrame* psrcframe);
