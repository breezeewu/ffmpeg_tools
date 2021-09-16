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
#include "lbdemux.h"
#include "avdemux.h"
#include "svdemux.h"
#include "audio_suquence_header.h"
//#include "avenc.h"
#define _CRT_SECURE_NO_WARNINGS
#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif

lbdec_ctx* lbdemux_open_context(char* purl)
{
	lbdec_ctx* pdc = svdemux_open(purl);
	if (pdc)
	{
		return pdc;
	}

	pdc = avdemux_open(purl);
	return pdc;
}

void lbdemux_close_contextp(lbdec_ctx** ppdc)
{
	if (ppdc && *ppdc)
	{
		lbdec_ctx* pdc = *ppdc;
		pdc->pclose_context(pdc->powner);
		free(pdc);
		*ppdc = pdc = NULL;
	}
}

int lbdemux_read_packet(lbdec_ctx* pdc, AVPacket* pkt)
{
	if (pdc && pdc->pread_packet)
	{
		return pdc->pread_packet(pdc, pkt);
	}

	return -1;
}

int lbdemux_read_frame(lbdec_ctx* pdc, AVFrame* pframe)
{
	if (pdc && pdc->pread_frame)
	{
		return pdc->pread_frame(pdc, pframe);
	}

	return -1;
}

int lbdemux_seek_packet(lbdec_ctx* pdc, int64_t pts)
{
	if (pdc && pdc->pseek_packet)
	{
		return pdc->pseek_packet(pdc, pts);
	}

	return -1;
}

int lbdemux_get_stream_info(lbdec_ctx* pdc, int mt, stream_info* psi)
{
	if (pdc && pdc->pget_stream_info)
	{
		return pdc->pget_stream_info(pdc->powner, mt, psi);
	}

	return -1;
}



int64_t avdemux_get_duration(lbdec_ctx* pdc, int mediatype)
{
	stream_info si;
	if (lbdemux_get_stream_info(pdc, mediatype, &si) == 0)
	{
		return si.lduration;
	}
	return 0;
}

int avdemux_trace_main_info(lbdec_ctx* pdc)
{
    int i;
    char* st_name_list[] = {"unknown", "video", "audio", "data", "subtitle", "attachment"};
    if(NULL == pdc)
    {
        lberror("Invalid parameter, pdc:%p\n", pdc);
        return -1;
    }

	for (i = 0; i < 2; i++)
	{
		stream_info si;
		lbdemux_get_stream_info(pdc, i, &si);
        lbtrace("stream %d: %s\n", si.nindex, st_name_list[si.ncodec_type+1]);
        lbtrace("codec name: %s\n", si.codec_name);
        lbtrace("bitrate:%d\n", si.nbitrate);
        lbtrace("duration:%lf\n", (double)si.lduration/1000000);
        lbtrace("nb_frames:%d\n", si.nb_frames);
        if(AVMEDIA_TYPE_VIDEO == si.ncodec_type)
        {
            lbtrace("width:%d\n", si.nwidth);
            lbtrace("height:%d\n", si.nheight);
            lbtrace("pixel format:%d\n", si.nformat);
            lbtrace("video frame rate: %d:%d\n", si.frame_rate.num, si.frame_rate.den);
            lbtrace("video aspect rate: %d:%d\n", si.aspect_rate.num, si.aspect_rate.den);
        }
        else if(AVMEDIA_TYPE_AUDIO == si.ncodec_type)
        {
            lbtrace("channel:%d\n", si.nchannel);
            lbtrace("sample rate:%d\n", si.nsample_rate);
            lbtrace("sample format:%d\n", si.nformat);
        }
        printf("\n");
    }

	return 0;
}

void avdemux_trace_info(lbdec_ctx* pdc, int flag, int64_t start_time, int64_t dur_time)
{
    if(NULL == pdc)
    {
        lberror("Invalid parameter, pdc:%p\n", pdc);
        return -1;
    }

    if(flag & AVMEDIA_PRINTF_MAIN_INFO)
    {
        avdemux_trace_main_info(pdc);
    }
    int vidx = -1;
    int aidx = -1;
    int veof = 1;
    int aeof = 1;
    int vnum = 0, anum = 0;
	FILE* pvfile = NULL;
	FILE* pafile = NULL;
    int64_t end_time = 0;
	int vindex = -1;
	int aindex = -1;
	stream_info vsi, asi;
	lbdemux_get_stream_info(pdc, 0, &vsi);
	lbdemux_get_stream_info(pdc, 1, &asi);
    if(dur_time > 0)
    {
        end_time = start_time + dur_time;
    }
    if(flag & AVMEDIA_PRINTF_VIDEO_PACKET  && vsi.nindex >= 0)
    {
        vidx = vsi.nindex;
        veof = 0;
    }
    if((flag & AVMEDIA_PRINTF_AUDIO_PACKET) && asi.nindex >= 0)
    {
        aidx = asi.nindex;
        aeof = 0;
    }
	if (flag & AVMEDIA_WRITE_VIDEO_PACKET  && vsi.nindex >= 0)
	{
		vidx = vsi.nindex;
		veof = 0;
		pvfile = fopen("video.data", "wb");
	}
	if ((flag & AVMEDIA_WRITE_AUDIO_PACKET) && asi.nindex >= 0)
	{
		aidx = asi.nindex;
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
			lbdemux_seek_packet(pdc, start_time);
        }
        while(1)
        {
            int ret = lbdemux_read_packet(pdc, pkt);
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
            /*else if(end_time > 0 && end_time < pkt->pts)
            {
                break;
            }*/
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

void avdemux_trace_sequence_header(lbdec_ctx* pdc, int flag)
{
	if (pdc)
	{
		stream_info vsi, asi;
		lbdemux_get_stream_info(pdc, 0, &vsi);
		lbdemux_get_stream_info(pdc, 1, &asi);
		if (flag & AVMEDIA_PRINTF_VIDEO_SH && vsi.nindex >= 0)
		{
			printf("video extradata, size:%d:\n", vsi.nseq_hdr_len);
			for (int i = 0; i < vsi.nseq_hdr_len; i++)
			{
				printf("%02x", (unsigned char)vsi.sequence_header[i]);
				if (0 == (i + 1) % 8)
				{
					printf("\n");
				}
			}
		}

		if (flag & AVMEDIA_PRINTF_AUDIO_SH && asi.nindex >= 0)
		{
			printf("audio extradata, size:%d:\n", asi.nseq_hdr_len);
			for (int i = 0; i < asi.nseq_hdr_len; i++)
			{
				printf("%02x", (unsigned char)asi.sequence_header[i]);
			}
			printf("\n");
		}
	}
}