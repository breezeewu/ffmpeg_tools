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
#include "lazylog.h"
#include "mutex.h"
#include "lbcomponent.h"
// scale message range: 2100-2199
#define LBSCALE_MSG_BASE                    2300
/*#define AVSCALE_MSG_BEGIN                   AVSCALE_MSG_BASE + 0    // param1:0, param2:0
#define AVSCALE_MSG_END                     AVSCALE_MSG_BASE + 1    // param1:0, param2:0
#define AVSCALE_MSG_SCALE_ERROR             AVSCALE_MSG_BASE + 2    // param1:0, param2:error code
#define AVSCALE_MSG_END_OF_STREAM           AVSCALE_MSG_BASE + 3    // param1:0, param2:0
#define AVSCALE_MSG_PROGRESS                AVSCALE_MSG_BASE + 4    // param1:0, param2:pts
*/
typedef struct lbscale_context
{
    lbcom_ctx           cc;
    struct SwsContext*  pswsctx;
    
    int                 nsrc_width;
    int                 nsrc_height;
    enum AVPixelFormat  esrc_format;
    
    int                 ndst_width;
    int                 ndst_height;
    enum AVPixelFormat  edst_format;
    
    AVFrame*            pcopy_frame;
} lbscale_ctx;

static lbscale_ctx* lbscale_open_context(int dstw, int dsth, int dstfmt);
static void lbscale_close_context(void* powner);
static int lbscale_handle_frame(void* powner, void* psrc, void** ppdst);
static int lbscale_frame(lbscale_ctx* psc, AVFrame* pframe);
/*static int scale_proc(void* powner)
{
    int ret = -1;
    lbscale_ctx* psc = (lbscale_ctx*)powner;
    if(NULL == psc || psc->ptc)
    {
        lberror("%s invalid parameter ptc:%p, psc->ptc:%p\n", __FUNCTION__, psc, psc->ptc);
        return -1;
    }
    lbcom_ctx* ptc = psc->ptc;
    lbcom_msg_callback(ptc, AVSCALE_MSG_BASE, 0, 0);
    while(ptc->brun)
    {
        if(list_size(ptc->plist_ctx) <= 0)
        {
            lazysleep(20);
        }
        
        AVFrame* pframe = pop(ptc->plist_ctx);
        if(pframe)
        {
            ret = avscale_frame(psc, pframe);
            if(rer <= 0)
            {
                lberror("ret:%d = avscale_frame(psc:%p, pframe:%p) failed!", ret, psc, pframe);
                continue;
            }
            else
            {
                lbcom_msg_callback(ptc, AVSCALE_MSG_PROGRESS, 0, (long)pframe->pts);
                lbcom_deliver_item(ptc->pdown_tc, pframe);
            }
        }
    }
    lbcom_msg_callback(ptc, AVSCALE_MSG_END, 0, 0);
    return 0;
}*/

static int lbscale_handle_frame(void* powner, void* psrc, void** ppdst)
{
    lbscale_ctx* psc = (lbscale_ctx*)powner;
    if(NULL == psc || NULL == ppdst)
    {
        lberror("invalid parameter, psc:%p, ppdst:%p\n", psc, ppdst);
        return -1;
    }
    
    if(NULL == psrc)
    {
        lbcom_msg_callback(&psc->cc, LBSCALE_MSG_BASE + LBCOMPONENT_MSG_END_OF_STREAM_OFFSET, 0, 0);
        lbtrace("lbscale end of stream!\n");
        return 0;
    }

    int ret = lbscale_frame(psc, (AVFrame*)psrc);
    if(ret <= 0)
    {
        lberror("%s ret:%d = avscale_frame(psc:%p, (AVFrame*)psrc:%p)\n", __FUNCTION__, ret, psc, psrc);
        lbcom_msg_callback(&psc->cc, LBSCALE_MSG_BASE + LBCOMPONENT_MSG_ERROR_OFFSET, 0, ret);
        return ret;
    }
    *ppdst = (AVFrame*)psrc;
    return ret;
}

static lbscale_ctx* lbscale_open_context(int dstw, int dsth, int dstfmt)
{
    lbtrace("avscale_open_context(dstw:%d, dsth:%d, dstfmt:%d)\n", dstw, dsth, dstfmt);
    lbscale_ctx* psc = (lbscale_ctx*)malloc(sizeof(lbscale_ctx));
    memset(psc, 0, sizeof(lbscale_ctx));
    psc->ndst_width = dstw;
    psc->ndst_height = dsth;
    psc->edst_format = (enum AVPixelFormat)dstfmt;
    lbcom_open_context(&psc->cc, AVMEDIA_TYPE_VIDEO, "scale", NULL, lbscale_handle_frame, lbscale_close_context, LBSCALE_MSG_BASE);
    psc->cc.pdur_ctx = lbduration_open_context("video scale");
    return psc;
}

static void lbscale_close_context(void* powner)
{
    lbtrace("avscale_close_context(psc:%p)\n", powner);
    if(powner)
    {
        lbscale_ctx* psc = (lbscale_ctx*)powner;
        lbcom_ctx* pcc = (lbcom_ctx*)psc;
        pthread_mutex_lock(pcc->pmutex);
        // free frame list
        if(pcc->plist_ctx)
        {
            while(list_size(pcc->plist_ctx) > 0)
            {
                AVFrame* pframe = (AVFrame*)pop(pcc->plist_ctx);
                if(pframe)
                {
                    av_frame_free(&pframe);
                }
            }
        }
        
        if(psc->pswsctx)
        {
            sws_freeContext(psc->pswsctx);
        }
        
        if(psc->pcopy_frame)
        {
            av_frame_free(&psc->pcopy_frame);
            psc->pcopy_frame = NULL;
        }
        pthread_mutex_unlock(pcc->pmutex);
    }
}

static void lbscale_free_context(lbscale_ctx** ppsc)
{
    if(ppsc && *ppsc)
    {
        lbscale_close_context(*ppsc);
        lbcom_free_context(ppsc);
    }
}

static int lbscale_from(lbscale_ctx* psc, AVFrame* psrc_frame, AVFrame* pdst_frame)
{
    if(NULL == psc || NULL == psrc_frame || NULL == pdst_frame)
    {
        lberror("avscale_frame, Invalid parameter, psc:%p, psrc_frame:%p, pdst_frame:%p\n", psc, psrc_frame, pdst_frame);
        return -1;
    }
    lbduration_begin(psc->cc.pdur_ctx);
    int ret = -1;
    int64_t begin = get_sys_time();
    pthread_mutex_lock(psc->cc.pmutex);
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
                lberror("psc->pswsctx:%p = sws_getContext(psc->nsrc_width:%d, psrc_frame->height:%d, (enum AVPixelFormat)psrc_frame->format:%d, psc->ndst_width:%d, psc->ndst_height:%d, psc->edst_format:%d) failed!\n", psc->pswsctx, psc->nsrc_width, psrc_frame->height, psrc_frame->format, psc->ndst_width, psc->ndst_height, psc->edst_format);
                ret = -1;
                break;
            }
            lbtrace("psc->pswsctx:%p = sws_getContext(psc->nsrc_width:%d, psrc_frame->height:%d, (enum AVPixelFormat)psrc_frame->format:%d, psc->ndst_width:%d, psc->ndst_height:%d, psc->edst_format:%d)\n", psc->pswsctx, psc->nsrc_width, psrc_frame->height, psrc_frame->format, psc->ndst_width, psc->ndst_height, psc->edst_format);
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
            lberror("ret:%d = sws_scale(psc->pswsctx:%p, psrc_frame->data:%p, psrc_frame->linesize, 0, psrc_frame->height:%d, pdst_frame->data:%p, pdst_frame->linesize:%p)\n", ret, psc->pswsctx, psrc_frame->data, psrc_frame->linesize, psrc_frame->height, pdst_frame->data, pdst_frame->linesize);
        }
    } while (0);
    pthread_mutex_unlock(psc->cc.pmutex);
    pdst_frame->pts = psrc_frame->pts;
    lbduration_end(psc->cc.pdur_ctx, 1);
    /*int64_t dur = get_sys_time() - begin;
    static int64_t sumdur = 0;
    sumdur += dur;
    
    lbtrace("scale time:%"PRId64", sumdur:%"PRId64", src->pts:%"PRId64", dst->pts:%"PRId64"", dur, sumdur, psrc_frame->pts, pdst_frame->pts);*/
    return ret;
}

static int lbscale_frame(lbscale_ctx* psc, AVFrame* pframe)
{
    if(NULL == psc || NULL == pframe)
    {
        lberror("avscale_frame failed, invalid parameter, psc:%p, pframe:%p\n", psc, pframe);
        return -1;
    }
    lbcom_ctx* pcc = (lbcom_ctx*)&psc->cc;
    pthread_mutex_lock(pcc->pmutex);
    if(NULL == psc->pcopy_frame)
    {
        psc->pcopy_frame = av_frame_alloc();
    }
    
    int ret = lbscale_from(psc, pframe, psc->pcopy_frame);
    if(ret <= 0)
    {
        lberror("ret:%d = avscale_from(psc:%p, pframe:%p, psc->pcopy_frame:%p) failed\n", ret, psc, pframe, psc->pcopy_frame);
    }
    av_frame_unref(pframe);
    av_frame_move_ref(pframe, psc->pcopy_frame);
    //pframe->pts = psc->pcopy_frame->pts;
    pthread_mutex_unlock(pcc->pmutex);
    return ret;
}

