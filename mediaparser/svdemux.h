#ifndef _REC_DEMUX_H_
#define _REC_DEMUX_H_
#include <stdio.h>
#include <stdint.h>
#include "lbdemux.h"
#define ENABLE_READ_WITH_AVPACKET
#ifdef ENABLE_READ_WITH_AVPACKET
#include <libavformat/avformat.h>
#endif
#define __STDC_FORMAT_MACROS
#define lbtrace(...)
#define lberror printf
#define lbinfo(...)
enum codec_id
{
    codec_id_none = -1,
    codec_id_h264 = 0,
    codec_id_h265 = 1,
    codec_id_mp3  = 2,
    codec_id_aac  = 8
};
#define FRAME_TAG_SYNC_NUM      0xEB0000AA
typedef struct hs003_packet_header{
  unsigned int tag;        		// sync 0xEB0000AA
  unsigned int size;        	// frame size
  unsigned int type;        	// frame type: 0, P frame, 1: I frame, 8: audio frame
  unsigned int fps;        		// frame rate
  unsigned int time_sec;      	// timestamp in second
  unsigned int time_usec;      // timestamp in microsecond
} VAVA_HS003_PACKET_HDR;

//video record head
typedef struct hs003_record_header{
  char tag;            		// 0:create record, 1.complete record
  char v_encode;          	// video codec, h264 0
  char a_encode;          	// audio codec, aac 3
  char res;            		// video resolution
  char fps;            		//֡ video frame rate
  char encrypt;          	// encrypt mode: 0, no encrypt, 1.aes encrypt
  unsigned short vframe;     // video frame count
  int size;            		// video record size
  int time;            		// video record duration
} VAVA_HS003_REC_HDR;


//帧头
typedef struct hs004_packet_header{
  unsigned int tag;        //同步头 0xEB0000AA
  unsigned int size;        //帧大小
  unsigned int type;        //帧类型 0 P帧 1 I帧 8音频帧
  unsigned int fps;        //实时帧率
  unsigned int framnum;      //帧编号
  unsigned long long ntsamp;    //时间戳(毫秒)
  unsigned int reserv;      //保留
}VAVA_HS004_PACKET_HDR;

//录像文件信息
typedef struct hs004_record_header{
  char tag;            //0 录像创建  1录像完成
  char ver;            //版本号 初始为0
  char v_encode;          //视频编码格式 0 H264 1 H265
  char a_encode;          //音频编码格式
  char res;            //分辨率
  char fps;            //帧率
  char encrypt;          //加密方式 0 不加密 0x1A AES加密
  char alarmtype;          //触发类型
  char reserv_1[4];        //保留
  unsigned short vframe;      //视频帧数
  unsigned short sample;          //音频采样率 
  int size;            //录像大小
  int time;            //录像时长
  int reserv_2[2];        //保留
}VAVA_HS004_REC_HDR;

typedef enum
{
    ecamera_hs_unknown = -1,
    ecamera_hs_001 = 1,
    ecamera_hs_002 = 2,
    ecamera_hs_003 = 3,
    ecamera_hs_004 = 4,
    ecamera_hs_005 = 5,
} ecamera_record_type;

typedef struct ipc_record_header
{
    char tag;
    char version;
    char vcodec_id;
    char acodec_id;
    char res;
    char fps;
    char encrypt;
    char alarm_type;
    int vframe_num;
    int sample_rate;
    int nrec_size;
    int nduration;
} IPC_REC_HDR;

typedef struct ipc_packet_header
{
    int mt;
    int codec_id;
    int keyflag;
    int64_t pts;
    int size;
    int frame_num;
} IPC_PACKET_HDR;

typedef int (*open_record)(void* powner);

typedef int (*seek_record)(void* powner, int64_t pts);

typedef int(*read_packet)(void* powner, IPC_PACKET_HDR* piph, char* pdata, int len);
/*typedef int (*open_record)(VAVA_HS_REC_CTX* prec_ctx);

typedef int (*seek_record)(VAVA_HS_REC_CTX* prec_ctx, int64_t pts);

typedef int(*read_packet)(VAVA_HS_REC_CTX* prec_ctx, IPC_PACKET_HDR* piph, char* pdata, int len);
*/
typedef struct record_context
{
    FILE*           pfile;
    ecamera_record_type rec_type;
    IPC_REC_HDR     rec_hdr;
    IPC_PACKET_HDR  pkt_hdr;
    int             nvframe_cnt;
    int             naframe_cnt;
    char*           pvsh;
    int             nvsh;
    char*           padts_hdr;
    int             nadts_hdr_len;

    open_record     popen_func;
    seek_record     pseek_func;
    read_packet     pread_func;

	char*			ppkt_data;
	int				npkt_data_len;
} VAVA_HS_REC_CTX;

lbdec_ctx* svdemux_open(const char* purl);
VAVA_HS_REC_CTX* vava_hs_open_record(const char* purl);

int vava_hs_seek_record(VAVA_HS_REC_CTX* prec_ctx, int64_t pts);

int svdemux_read_packet(VAVA_HS_REC_CTX* prec_ctx, AVPacket* pkt);

int vava_hs_read_packet(VAVA_HS_REC_CTX* prec_ctx, IPC_PACKET_HDR* piph, char* pdata, int len);


int vava_hs_get_stream_info(VAVA_HS_REC_CTX* prec_ctx, int mt, stream_info* psi);
int vava_hs_parser_sequence_hdr(VAVA_HS_REC_CTX* prec_ctx);

void vava_hs_close_record(VAVA_HS_REC_CTX** pprec_ctx);

#endif