#include "avdemux.h"
#include "lbdemux.h"
#include "avenc.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "avscale.h"
#define SHOT_VERSION_MAYJOR     0
#define SHOT_VERSION_MINOR      1
#define SHOT_VERSION_MICRO      1
#define SHOT_VERSION_TINY       0

typedef struct
{
    char*       purl;
    char*       pout;
    int64_t     lstart_pts;
    int64_t     lduration;
    int         dst_width;
    int         dst_height;
    int         printf_flag;
	int			write_flag;
} SHOT_INFO;
#if 0
typedef int(*args_parse_func)(const char* ptag, const char* pval);//, const char* psn, const char* pfn);

typedef struct
{
	char* pshort_name;
	char* pfull_name;
	char* premark;
	args_parse_func pparse_func;
} args_param_t;

int args_parse(const char* ptag, const char* pval)
{
	if (NULL == ptag || NULL == pval)
	{
		lberror("Invalid tag:%s or value:%s\n", ptag, pval);
		return -1;
	}

	if (0 == memcmp(ptag, , strlen(psn)) || 0 == memcmp(ptag, pfn, strlen(pfn)))
	{

	}
}
args_param_t args_list[] =
{
	{"-h", "-help", "show usage!", NULL};
	{"-i", "-input", "input image shot media file", args_parse};
	{"-b", "-input", "input image shot media file", args_parse};
};

void usage()
{

}
#endif
void usage()
{
    printf("usage:\n");
    printf("-i: input image shot media file\n");
    printf("-b: image shot time in seconds\n");
    printf("-v: image shot program version\n");
    printf("-s: output image resolution, such as: -s 640x480\n");
    printf("-o: output image file name\n");
    printf("-p: printf meedia info include vpacket, apacket, allpacekt, main\n");
    printf("-d: duration of printf packets\n");
	printf("-w: write media packets, -v indicate video -a -indicate audio, -all indicate both video and audio\n");
	printf("-sh: show audio or video sequence header!\n");
}

int tolower_cstring(char* pdst, int len, char* psrc)
{
	if (NULL == psrc || len <= strlen(psrc))
	{
		return -1;
	}

	while (*psrc)
	{
		*pdst++ = tolower(*psrc++);
	};
	*pdst = 0;
	return 0;
}

int toupper_cstring(char* pdst, int len, char* psrc)
{
	if (NULL == psrc || len <= strlen(psrc))
	{
		return -1;
	}

	while (*psrc++)
	{
		*pdst++ = toupper(*psrc);
	};
	*pdst = 0;
	return 0;
}

int parser_args(int argc, char** args, SHOT_INFO* psi)
{
    int ret = -1;
	char argstr[256];
    assert(psi);
    memset(psi, 0, sizeof(SHOT_INFO));
    int i;
    for(i = 1; i < argc; i++)
    {
		tolower_cstring(argstr, 256, args[i]);
        if(0 == strcmp(argstr, "-i") || 0 == strcmp(argstr, "-input"))
        {
            i++;
            if(i >= argc)
            {
                ret = -1;
                break;
            }
            int len = strlen(args[i]);
            psi->purl = (char*)malloc(len+1);
            memcpy(psi->purl, args[i], len+1);
            ret = 0;
        }
        else if(0 == strcmp(argstr, "-s") || 0 == strcmp(argstr, "-start"))
        {
            i++;
            if(i >= argc)
            {
                ret = -1;
                break;
            }
            char buf[256] = {0};
			memcpy(buf, args[i], strlen(args[i]) + 1);
            //strcpy(buf, args[i]);
            char* ptag = strchr(args[i], 'x');
            if(NULL != ptag)
            {
                *ptag = (uint8_t)0;

                //strcpy(buf, args[i]);
				memcpy(buf, args[i], strlen(args[i]) + 1);
                psi->dst_width = atoi(buf);
                psi->dst_height = atoi(ptag+1);
            }

        }
        else if(0 == strcmp(argstr, "-o") || 0 == strcmp(argstr, "-output"))
        {
            i++;
            if(i >= argc)
            {
                ret = -1;
                break;
            }
            int len = strlen(args[i]);
            psi->pout = (char*)malloc(len+1);
            memcpy(psi->pout, args[i], len+1);
            //printf("pout:%s\n", psi->pout);
        }
        else if(0 == strcmp(argstr, "-b") || 0 == strcmp(argstr, "-begin"))
        {
            i++;
            if(i >= argc)
            {
                ret = -1;
                break;
            }
            double dpts = atof(args[i]);
            psi->lstart_pts = (int64_t)dpts*1000000;
        }
        else if(0 == strcmp(argstr, "-d") || 0 == strcmp(argstr, "-duration"))
        {
            i++;
            if(i >= argc)
            {
                ret = -1;
                break;
            }
            double dur = atof(args[i]);
            psi->lduration = (int64_t)dur*1000000;
        }
        else if(0 == strcmp(argstr, "-v") || 0 == strcmp(argstr, "-version"))
        {
            printf("Version:%d.%d.%d.%d\nCopyright (c) 2020 lazybear.com.cn\n", SHOT_VERSION_MAYJOR, SHOT_VERSION_MINOR, SHOT_VERSION_MICRO, SHOT_VERSION_TINY);
            return 1;
        }
        else if(0 == strcmp(argstr, "-h") || 0 == strcmp(argstr, "-help"))
        {
            usage();
            return 1;
        }
        else if(0 == strcmp(argstr, "-p") || 0 == strcmp(argstr, "-pts"))
        {
            i++;
            if(i >= argc)
            {
                ret = -1;
                break;
            }
            if(strcmp(args[i], "vpacket") == 0 || 0 == strcmp(args[i], "v"))
            {
                psi->printf_flag |= AVMEDIA_PRINTF_VIDEO_PACKET;
            }
            else if(strcmp(args[i], "apacket") == 0 || 0 == strcmp(args[i], "a"))
            {
                psi->printf_flag |= AVMEDIA_PRINTF_AUDIO_PACKET;
            }
            else if(strcmp(args[i], "allpacket") == 0 || 0 == strcmp(args[i], "all"))
            {
                psi->printf_flag |= AVMEDIA_PRINTF_VIDEO_PACKET;
                psi->printf_flag |= AVMEDIA_PRINTF_AUDIO_PACKET;
            }
            else if(strcmp(args[i], "main") == 0)
            {
                psi->printf_flag |= AVMEDIA_PRINTF_MAIN_INFO;
            }
            //printf("psi->printf_flag:%d\n", psi->printf_flag);
        }
		else if (0 == strcmp(argstr, "-w") || 0 == strcmp(argstr, "-write"))
		{
			i++;
			if (i >= argc)
			{
				ret = -1;
				break;
			}
			if (strcmp(args[i], "vpacket") == 0 || 0 == strcmp(args[i], "v"))
			{
				psi->printf_flag |= AVMEDIA_WRITE_VIDEO_PACKET;
			}
			else if (strcmp(args[i], "apacket") == 0 || 0 == strcmp(args[i], "a"))
			{
				psi->printf_flag |= AVMEDIA_WRITE_AUDIO_PACKET;
			}
			else if (strcmp(args[i], "allpacket") == 0 || 0 == strcmp(args[i], "all"))
			{
				psi->printf_flag |= AVMEDIA_WRITE_VIDEO_PACKET;
				psi->printf_flag |= AVMEDIA_WRITE_AUDIO_PACKET;
			}
		}
		else if (0 == strcmp(argstr, "-sh") || 0 == strcmp(argstr, "-sequenceheader"))
		{
			i++;
			if (strcmp(args[i], "v") == 0)
			{
				psi->printf_flag |= AVMEDIA_PRINTF_VIDEO_SH;
			}
			else if (strcmp(args[i], "a") == 0)
			{
				psi->printf_flag |= AVMEDIA_PRINTF_AUDIO_SH;
			}
			else if (strcmp(args[i], "all") == 0)
			{
				psi->printf_flag |= AVMEDIA_PRINTF_VIDEO_SH;
				psi->printf_flag |= AVMEDIA_PRINTF_AUDIO_SH;
			}
		}
        else
        {
            usage();
			return 0;
        }
        
    }

    /*if(NULL == psi->purl)
    {
        return -1;
    }*/
    return ret;
}

int main(int argc, char** args)
{
    int ret = -1;
    SHOT_INFO si;
    stream_info stream_info;
    struct avdemux_context* pdemux = NULL;
	struct lbdec_ctx* pdc = NULL;
    avscale_ctx* psc = NULL;
    AVPacket* pkt = NULL;
    AVPacket* jpgpkt = NULL;
    int write_len = 0;
    int max_jpg_len = 0;
    int jpg_len = 0;
    char* pjpgbuf = NULL;
    int got_keyframe = 0;
    AVFrame* pframe = NULL;
    memset(&si, 0, sizeof(SHOT_INFO));
    do
    {
         ret = parser_args(argc, args, &si);
        if(0 != ret)
        {
            if(ret < 0)
            {
                lberror("Invalid input argument\n");
                usage();
            }
            break;
        }
        
        if(NULL == si.purl)
        {
            break;
        }

		pdc = lbdemux_open_context(NULL, si.purl);
        //lbtrace("pdemux:%p = avdemux_open_context(NULL, si.purl:%s), si.pout:%s\n", pdemux, si.purl, si.pout);
        if(NULL == pdc)
        {
            lberror("pdc:%p = lbdemux_open_context(NULL, %s) failed\n", pdc, si.purl);
            ret = -1;
            break;
        }
        if(si.printf_flag > 0)
        {
            lbtrace("printf_flag:%d, lstart_pts:%" PRId64 ", lduration:%" PRId64 "\n", si.printf_flag, si.lstart_pts, si.lduration);
            lbdemux_trace_info(pdc, si.printf_flag, si.lstart_pts, si.lduration);
        }

		lbdemux_trace_sequence_header(pdc, si.printf_flag);

        if(NULL == si.pout)
        {
            break;
        }
        memset(&stream_info, 0, sizeof(stream_info));
        lbdemux_get_stream_info(pdc, AVMEDIA_TYPE_VIDEO, &stream_info);
        if(stream_info.nindex < 0)
        {
            lberror("No available video stream found, stream_info.nindex:%d\n", stream_info.nindex);
            ret = -1;
            break;
        }
        
        ret = lbdemux_seek_packet(pdc, si.lstart_pts);
        printf("index:%d, stream_info.nwidth:%d, stream_info.nheight:%d\n", stream_info.nindex, stream_info.nwidth, stream_info.nheight);

        if(si.dst_width > 0 && si.dst_height > 0 || AV_PIX_FMT_YUVJ420P != stream_info.nformat)
        {
            if(si.dst_width <= 0 || si.dst_height <= 0)
            {
                si.dst_width = stream_info.nwidth;
                si.dst_height = stream_info.nheight;
            }
            psc = avscale_open_context(si.dst_width, si.dst_height, AV_PIX_FMT_YUVJ420P);
        }
        
        //lbtrace("si.pout:%s\n", si.pout);
        do{
            if(NULL == pkt)
            {
                pkt = av_packet_alloc();
            }
            else
            {
                av_packet_unref(pkt);
            }
            
            if(NULL == pframe)
            {
                pframe = av_frame_alloc();
            }
            else
            {
                av_frame_unref(pframe);
            }
			ret = lbdemux_read_frame(pdc, pframe);
            //ret = lbdemux_read_packet(pdemux, pkt);

            if(ret < 0)
            {
                lberror("ret:%d = avdemux_read_packet(pdemux:%p, &pframe:%p) failed\n", ret, pdemux, pkt);
                break;
            }
			if (pframe->pts >= si.lstart_pts && pframe->width > 0 && pframe->height > 0)
			{
				/*if(si.dst_width > 0 && si.dst_height > 0 || )
				{
				psc = avscale_open_context(si.dst_width, si.dst_height, AV_PIX_FMT_YUVJ420P);
				}*/
				if (psc)
				{
					ret = avscale_frame(psc, pframe);
					if (ret < 0)
					{
						//av_buffer_make_writable;
					}
				}
#ifdef ENCODER_PACKET_FROM_FRAME
				jpgpkt = av_packet_alloc();
				jpg_len = encoder_packet_from_frame(AV_CODEC_ID_MJPEG, pframe, jpgpkt);//encoder_frame(AV_CODEC_ID_MJPEG, pframe, pjpgbuf, max_jpg_len);
#else
				max_jpg_len = stream_info.nwidth * stream_info.nheight;
				max_jpg_len = max_jpg_len < 1024 * 1024 ? 1024 * 1024 : max_jpg_len;
				pjpgbuf = (char*)malloc(max_jpg_len);
				jpg_len = encoder_frame(AV_CODEC_ID_MJPEG, pframe, pjpgbuf, max_jpg_len);
#endif
				lbtrace("jpglen:%d = encoder_frame, pframe->pts:%" PRId64 ", si.lstart_pts:%" PRId64 "\n", jpg_len, pframe->pts, si.lstart_pts);
				if (jpg_len > 0)
				{
					break;
				}
				else
				{
					lberror("jpglen:%d = encoder_frame(AV_CODEC_ID_MJPEG, pframe:%p, pjpgbuf:%p, jpg_len) failed\n", jpg_len, pframe, pjpgbuf);
				}
			}
			/*
            if(pkt->stream_index == stream_info.nindex)
            {
                if(!got_keyframe && !pkt->flags)
                {
                    continue;
                }
                if(NULL == pframe)
                {
                    pframe = av_frame_alloc();
                }
                else
                {
                    av_frame_unref(pframe);
                }
                
                got_keyframe = 1;
                ret =lbdemux_decode_packet(pdemux, pkt, pframe);
                if(ret < 0)
                {
                    lberror("ret:%d = avdemux_decode_packet(pdemux:%p, pkt:%p, pframe:%p)\n", ret, pdemux, pkt, pframe);
                    continue;
                }
                if(pframe->pts >= si.lstart_pts && pframe->width > 0 && pframe->height > 0)
                {
                    if(psc)
                    {
                        ret = avscale_frame(psc, pframe);
                        if(ret < 0)
                        {
                            //av_buffer_make_writable;
                        }
                    }
#ifdef ENCODER_PACKET_FROM_FRAME
                    jpgpkt = av_packet_alloc();
                    jpg_len = encoder_packet_from_frame(AV_CODEC_ID_MJPEG, pframe, jpgpkt);//encoder_frame(AV_CODEC_ID_MJPEG, pframe, pjpgbuf, max_jpg_len);
#else
                    max_jpg_len = stream_info.nwidth * stream_info.nheight;
                    max_jpg_len = max_jpg_len < 1024*1024 ? 1024*1024 : max_jpg_len;
                    pjpgbuf = (char*)malloc(max_jpg_len);
                    jpg_len = encoder_frame(AV_CODEC_ID_MJPEG, pframe, pjpgbuf, max_jpg_len);
#endif
                    lbtrace("jpglen:%d = encoder_frame, pframe->pts:%" PRId64 ", si.lstart_pts:%" PRId64 "\n", jpg_len, pframe->pts, si.lstart_pts);
                    if(jpg_len > 0)
                    {
                        break;
                    }
                    else
                    {
                        lberror("jpglen:%d = encoder_frame(AV_CODEC_ID_MJPEG, pframe:%p, pjpgbuf:%p, jpg_len) failed\n", jpg_len, pframe, pjpgbuf);
                    }
                }
            }*/
            //printf("ret:%d = avdemux_read_frame, width:%d, hegiht:%d\n", ret, pframe->width, pframe->height);
            
        }while(1);
    } while (0);
    
   if(pframe)
    {
        av_frame_free(&pframe);
    }

    if(pkt)
    {
        av_packet_free(&pkt);
    }
#ifdef ENCODER_PACKET_FROM_FRAME
    if(jpgpkt && jpgpkt->size > 0)
    {
		FILE* pfile = NULL;
#ifdef WIN32
		fopen_s(&pfile,si.pout, "wb");
#else
		pfile =	fopen(si.pout, "wb");
#endif
        if(pfile)
        {
            write_len = fwrite(jpgpkt->data, 1, jpgpkt->size, pfile);
            fclose(pfile);
        }
        av_packet_free(&jpgpkt);
    }
#else
    if(pjpgbuf && max_jpg_len > 0)
    {
        FILE* pfile = fopen(si.pout, "wb");
        if(pfile)
        {
            write_len = fwrite(pjpgbuf, 1, max_jpg_len, pfile);
            fclose(pfile);
        }
    }
    
    if(pjpgbuf)
    {
        free(pjpgbuf);
    }
#endif
    

    if(psc)
    {
        avscale_close_context(&psc);
    }

    if(pdemux)
    {
        avdemux_close_contextp(&pdemux);
    }

    if(si.purl)
    {
        free(si.purl);
        si.purl = NULL;
    }

    if(si.pout)
    {
        if(jpg_len <= 0)
        {
            lbtrace("capture image failed, ret:%d, jpg_len:%d, write_len:%d\n", ret, jpg_len, write_len);
        }
        else
        {
            lbtrace("capture image success, write jpglen:%d\n", write_len);
        }
    }

    if(si.pout)
    {
        free(si.pout);
        si.pout = NULL;
    }
#ifdef WIN32
	system("pause");
#endif
    return write_len > 0 ? 0 : -1;
}
