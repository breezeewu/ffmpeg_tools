/*
 * avmuxer.c
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include "avdemux.h"

//#include "avenc.h"
//#include "cachebuf.h"
//#include "avresample.h"
//#include "list.h"
//#include "audio_suquence_header.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "libavutil/imgutils.h"
#include <libavutil/samplefmt.h>
#include "audio_suquence_header.h"
#define _CRT_SECURE_NO_WARNINGS
#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif
AVRational stdtb = { 1, AV_TIME_BASE };
typedef struct avdemux_context
{
    AVFormatContext*            pfmt_ctx;
    pthread_mutex_t*            pmutex;
    
	stream_info*    pst_list;
    int             nstream_num;
    int             nvindex;
    int             naindex;

    AVCodecContext*	 pvideo_codecctx;
    AVCodecContext*	 paudio_codecctx;

    int64_t     lmedia_duration;
    
} enc_ctx;

struct avdemux_context* avdemux_alloc_context()
{
    struct avdemux_context* pdemux = malloc(sizeof(struct avdemux_context));
    memset(pdemux, 0, sizeof(struct avdemux_context));
    pdemux->pmutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    memset(pdemux->pmutex, 0, sizeof(pthread_mutex_t));

    pthread_mutexattr_t attr;
    memset(&attr, 0, sizeof(attr));
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(pdemux->pmutex, &attr);

    return pdemux;
}

struct avdemux_context* avdemux_open_context(struct avdemux_context* pdemux, const char* purl)
{
    int ret = -1;
    if(NULL == pdemux)
    {
        pdemux = avdemux_alloc_context();
    }

    if(NULL == pdemux)
    {
        return NULL;
    }

    do
    {
        int i;
        ret = avformat_open_input(&pdemux->pfmt_ctx, purl, NULL, NULL);
        //lbtrace("ret:%d = avformat_open_input(&pdemux->pfmt_ctx:%p, purl:%s, NULL, NULL)\n", ret, pdemux->pfmt_ctx, purl);
        if(ret != 0)
        {
            lberror("ret:%d = avformat_open_input(&pdemux->pfmt_ctx, purl:%s, NULL, NULL) failed\n", ret, purl);
            break;
        }

        ret = avformat_find_stream_info(pdemux->pfmt_ctx, NULL);
        if(ret != 0)
        {
            lberror("ret:%d = avformat_find_stream_info(&pdemux->pfmt_ctx:%p, NULL) failed\n", ret, pdemux->pfmt_ctx);
            break;
        }
        pdemux->nstream_num = pdemux->pfmt_ctx->nb_streams;
        pdemux->pst_list = (stream_info*)malloc(sizeof(stream_info)*pdemux->nstream_num);
        memset(pdemux->pst_list, 0, sizeof(stream_info) * pdemux->pfmt_ctx->nb_streams);
        for(i = 0; i < pdemux->pfmt_ctx->nb_streams; i++)
        {
            AVStream* pst = pdemux->pfmt_ctx->streams[i];
            AVCodecParameters*  pcodecptr = pst->codecpar;
            AVCodec* pcodec = avcodec_find_decoder(pcodecptr->codec_type);

            pdemux->pst_list[i].nindex = i;
            pdemux->pst_list[i].ncodec_type   = pcodecptr->codec_type;
            pdemux->pst_list[i].ncodec_id   = pcodecptr->codec_id;
            pdemux->pst_list[i].nformat = pcodecptr->format;
            pdemux->pst_list[i].lduration = av_rescale_q(pst->duration, pst->time_base, stdtb);
            pdemux->pst_list[i].nbitrate = (int)pcodecptr->bit_rate;
            pdemux->pst_list[i].nb_frames = pst->nb_frames;
            if(AVMEDIA_TYPE_VIDEO == pcodecptr->codec_type)
            {
                pdemux->pst_list[i].nwidth = pcodecptr->width;
                pdemux->pst_list[i].nheight = pcodecptr->height;
                pdemux->pst_list[i].frame_rate = pst->avg_frame_rate;
                pdemux->pst_list[i].aspect_rate = pst->sample_aspect_ratio;
                printf("width:%d, height:%d\n", pcodecptr->width, pcodecptr->height);
            }
            else if(AVMEDIA_TYPE_AUDIO == pcodecptr->codec_type)
            {
                 pdemux->pst_list[i].nchannel = pcodecptr->channels;
                pdemux->pst_list[i].nsample_rate = pcodecptr->sample_rate;
            }

            if(pcodec)
            {
                memcpy(pdemux->pst_list[i].codec_name, pcodec->name, strlen(pcodec->name)+1);
            }
        }

        pdemux->nvindex = av_find_best_stream(pdemux->pfmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if(pdemux->nvindex >= 0)
        {
            AVStream* pst = pdemux->pfmt_ctx->streams[pdemux->nvindex];
            pdemux->pvideo_codecctx = avdemux_open_stream(pst);
            if(NULL == pdemux->pvideo_codecctx)
            {
                lberror("pdemux->pvideo_codecctx:%p = avdemux_open_stream(pst:%p)\n", pdemux->pvideo_codecctx, pst);
            }
        }
        
        pdemux->naindex = av_find_best_stream(pdemux->pfmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        if(pdemux->naindex >= 0)
        {
            AVStream* pst = pdemux->pfmt_ctx->streams[pdemux->naindex];
            pdemux->paudio_codecctx = avdemux_open_stream(pst);
            if(NULL == pdemux->paudio_codecctx)
            {
                lberror("pdemux->paudio_codecctx:%p = avdemux_open_stream(pst:%p)\n", pdemux->paudio_codecctx, pst);
            }
        }
        /*
        //lbtrace("ret:%d = av_find_best_stream\n", ret);
        if(ret >= 0)
        {
            AVStream* pst = pdemux->pfmt_ctx->streams[ret];
            AVCodecParameters*  pcodecptr = pst->codecpar;
            pdemux->video_info.nindex = ret;
            pdemux->video_info.ncodec_id = pcodecptr->codec_id;
            pdemux->video_info.nwidth = pcodecptr->width;
            pdemux->video_info.nheight = pcodecptr->height;
            pdemux->video_info.nformat = pcodecptr->format;
            pdemux->video_info.lduration = av_rescale_q(pst->duration, pst->time_base, stdtb);
            //lbtrace("width:%d, height:%d, extradata:%p, extradata_size:%d, vduration:%ld\n", pdemux->video_info.nwidth, pdemux->video_info.nheight, pcodecptr->extradata, pcodecptr->extradata_size, pdemux->video_info.lduration);
            if(pcodecptr->extradata_size > 0)
            {
                pdemux->pvextra_data = (char*)malloc(pcodecptr->extradata_size);
                memcpy(pdemux->pvextra_data, pcodecptr->extradata, pcodecptr->extradata_size);
                pdemux->nvextra_data_size = pcodecptr->extradata_size;
            }

            pdemux->pvideo_codecctx = avdemux_open_stream(pst);
            if(NULL == pdemux->pvideo_codecctx)
            {
                lberror("pdemux->pvideo_codecctx:%p = avdemux_open_stream(pst:%p)\n", pdemux->pvideo_codecctx, pst);
            }
        }
        else
        {
            pdemux->video_info.nindex = -1;
        }
        

        // parser video stream info
        
        
        // parser audio stream info
        ret = av_find_best_stream(pdemux->pfmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        if(ret < 0)
        {
            lberror("ret:%d = av_find_best_stream(&pdemux->pfmt_ctx:%p, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0) failed\n", ret, pdemux->pfmt_ctx);
            break;
        }
        if(ret >= 0)
        {
            AVStream* pst = pdemux->pfmt_ctx->streams[ret];
            AVCodecParameters* pcodecptr = pst->codecpar;
            pdemux->audio_info.nindex = ret;
            pdemux->audio_info.ncodec_id = pcodecptr->codec_id;
            pdemux->audio_info.nchannel = pcodecptr->channels;
            pdemux->audio_info.nsamplerate = pcodecptr->sample_rate;
            pdemux->audio_info.nformat = pcodecptr->format;
            pdemux->audio_info.lduration = av_rescale_q(pst->duration, pst->time_base, stdtb);
            if(pcodecptr->extradata_size > 0)
            {
                pdemux->paextra_data = (char*)malloc(pcodecptr->extradata_size);
                memcpy(pdemux->paextra_data, pcodecptr->extradata, pcodecptr->extradata_size);
                pdemux->naextra_data_size = pcodecptr->extradata_size;
            }
            
            pdemux->paudio_codecctx = avdemux_open_stream(pst);
            if(NULL == pdemux->paudio_codecctx)
            {
                lberror("pdemux->paudio_codecctx:%p = avdemux_open_stream(pst:%p)\n", pdemux->paudio_codecctx, pst);
            }
        }
        else
        {
            pdemux->audio_info.nindex = -1;
        }*/
        
        if(pdemux->nvindex < 0 && pdemux->naindex <  0)
        {
            lberror("No available video or audio stream found, pdemux->nvindex:%d, pdemux->naindex:%d\n", pdemux->nvindex, pdemux->naindex);
            return NULL;
        }
        // media info
        pdemux->lmedia_duration = pdemux->pfmt_ctx->duration;
        ret = 0;
    }
    while(0);

    if(ret != 0)
    {
        avdemux_close_contextp(&pdemux);

    }

    return pdemux;
}

lbdec_ctx* avdemux_open(const char* purl)
{
	lbdec_ctx* pdc = NULL;
	struct avdemux_context* padc = avdemux_open_context(pdc, purl);
	if (NULL == padc)
	{
		return NULL;
	}

	pdc = (lbdec_ctx*)malloc(sizeof(lbdec_ctx));
	pdc->pread_packet = avdemux_read_packet;
	pdc->pread_frame = avdemux_read_frame;
	pdc->pseek_packet = avdemux_seek;
	pdc->pclose_context = avdemux_close_contextp;
	pdc->pget_stream_info = avdemux_get_stream_info;

	return pdc;
}
void avdemux_close_contextp(struct avdemux_context** ppdemux)
{
    //lbtrace("avdemux_close_contextp(ppdemux:%p)\n", ppdemux);
    if(ppdemux && *ppdemux)
    {
        struct avdemux_context* pdemux = *ppdemux;
        if(pdemux->pfmt_ctx)
        {
            //lbtrace("before avformat_close_input pdemux->pfmt_ctx:%p\n", pdemux->pfmt_ctx);
            avformat_close_input(&pdemux->pfmt_ctx);
            
        }

        if(pdemux->pvideo_codecctx)
        {
            avcodec_free_context(&pdemux->pvideo_codecctx);
        }
        //lbtrace("after avcodec_free_context pdemux->pvideo_codecctx:%p\n", pdemux->pvideo_codecctx);
        if(pdemux->paudio_codecctx)
        {
            avcodec_free_context(&pdemux->paudio_codecctx);
        }
        //lbtrace("after avcodec_free_context pdemux->paudio_codecctx:%p\n", pdemux->paudio_codecctx);
        pthread_mutex_destroy(pdemux->pmutex);
        //lbtrace("after pthread_mutex_destroy\n");
        free(pdemux->pmutex);
        free(pdemux);
        *ppdemux = pdemux = NULL;
    }
    //lbtrace("avdemux_close_contextp(ppdemux) end\n");
}

int64_t avdemux_get_duration(struct avdemux_context* pdemux, int mediatype)
{
    if(NULL == pdemux)
    {
        return -1;
    }

    if(AVMEDIA_TYPE_VIDEO == mediatype && pdemux->nvindex >= 0)
    {
		return pdemux->pst_list[pdemux->nvindex].lduration;
		
    }
    else if(AVMEDIA_TYPE_AUDIO == mediatype  && pdemux->naindex >= 0)
    {
        return pdemux->pst_list[pdemux->naindex].lduration;
    }
    else
    {
        return pdemux->lmedia_duration;
    }
    
}

int avdemux_seek(struct avdemux_context* pdemux, int64_t pts)
{
    if(NULL == pdemux || NULL == pdemux->pfmt_ctx)
    {
        return -1;
    }
    int seekflag = 0;//|AVSEEK_FLAG_BACKWARD;//AVSEEK_FLAG_BACKWARD;//AVSEEK_FLAG_ANY;//AVSEEK_FLAG_FRAME;//AVSEEK_FLAG_BYTE;
    if(pdemux->pfmt_ctx->iformat && 0 == strcmp(pdemux->pfmt_ctx->iformat->name, "mpegts"))
    {
        pts  = pts > 2000000 ? pts - 2000000 : 0;
    }
    /*int seek_by_bytes = 0;//!!(pdemux->pfmt_ctx->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", pdemux->pfmt_ctx->iformat->name);
    if(seek_by_bytes)
    {
        seek_by_bytes = AVSEEK_FLAG_BYTE;
    }*/
    int ret = av_seek_frame(pdemux->pfmt_ctx, -1, pts, seekflag);
    //int ret = avformat_seek_file(pdemux->pfmt_ctx, -1, INT64_MIN, pts, pts, AVSEEK_FLAG_BACKWARD);//av_seek_frame(pdemux->pfmt_ctx, -1, pts, seekflag);//
    //lbtrace("ret:%d = avformat_seek_file(pdemux->pfmt_ctx:%p, -1, INT64_MIN, pts:%ld, INT64_MAX, seekflag:%d)\n", ret, pdemux->pfmt_ctx, pts, seekflag);
    return ret;
}

int avdemux_read_packet(struct avdemux_context* pdemux, AVPacket* pkt)
{
    if(NULL == pdemux || NULL == pdemux->pfmt_ctx || NULL == pkt)
    {
        assert(0);
        return -1;
    }
    int ret = av_read_frame(pdemux->pfmt_ctx, pkt);
    //lbtrace("ret:%d = av_read_frame(pdemux->pfmt_ctx, pkt), index:%d, flags:%d, pts:%ld, stdpts:%ld, size:%d\n", ret, pkt->stream_index, pkt->flags, pkt->pts, av_rescale_q(pkt->pts, pdemux->pfmt_ctx->streams[pkt->stream_index]->time_base, stdtb), pkt->size);
    if (0 == ret)
	{
		int64_t pts = pkt->pts;
		AVStream* pst = pdemux->pfmt_ctx->streams[pkt->stream_index];
		
		if (-1 != pkt->pts)
		{
			pkt->pts = av_rescale_q(pkt->pts, pdemux->pfmt_ctx->streams[pkt->stream_index]->time_base, stdtb);
		}
		if (-1 != pkt->dts)
		{
			pkt->dts = av_rescale_q(pkt->dts, pdemux->pfmt_ctx->streams[pkt->stream_index]->time_base, stdtb);
		}
		//lbtrace("idx:%d, pts:%lld, st_num:%d, st_den:%d\n", pkt->stream_index, pts, pst->time_base.num, pst->time_base.den);
	}

    return ret;
}

int avdemux_read_frame(struct avdemux_context* pdemux, AVFrame* pframe)
{
    if(NULL == pdemux || NULL == pdemux->pfmt_ctx || NULL == pframe)
    {
        assert(0);
        return -1;
    }

    AVCodecContext* pcodecctx = NULL;
    AVPacket* pkt = av_packet_alloc();
    int ret = av_read_frame(pdemux->pfmt_ctx, pkt);
    if (0 == ret)
	{
		if (AV_NOPTS_VALUE != pkt->pts)
		{
			pkt->pts = av_rescale_q(pkt->pts, pdemux->pfmt_ctx->streams[pkt->stream_index]->time_base, stdtb);
		}
		if (AV_NOPTS_VALUE != pkt->dts)
		{
			pkt->dts = av_rescale_q(pkt->dts, pdemux->pfmt_ctx->streams[pkt->stream_index]->time_base, stdtb);
		}
	}

    if(pdemux->nvindex == pkt->stream_index)
    {
        pcodecctx = pdemux->pvideo_codecctx;
    }

    else if(pdemux->naindex == pkt->stream_index)
    {
        pcodecctx = pdemux->paudio_codecctx;
    }
    
    if(NULL == pcodecctx)
    {
        return -1;
    }

    ret = avcodec_send_packet(pcodecctx, pkt);
    if(ret < 0)
    {
        lberror("ret:%d = avcodec_send_packet(pcodecctx:%p, &pkt:%p) faield\n", ret, pcodecctx, pkt);
        return ret;
    }
    
    ret = avcodec_receive_frame(pcodecctx, pframe);
    if(AVERROR(EAGAIN) == ret)
    {
        return ret;
    }
    else if(0 != ret)
    {
        lberror("ret:%d = avcodec_receive_frame(pcodecctx:%p, pframe:%p)\n", ret, pcodecctx, pframe);
        return ret;
    }

    return ret;
}

int avdemux_get_stream_info(struct avdemux_context* pdemux, int mediatype, stream_info* psi)
{
    if(NULL == pdemux || NULL == psi)
    {
        lberror("Invalid parameter, pdemux:%p, psi:%p\n", pdemux, psi);
        return -1;
    }

    if(AVMEDIA_TYPE_VIDEO == mediatype && pdemux->nvindex >= 0)
    {
        *psi = pdemux->pst_list[pdemux->nvindex];
    }
    else if(AVMEDIA_TYPE_AUDIO == mediatype && pdemux->naindex >= 0)
    {
        *psi = pdemux->pst_list[pdemux->naindex];
    }
    else
    {
        psi->nindex = -1;
        return -1;
    }
    
    
    return 0;
}

AVCodecContext* avdemux_open_stream(AVStream* pst)
{
    int ret = -1;
    AVCodecContext* pcodecctx = NULL;
    if(NULL == pst)
    {
        assert(0);
        return NULL;
    }
    do
    {
        AVCodec* pcodec = avcodec_find_decoder(pst->codecpar->codec_id);
        if(NULL == pcodec)
        {
            lberror(" pcodec:%p = avcodec_find_decoder(pst->codecpar->codec_id:%d)\n",  pcodec, pst->codecpar->codec_id);
            break;
        }
        pcodecctx = avcodec_alloc_context3(pcodec);
        if(NULL == pcodecctx)
        {
            lberror("pcodecctx:%p = avcodec_alloc_context3(pcodec) failed\n", pcodecctx);
            break;
        }
        //lbtrace("before avcodec_get_context_defaults3\n");
        //avcodec_get_context_defaults3(pcodecctx, pcodec);
        ret = avcodec_parameters_to_context(pcodecctx, pst->codecpar);
        //lbtrace("ret:%d = avcodec_parameters_to_context\n", ret);
        if(ret < 0)
        {
            lberror("ret:%d = avcodec_parameters_to_context(pst->codecpar:%p, pcodecctx:%p)\n", ret, pst->codecpar, pcodecctx);
            break;
        }
        if(pst->codecpar->extradata && pst->codecpar->extradata_size > 0)
        {
            pcodecctx->extradata = (uint8_t*)av_malloc(pst->codecpar->extradata_size);
            memcpy(pcodecctx->extradata, pst->codecpar->extradata, pst->codecpar->extradata_size);
            pcodecctx->extradata_size = pst->codecpar->extradata_size;
        }
        //lbtrace("mt:%d pextradata:%p, len:%d\n", pcodecctx->codec_type, pcodecctx->extradata, pcodecctx->extradata_size);
        ret = avcodec_open2(pcodecctx, pcodec, NULL);
        if(ret < 0)
        {
            lberror("ret:%d = avcodec_open2(pcodecctx:%p, pcodec:%p, NULL)\n", ret, pcodecctx, pcodec);
            break;
        }
    } while(0);
    
   if(ret < 0)
   {
       avcodec_free_context(&pcodecctx);
       pcodecctx = NULL;
   }

    return pcodecctx;
}

int avdemux_decode_packet(struct avdemux_context* pdemux, AVPacket* pkt, AVFrame* pframe)
{
    int ret = 0;
     AVCodecContext* pcodecctx = NULL;
    if(NULL == pdemux || NULL == pkt || NULL == pframe)
    {
        lberror("Invalid parameter, pdemux:%p, pkt:%p, pframe:%p\n", pdemux, pkt, pframe);
        return -1;
    }

    if(pdemux->nvindex == pkt->stream_index)
    {
        pcodecctx = pdemux->pvideo_codecctx;
    }
    else if(pdemux->naindex == pkt->stream_index)
    {
        pcodecctx = pdemux->paudio_codecctx;
    }
    
    if(NULL == pcodecctx)
    {
        return -1;
    }
    ret = avcodec_send_packet(pcodecctx, pkt);
    if(ret < 0)
    {
        lberror("ret:%d = avcodec_send_packet(pcodecctx:%p, &pkt:%p) faield\n", ret, pcodecctx, pkt);
        return ret;
    }
    
    ret = avcodec_receive_frame(pcodecctx, pframe);
    if(AVERROR(EAGAIN) == ret)
    {
        return 0;
    }
    else if(0 != ret)
    {
        lberror("ret:%d = avcodec_receive_frame(pcodecctx:%p, pframe:%p)\n", ret, pcodecctx, pframe);
        return ret;
    }

    return ret;
}

/*
int avdemux_trace_main_info(struct avdemux_context* pdemux)
{
    int i;
    char* st_name_list[] = {"unknown", "video", "audio", "data", "subtitle", "attachment"};
    if(NULL == pdemux || NULL == pdemux->pfmt_ctx)
    {
        lberror("Invalid parameter, pdemux:%p,  pdemux->pfmt_ctx:%p\n", pdemux,  pdemux ? pdemux->pfmt_ctx : NULL);
        return -1;
    }

    for(i = 0; i < pdemux->nstream_num; i++)
    {
        lbtrace("stream %d: %s\n", pdemux->pst_list[i].nindex, st_name_list[pdemux->pst_list[i].ncodec_type+1]);
        lbtrace("codec name: %s\n", pdemux->pst_list[i].codec_name);
        lbtrace("bitrate:%d\n", pdemux->pst_list[i].nbitrate);
        lbtrace("duration:%lf\n", (double)pdemux->pst_list[i].lduration/1000000);
        lbtrace("nb_frames:%d\n", pdemux->pst_list[i].nb_frames);
        if(AVMEDIA_TYPE_VIDEO == pdemux->pst_list[i].ncodec_type)
        {
            lbtrace("width:%d\n", pdemux->pst_list[i].nwidth);
            lbtrace("height:%d\n", pdemux->pst_list[i].nheight);
            lbtrace("pixel format:%d\n", pdemux->pst_list[i].nformat);
            lbtrace("video frame rate: %d:%d\n", pdemux->pst_list[i].frame_rate.num, pdemux->pst_list[i].frame_rate.den);
            lbtrace("video aspect rate: %d:%d\n", pdemux->pst_list[i].aspect_rate.num, pdemux->pst_list[i].aspect_rate.den);
        }
        else if(AVMEDIA_TYPE_AUDIO == pdemux->pst_list[i].ncodec_type)
        {
            lbtrace("channel:%d\n", pdemux->pst_list[i].nchannel);
            lbtrace("sample rate:%d\n", pdemux->pst_list[i].nsample_rate);
            lbtrace("sample format:%d\n", pdemux->pst_list[i].nformat);
        }
        printf("\n");
    }

	return 0;
}

void avdemux_trace_info(struct avdemux_context* pdemux, int flag, int64_t start_time, int64_t dur_time)
{
    if(NULL == pdemux || NULL == pdemux->pfmt_ctx)
    {
        lberror("Invalid parameter, pdemux:%p,  pdemux->pfmt_ctx:%p\n", pdemux,  pdemux ? pdemux->pfmt_ctx : NULL);
        return -1;
    }

    if(flag & AVMEDIA_PRINTF_MAIN_INFO)
    {
        avdemux_trace_main_info(pdemux);
    }
    int vidx = -1;
    int aidx = -1;
    int veof = 1;
    int aeof = 1;
    int vnum = 0, anum = 0;
	FILE* pvfile = NULL;
	FILE* pafile = NULL;
    int64_t end_time = 0;
    if(dur_time > 0)
    {
        end_time = start_time + dur_time;
    }
    if(flag & AVMEDIA_PRINTF_VIDEO_PACKET  && pdemux->nvindex >= 0)
    {
        vidx = pdemux->nvindex;
        veof = 0;
    }
    if((flag & AVMEDIA_PRINTF_AUDIO_PACKET) && pdemux->naindex >= 0)
    {
        aidx = pdemux->naindex;
        aeof = 0;
    }
	if (flag & AVMEDIA_WRITE_VIDEO_PACKET  && pdemux->nvindex >= 0)
	{
		vidx = pdemux->nvindex;
		veof = 0;
		pvfile = fopen("video.data", "wb");
	}
	if ((flag & AVMEDIA_WRITE_AUDIO_PACKET) && pdemux->naindex >= 0)
	{
		aidx = pdemux->naindex;
		aeof = 0;
		pafile = fopen("audio.data", "wb");
	}
    if(vidx >= 0 || aidx >= 0)
    {
        //lbtrace("mt\tseq\tpts\tflag\tsize\n");
        lbtrace("%-5s  %5s   %s   %s  %6s\n", "mt", "seq", "pts", "flag", "size");
        //lbtrace("mediatype  index   pts   keyflag   size\n");
        AVPacket* pkt = av_packet_alloc();
        if(start_time > 0)
        {
            avdemux_seek(pdemux, start_time);
        }
        while(1)
        {
            int ret = avdemux_read_packet(pdemux, pkt);
            //lbtrace("ret:%d = avdemux_read_packet(pdemux, pkt), flags:%d, pts:%ld, size:%d\n", ret, pkt->flags, pkt->pts, pkt->size);
            if(ret < 0)
            {
                break;
            }
            if(start_time > 0 && start_time >= pkt->pts)
            {
                av_packet_unref(pkt);
                continue;
            }

             //lbtrace("mediatype  index    pts     keyflag     size\n");
            if(pkt->stream_index == vidx)
            {
                if(end_time > 0 && end_time < pkt->pts)
                {
                    veof = 1;
                }
                else
                {
					if (flag & AVMEDIA_PRINTF_VIDEO_PACKET)
					{
						lbtrace("video  %5d  %5.3f  %4d  %6d\n", vnum++, (float)pkt->pts / 1000000, pkt->flags, pkt->size);
					}
					if (pvfile)
					{
						fwrite(pkt->data, 1, pkt->size, pvfile);
					}
                    
                }
            }
            else if(pkt->stream_index == aidx)
            {
                if(end_time > 0 && end_time < pkt->pts)
                {
                    aeof = 1;
                }
                else
                {
					adts_ctx* pac = adts_demux_open(pkt->data, pkt->size);
					if (pac)
					{
						printf("channel:%d, samplerate:%d\n", pac->channel_configuration, pac->samplerate);
						adts_demux_close(&pac);
					}

					if (flag & AVMEDIA_PRINTF_VIDEO_PACKET)
					{
						lbtrace("audio  %5d  %5.3f  %4d  %6d\n", anum++, (float)pkt->pts / 1000000, pkt->flags, pkt->size);
					}
					if (pafile)
					{
						fwrite(pkt->data, 1, pkt->size, pafile);
					}
                    
                }
            }
            av_packet_unref(pkt);
            if(veof && aeof)
            {
                break;
            }
        };

		if (pvfile)
		{
			fclose(pvfile);
		}

		if (pafile)
		{
			fclose(pafile);
		}
        av_packet_free(&pkt);
    }
}

void avdemux_trace_sequence_header(struct avdemux_context* pdemux, int flag)
{
	if (pdemux && pdemux->pfmt_ctx)
	{
		if (flag & AVMEDIA_PRINTF_VIDEO_SH)
		{
			uint8_t* pextradata = pdemux->pfmt_ctx->streams[pdemux->nvindex]->codecpar->extradata;
			int extradata_size = pdemux->pfmt_ctx->streams[pdemux->nvindex]->codecpar->extradata_size;
			if (pextradata)
			{
				printf("video extradata, size:%d:\n", extradata_size);
				for (int i = 0; i < extradata_size; i++)
				{
					printf("%02x", *(pextradata + i));
					if (0 == (i + 1) % 8)
					{
						printf("\n");
					}
				}
			}
			else
			{
				AVPacket* pkt = av_packet_alloc();
				while (1)
				{
					int ret = avdemux_read_packet(pdemux, pkt);
					if (ret < 0)
					{
						break;
					}
					if (AVMEDIA_TYPE_VIDEO ==  pdemux->pfmt_ctx->streams[pkt->stream_index] && pkt->flags)
					{

					}
				};
			}
			printf("\n");
		}

		if (flag & AVMEDIA_PRINTF_AUDIO_SH)
		{
			uint8_t* pextradata = pdemux->pfmt_ctx->streams[pdemux->naindex]->codecpar->extradata;
			int extradata_size = pdemux->pfmt_ctx->streams[pdemux->naindex]->codecpar->extradata_size;
			printf("audio extradata, size:%d:\n", extradata_size);
			for (int i = 0; i < extradata_size; i++)
			{
				printf("%02x", *(pextradata + i));
			}
			printf("\n");
		}
	}
	
}*/