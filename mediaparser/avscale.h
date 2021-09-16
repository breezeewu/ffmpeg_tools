/*
 * avmuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#pragma once
#include <stdint.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
// scale message range: 2100-2199
#define LBSCALE_MSG_BASE                    2300
#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif
typedef struct avscale_context
{
    struct SwsContext*  pswsctx;
    
    int                 nsrc_width;
    int                 nsrc_height;
    enum AVPixelFormat  esrc_format;
    
    int                 ndst_width;
    int                 ndst_height;
    enum AVPixelFormat  edst_format;
    
    AVFrame*            pcopy_frame;
} avscale_ctx;

static avscale_ctx* avscale_open_context(int dstw, int dsth, int dstfmt);
static void avscale_close_context(avscale_ctx** ppsc);
static int avscale_frame(avscale_ctx* psc, AVFrame* pframe);

static avscale_ctx* avscale_open_context(int dstw, int dsth, int dstfmt)
{
    //lbtrace("avscale_open_context(dstw:%d, dsth:%d, dstfmt:%d)\n", dstw, dsth, dstfmt);
    avscale_ctx* psc = (avscale_ctx*)malloc(sizeof(avscale_ctx));
    memset(psc, 0, sizeof(avscale_ctx));
    psc->ndst_width = dstw;
    psc->ndst_height = dsth;
    psc->edst_format = (enum AVPixelFormat)dstfmt;
    
    return psc;
}

static void avscale_close_context(avscale_ctx** ppsc)
{
    //lbtrace("avscale_close_context(ppsc:%p)\n", ppsc);
    if(ppsc && *ppsc)
    {
        avscale_ctx* psc = *ppsc;
        // free frame list
        if(psc->pswsctx)
        {
            sws_freeContext(psc->pswsctx);
        }
        
        if(psc->pcopy_frame)
        {
            av_frame_free(&psc->pcopy_frame);
            psc->pcopy_frame = NULL;
        }
        free(psc);
        *ppsc = psc = NULL;
    }
}

static int avscale_from(avscale_ctx* psc, AVFrame* psrc_frame, AVFrame* pdst_frame)
{
    if(NULL == psc || NULL == psrc_frame || NULL == pdst_frame)
    {
        lberror("avscale_frame, Invalid parameter, psc:%p, psrc_frame:%p, pdst_frame:%p\n", psc, psrc_frame, pdst_frame);
        return -1;
    }
    int ret = -1;
    if(psc->ndst_width <= 0)
    {
        psc->ndst_width = psrc_frame->width;
    }

    if(psc->ndst_height <= 0)
    {
        psc->ndst_height = psrc_frame->height;
    }

    if(psc->edst_format < 0)
    {
        psc->edst_format = psrc_frame->format;
    }
    
    do {
        if(psrc_frame->width == psc->ndst_width && psrc_frame->height == psc->ndst_height && psrc_frame->format == psc->edst_format)
        {
            if(NULL == pdst_frame->data[0] || pdst_frame->width == psc->ndst_width || pdst_frame->height == psc->ndst_height || pdst_frame->format == psc->edst_format)
            {
                av_frame_unref(pdst_frame);
                pdst_frame->width = psc->ndst_width;
                pdst_frame->height = psc->ndst_height;
                pdst_frame->format = psc->edst_format;
                av_frame_get_buffer(pdst_frame, 0);
            }
            
            av_frame_copy(pdst_frame, psrc_frame);
            ret = pdst_frame->height;
            pdst_frame->pts = psrc_frame->pts;
            break;
        }
        if(NULL == psc->pswsctx || (psrc_frame->width != psc->nsrc_width || psrc_frame->height != psc->nsrc_height || psrc_frame->format != psc->esrc_format))
        {
            if(psc->pswsctx)
            {
                sws_freeContext(psc->pswsctx);
                psc->pswsctx = NULL;
            }

            psc->pswsctx = sws_getContext(psrc_frame->width, psrc_frame->height, (enum AVPixelFormat)psrc_frame->format, psc->ndst_width, psc->ndst_height, (enum AVPixelFormat)psc->edst_format, SWS_BILINEAR, NULL, NULL, NULL);
            if(NULL == psc->pswsctx)
            {
                lberror("psc->pswsctx:%p = sws_getContext(psrc_frame->width:%d, psrc_frame->height:%d, (enum AVPixelFormat)psrc_frame->format:%d, psc->ndst_width:%d, psc->ndst_height:%d, psc->edst_format:%d) failed!\n", psc->pswsctx, psrc_frame->width, psrc_frame->height, psrc_frame->format, psc->ndst_width, psc->ndst_height, psc->edst_format);
                ret = -1;
                break;
            }
            //lbtrace("psc->pswsctx:%p = sws_getContext(psc->nsrc_width:%d, psrc_frame->height:%d, (enum AVPixelFormat)psrc_frame->format:%d, psc->ndst_width:%d, psc->ndst_height:%d, psc->edst_format:%d)\n", psc->pswsctx, psc->nsrc_width, psrc_frame->height, psrc_frame->format, psc->ndst_width, psc->ndst_height, psc->edst_format);
            psc->nsrc_width = psrc_frame->width;
            psc->nsrc_height = psrc_frame->height;
            psc->esrc_format = psrc_frame->format;
            
        }
        
        if(NULL == pdst_frame->data[0] || pdst_frame->width != psc->ndst_width || pdst_frame->height != psc->ndst_height || pdst_frame->format != psc->edst_format)
        {
            av_frame_unref(pdst_frame);
            pdst_frame->width   = psc->ndst_width;
            pdst_frame->height  = psc->ndst_height;
            pdst_frame->format  = psc->edst_format;
            av_frame_get_buffer(pdst_frame, 16);
        }
        
        ret = sws_scale(psc->pswsctx, psrc_frame->data, psrc_frame->linesize, 0, psrc_frame->height, pdst_frame->data, pdst_frame->linesize);
        if(ret <= 0)
        {
            lberror("ret:%d = sws_scale(psc->pswsctx:%p, psrc_frame->data:%p, psrc_frame->linesize[0]:%d, 0, psrc_frame->height:%d, pdst_frame->data:%p, pdst_frame->linesize[0]:%d)\n", 
            ret, psc->pswsctx, psrc_frame->data, psrc_frame->linesize[0], psrc_frame->height, pdst_frame->data, pdst_frame->linesize[0]);
        }
    } while (0);
    pdst_frame->pts = psrc_frame->pts;
    /*int64_t dur = get_sys_time() - begin;
    static int64_t sumdur = 0;
    sumdur += dur;
    
    lbtrace("scale time:%"PRId64", sumdur:%"PRId64", src->pts:%"PRId64", dst->pts:%"PRId64"", dur, sumdur, psrc_frame->pts, pdst_frame->pts);*/
    return ret;
}

static int avscale_frame(avscale_ctx* psc, AVFrame* pframe)
{
    if(NULL == psc || NULL == pframe)
    {
        lberror("avscale_frame failed, invalid parameter, psc:%p, pframe:%p\n", psc, pframe);
        return -1;
    }
    
    if(NULL == psc->pcopy_frame)
    {
        psc->pcopy_frame = av_frame_alloc();
    }
    
    int ret = avscale_from(psc, pframe, psc->pcopy_frame);
    if(ret <= 0)
    {
        lberror("ret:%d = avscale_from(psc:%p, pframe:%p, psc->pcopy_frame:%p) failed\n", ret, psc, pframe, psc->pcopy_frame);
    }
    av_frame_unref(pframe);
    av_frame_move_ref(pframe, psc->pcopy_frame);
    return ret;
}

