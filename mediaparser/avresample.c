/*
 * avmuxer.c
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#include "avresample.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <unistd.h>
#include <assert.h>
#include <libswresample/swresample.h>

#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif
typedef struct avresample_context
{
    struct SwrContext* pswrctx;
    int         nsrc_channel;
    int         nsrc_samplerate;
    int         nsrc_format;
    int         ndst_channel;
    int         ndst_samplerate;
    int         ndst_format;
} resample_ctx;
struct avresample_context* avresample_alloc_context()
{
    struct avresample_context* pswrctx = (struct avresample_context*)malloc(sizeof(struct avresample_context));
    memset(pswrctx, 0, sizeof(struct avresample_context));
    return pswrctx;
}

void avresample_free_contextp(struct avresample_context** ppresample)
{
    if(ppresample && *ppresample)
    {
        struct avresample_context* presample = *ppresample;
        swr_free(&presample->pswrctx);
        free(presample);
        *ppresample = NULL;
    }
}

struct avresample_context* avresample_init(struct avresample_context* presample, int dst_channel, int dst_samplerate, int dst_format)
{
    if(NULL == presample)
    {
        presample = avresample_alloc_context();
    }
    
    if(presample->pswrctx)
    {
        swr_free(&presample->pswrctx);
    }
    
    /*presample->nsrc_channel = src_channel;
    presample->nsrc_samplerate = src_samplerate;
    presample->nsrc_format = src_format;*/
    presample->ndst_channel = dst_channel;
    presample->ndst_samplerate = dst_samplerate;
    presample->ndst_format = dst_format;
    /*struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
    int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
    int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
    int log_offset, void *log_ctx);*/
    /*presample->pswrctx = swr_alloc_set_opts(NULL, av_get_default_channel_layout(presample->ndst_channel), (enum AVSampleFormat)presample->ndst_format, presample->ndst_samplerate, av_get_default_channel_layout(presample->nsrc_channel), (enum AVSampleFormat)presample->nsrc_format, presample->nsrc_samplerate, 0, NULL);
    if(NULL == presample->pswrctx)
    {
        avresample_free_contextp(&presample);
    }*/
    return presample;
}

int avresample_resample(struct avresample_context* presample, AVFrame* pdstframe, AVFrame* psrcframe)
{
    if(psrcframe->channels == presample->ndst_channel && psrcframe->sample_rate == presample->ndst_samplerate && psrcframe->format == presample->ndst_format)
    {
        av_frame_move_ref(pdstframe, psrcframe);
        return pdstframe->nb_samples;
    }
    if(!confirm_swr_avaiable(presample, psrcframe))
    {
        return -1;
    }
    int nbsamples = (int)av_rescale_rnd(swr_get_delay(presample->pswrctx, presample->nsrc_samplerate) + psrcframe->nb_samples, presample->ndst_samplerate, presample->nsrc_samplerate, AV_ROUND_UP);
    if(pdstframe->nb_samples != nbsamples || pdstframe->channels != presample->ndst_channel || pdstframe->sample_rate != presample->ndst_samplerate || pdstframe->format != presample->ndst_format)
    {
        av_frame_unref(pdstframe);
        pdstframe->nb_samples = nbsamples;
        pdstframe->channels = presample->ndst_channel;
        pdstframe->channel_layout = av_get_default_channel_layout(presample->ndst_channel);
        pdstframe->format = presample->ndst_format;
        pdstframe->sample_rate = presample->ndst_samplerate;
        av_frame_get_buffer(pdstframe, 4);
        //return 0;
    }
    int ret = swr_convert(presample->pswrctx, pdstframe->data, nbsamples, psrcframe->data, psrcframe->nb_samples);
    
    return ret;
}

int confirm_swr_avaiable(struct avresample_context* presample, AVFrame* psrcframe)
{
    if(NULL == presample->pswrctx || presample->nsrc_channel != psrcframe->channels || presample->nsrc_samplerate != psrcframe->sample_rate || presample->nsrc_format != psrcframe->format)
    {
        if(presample->pswrctx)
        {
            swr_free(&presample->pswrctx);
        }
        presample->nsrc_channel = psrcframe->channels;
        presample->nsrc_samplerate = psrcframe->sample_rate;
        presample->nsrc_format = psrcframe->format;
        presample->pswrctx = swr_alloc_set_opts(NULL, av_get_default_channel_layout(presample->ndst_channel), (enum AVSampleFormat)presample->ndst_format, presample->ndst_samplerate, av_get_default_channel_layout(presample->nsrc_channel), (enum AVSampleFormat)presample->nsrc_format, presample->nsrc_samplerate, 0, NULL);
        if(NULL == presample->pswrctx)
        {
            lberror("swr_alloc_set_opts failed!\n");
            return 0;
        }
        int ret = swr_init(presample->pswrctx);
        if(ret < 0)
        {
            lberror("swr_init failed, ret:%d\n", ret);
            return ret;
        }
        
        return 0;
    }
    
    return 1;
}
