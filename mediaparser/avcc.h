#pragma once
#include "bitstream.h"
#define AVC_PROFILE_BASELINE		66
#define AVC_PROFILE_MAIN			77
#define AVC_PROFILE_EXTENDED		88
#define AVC_PROFILE_HIGHT_FREXT		100
#define AVC_PROFILE_HIGHT10_FREXT   110
#define AVC_PROFILE_HIGHT_422_FREXT 122
#define AVC_PROFILE_HIGHT_444_FREXT 144
/*#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif*/
#define ue_size uint8_t
#define se_size uint8_t
typedef struct sps_contxt
{
	// 66 Baseline, 77 Main, 88 Extended, 100 High(FRExt), 110 High10(FRExt) 122 High4：2：2(FRExt) 144 High4:4:4(FRExt)
	uint8_t profile_idc;// 8 bits
	// frame per second
	uint8_t constraint_set0_flag : 1;
	uint8_t constraint_set1_flag : 1;
	uint8_t constraint_set2_flag : 1;
	uint8_t constraint_set3_flag : 1;
	uint8_t constraint_set4_flag : 1;
	uint8_t constraint_set5_flag : 1;
	uint8_t reserved_zero_2bits : 2;
	uint8_t level_idc;// 8bits
	ue_size seq_parameter_set_id;// ue
	ue_size chroma_format_idc;// ue
	uint8_t separate_colour_plane_flag; // 1bit, 3 == chroma_format_idc时存在
	ue_size bit_depth_luma_minus8; // ue
	ue_size bit_depth_chroma_minus8; // ue
	uint8_t qpprime_y_zero_transform_bypass_flag;// 1bit
	uint8_t seq_scaling_matrix_present_flag; //1bit
	uint8_t* seq_scaling_list_present_flag;// 1bit, 
	ue_size log2_max_frame_num_minus4; // ue
	ue_size pic_order_cnt_type; // ue
	ue_size log2_max_pic_order_cnt_lsb_minus4; // ue
	ue_size delta_pic_order_always_zero_flag; // 1bit, if 1 == pic_order_cnt_type
	se_size offset_for_non_ref_pic; // se
	se_size offset_for_top_to_bottom_field; // se
	ue_size num_ref_frames_in_pic_order_cnt_cycle; // ue
	se_size* offset_for_ref_frame; // se, list
	ue_size max_num_ref_frames; // ue 最大参考帧间隔
	uint8_t gaps_in_frame_num_value_allowed_flag; // 1 bit
	int		time_scale;
	int		num_units_in_tick;
	int fps;
	int width;
	int height;
	int encwidth;
	int encheight;
	int frame_crop_left_offset;
	int frame_crop_right_offset;
	int frame_crop_top_offset;
	int frame_crop_bottom_offset;
	char* psps;
	int sps_len;
} sps_ctx;

typedef struct pps_context
{
	ue_size pic_parameter_set_id;
	ue_size seq_parameter_set_id;
	uint8_t entropy_coding_mode_flag; // 1bit
	uint8_t pic_order_present_flag;   // 1bit
	ue_size num_slice_groups_minus1;
	ue_size slice_group_map_type;   // if(num_slice_groups_minus1 > 0)

	ue_size* run_length_minus1;		// slice_group_map_type == 0
	ue_size* top_left;				// slice_group_map_type == 2
	ue_size* bottom_right;			// slice_group_map_type == 2
	uint8_t slice_group_change_direction_flag; // 1bit lice_group_map_type == 3,4,5
	ue_size slice_group_change_rate_minus1; // lice_group_map_type == 3,4,5
	uint8_t pic_size_in_map_units_minus1; // lice_group_map_type == 6
	ue_size* slice_group_id;			  // lice_group_map_type == 6
	ue_size num_ref_idx_10_active_minus1;
	ue_size num_ref_idx_11_active_minue1;
	uint8_t weighted_pred_flag;			// 1bit
	uint8_t weighted_bipred_ide;		// 2bit
	se_size pic_init_qp_minus26;
	se_size pic_init_qs_index_offset;
	se_size chroma_qp_index_offset;
	uint8_t deblocking_filter_control_present_flag; // 1bit
	uint8_t constrained_intra_pred_flag;			// 1bit
	uint8_t redundant_pic_cnt_present_flag;			// 1bit

	// more rbsp data
	uint8_t transform_8x8_mode_flag; // 1bit
	uint8_t pic_scaling_matrix_present_flag; // 1bit

	// pic_scaling_matrix_present_flag != 0
	uint8_t* pic_scaling_list_present_flag; // 1bit
	se_size second_chroma_qp_index_offset;

	
} pps_ctx;
typedef struct avcc_context
{
	unsigned char version;		// h264 version, usually 0x01
	unsigned char profile;		// h264 profile
	unsigned char compatibility;
	unsigned char level;		// h264 level
	unsigned short reserved : 6;// all bits on, 即该字节取值为0xFC|NALULengthSizeMinusOne
	unsigned short nalu_length_size_minus_one : 2; // NAL长度字节数-1,取值通常为0, 1, 3, 即spssize, ppssize对应的字节数为1，2，4字节
	unsigned short reserved2 : 3; // all bits on,即该字节取值为0xE0|numofspsnalu
	unsigned short num_of_sps_nalu : 5; // number of sps NALUS(usually 1)
	int spssize;	// sps 长度
	sps_ctx**	ppsps;
	unsigned char num_of_pps_nalu;
	int ppssize;	// pps 长度
	pps_ctx* ppps;
	char* pps;
} avcc_ctx;

/*static int find_start_code(char* pbuf, int size, int* pos, int* plen, int* pstartcode, int nalu)
{
	if (!pbuf)
	{
		return -1;
	}
	char* psrc = pbuf;
	int num = 0;
	for (char* psrc = pbuf; psrc - pbuf <= size - 4; psrc++)
	{
		if (0 == *psrc)
			num++;
		else
			num = 0;

		if (num >= 2 && 1 == psrc[1])
		{
			int nalval = psrc[2] & 0x1f;
			if (nalu && nalu != nalval)
			{
				num = 0;
				continue;
			}
			//nalu = psrc[1] & 0x1f;
			//if (pnalu) *pnalu = psrc[1] & 0x1f;
			if(pos) *pos = psrc - pbuf - num + 1 + *pos;
			if(pstartcode){*pstartcode = num + 1;}
			if (plen)
			{
				int nxtpos = *pos;
				int nalutype = find_start_code(psrc, size - *pos, &nxtpos, NULL, NULL, 0);
				if (nalutype < 0)
				{
					//*plen = size - *pos + num - 1;
					nxtpos = size - num + 1;
				}
				*plen = nxtpos - *pos + num - 1;
			}
			return  nalval;
		}
	}

	return -1;
}*/

static int find_start_code(char* pdata, int size, int* pstart_code_size, int* pnal_type)
{
    int offset = 0;
    int start_code_size = 0;
    int nal_type = 0;
    char* ptmp = pdata;
    char* pend = pdata + size;
    while (ptmp < pend - 3)
    {
        if (0 == *ptmp && 0 == *(ptmp + 1))
        {
            if (0 == *(ptmp + 2) && 1 == *(ptmp + 3))
            {
                start_code_size = 4;
                nal_type = *(ptmp + 4) & 0x1f;
                break;
            }
            else if (1 == *(ptmp + 2))
            {
                start_code_size = 3;
                nal_type = *(ptmp + 3) & 0x1f;
                break;
            }
        }
        ptmp++;
    }
    if (pstart_code_size)
    {
        *pstart_code_size = start_code_size;
    }

    if (pnal_type)
    {
        *pnal_type = nal_type;
    }
    if (start_code_size > 0)
    {
        return ptmp - pdata;
    }

    return -1;
}

static sps_ctx* demux_sps(char* pspsbuf, int size)
{
	if (pspsbuf && size > 0)
	{
		sps_ctx* psps = NULL;
		if (7 != (*pspsbuf & 0x1f))
		{
			while (*pspsbuf == 0)
			{
				pspsbuf++;
				size--;
			}
		
			assert(1 == *pspsbuf);
			pspsbuf++;
            size--;
		}
        if(7 != (*pspsbuf & 0x1f))
        {
            return NULL;
        }
        
        psps = (sps_ctx*)malloc(sizeof(sps_ctx));
        memset(psps, 0, sizeof(sps_ctx));
        psps->psps = (char*)malloc(size);
        memcpy(psps->psps, pspsbuf, size);
        psps->sps_len = size;
		pspsbuf++;
		bitstream_ctx* pbsc = bitstream_open(pspsbuf, size);
		if (!pbsc)
		{
			return psps;
		}
		
		psps->profile_idc = read_bits(pbsc, 8);//read_byte(pbsc, 1);
		psps->constraint_set0_flag = read_bits(pbsc, 1);
		psps->constraint_set1_flag = read_bits(pbsc, 1);
		psps->constraint_set2_flag = read_bits(pbsc, 1);
		psps->constraint_set3_flag = read_bits(pbsc, 1);
		psps->constraint_set4_flag = read_bits(pbsc, 1);
		psps->constraint_set5_flag = read_bits(pbsc, 1);
		psps->reserved_zero_2bits = read_bits(pbsc, 2);
		assert(0 == psps->reserved_zero_2bits);// equal to 0
		psps->level_idc = read_bits(pbsc, 8);
		psps->seq_parameter_set_id = read_ue(pbsc);//read_bits(pbsc, 8);
		if (AVC_PROFILE_HIGHT_FREXT == psps->profile_idc || AVC_PROFILE_HIGHT10_FREXT == psps->profile_idc ||
			AVC_PROFILE_HIGHT_422_FREXT == psps->profile_idc || AVC_PROFILE_HIGHT_444_FREXT == psps->profile_idc)
		{
			//int residual_colour_transform_flag = 0;
			psps->chroma_format_idc = read_ue(pbsc);
			if (3 == psps->chroma_format_idc)
			{
				psps->separate_colour_plane_flag = read_bits(pbsc, 1);//residual_colour_transform_flag = read_bits(pbsc, 1);
			}
			psps->bit_depth_luma_minus8 = read_ue(pbsc);//Ue(buf, nLen, StartBit);
			psps->bit_depth_chroma_minus8 = read_ue(pbsc);//Ue(buf, nLen, StartBit);
			psps->qpprime_y_zero_transform_bypass_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
			psps->seq_scaling_matrix_present_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);

			//int seq_scaling_list_present_flag[8];
			if (psps->seq_scaling_matrix_present_flag)
			{
				int len = psps->chroma_format_idc != 3 ? 8 : 12;
				psps->seq_scaling_list_present_flag = malloc(len*sizeof(uint8_t));
				for (int i = 0; i < len; i++)
				{
					psps->seq_scaling_list_present_flag[i] = read_bits(pbsc, 1);//U(1, buf, StartBit);
				}
			}

		}

		psps->log2_max_frame_num_minus4 = read_ue(pbsc);//Ue(buf, nLen, StartBit);
		psps->pic_order_cnt_type = read_ue(pbsc);//Ue(buf, nLen, StartBit);
		if (psps->pic_order_cnt_type == 0)
			psps->log2_max_pic_order_cnt_lsb_minus4 = read_ue(pbsc);//Ue(buf, nLen, StartBit);
		else if (psps->pic_order_cnt_type == 1)
		{
			psps->delta_pic_order_always_zero_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
			psps->offset_for_non_ref_pic = read_se(pbsc);//Se(buf, nLen, StartBit);
			psps->offset_for_top_to_bottom_field = read_se(pbsc);
			psps->num_ref_frames_in_pic_order_cnt_cycle = read_ue(pbsc);//Ue(buf, nLen, StartBit);

            psps->offset_for_ref_frame = (se_size*)malloc(psps->num_ref_frames_in_pic_order_cnt_cycle*sizeof(se_size));//new uint8_t[psps->num_ref_frames_in_pic_order_cnt_cycle];
			for (int i = 0; i < psps->num_ref_frames_in_pic_order_cnt_cycle; i++)
				psps->offset_for_ref_frame[i] = read_se(pbsc);
			//delete[] offset_for_ref_frame;
		}
		int num_ref_frames = read_ue(pbsc);//Ue(buf, nLen, StartBit);
		int gaps_in_frame_num_value_allowed_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
		int pic_width_in_mbs_minus1 = read_ue(pbsc);//Ue(buf, nLen, StartBit);
		int pic_height_in_map_units_minus1 = read_ue(pbsc);//Ue(buf, nLen, StartBit);

		psps->encwidth = (pic_width_in_mbs_minus1 + 1) * 16;
		psps->encheight = (pic_height_in_map_units_minus1 + 1) * 16;

        int mb_adaptive_frame_field_flag = 0;
		int frame_mbs_only_flag = read_bits(pbsc, 1);// U(1, buf, StartBit);
		if (!frame_mbs_only_flag)
			mb_adaptive_frame_field_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);

		int direct_8x8_inference_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
		int frame_cropping_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
		if (frame_cropping_flag)
		{
			psps->frame_crop_left_offset = read_ue(pbsc);//Ue(buf, nLen, StartBit);
			psps->frame_crop_right_offset = read_ue(pbsc);//Ue(buf, nLen, StartBit);
			psps->frame_crop_top_offset = read_ue(pbsc);//Ue(buf, nLen, StartBit);
			psps->frame_crop_bottom_offset = read_ue(pbsc);//Ue(buf, nLen, StartBit);

			//todo: deal with frame_mbs_only_flag
			psps->width = psps->encwidth - 2 * psps->frame_crop_left_offset
				- 2 * psps->frame_crop_right_offset;
			psps->height = psps->encheight - 2 * psps->frame_crop_top_offset
				- 2 * psps->frame_crop_bottom_offset;

		}
		else {
			psps->width = psps->encwidth;
			psps->height = psps->encheight;
		}

		int vui_parameter_present_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
		if (vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
			if (aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc = read_bits(pbsc, 8);//U(8, buf, StartBit);
				if (aspect_ratio_idc == 255)
				{
					int sar_width = read_bits(pbsc, 16);//U(16, buf, StartBit);
					int sar_height = read_bits(pbsc, 16);//U(16, buf, StartBit);
				}
			}
			int overscan_info_present_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
            int overscan_appropriate_flagu = 0;
			if (overscan_info_present_flag)
				overscan_appropriate_flagu = read_bits(pbsc, 1);//U(1, buf, StartBit);
			int video_signal_type_present_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
			if (video_signal_type_present_flag)
			{
				int video_format = read_bits(pbsc, 3);//U(3, buf, StartBit);
				int video_full_range_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
				int colour_description_present_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
				if (colour_description_present_flag)
				{
					int colour_primaries = read_bits(pbsc, 8);//U(8, buf, StartBit);
					int transfer_characteristics = read_bits(pbsc, 8);//U(8, buf, StartBit);
					int matrix_coefficients = read_bits(pbsc, 8);//U(8, buf, StartBit);
				}
			}
			int chroma_loc_info_present_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
			if (chroma_loc_info_present_flag)
			{
				int chroma_sample_loc_type_top_field = read_ue(pbsc);//Ue(buf, nLen, StartBit);
				int chroma_sample_loc_type_bottom_field = read_ue(pbsc);//Ue(buf, nLen, StartBit);
			}
			int timing_info_present_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);

			if (timing_info_present_flag)
			{
				psps->num_units_in_tick = read_bits(pbsc, 32);//U(32, buf, StartBit);
				psps->time_scale = read_bits(pbsc, 32);//U(32, buf, StartBit);
				//psps->fps = time_scale / num_units_in_tick;
				int fixed_frame_rate_flag = read_bits(pbsc, 1);//U(1, buf, StartBit);
				if (fixed_frame_rate_flag)
				{
					psps->fps = psps->fps / 2;
				}
			}

		}
        
		if (pbsc)
		{
			bitstream_close(&pbsc);
		}

		return psps;
	}
	return NULL;
}
static void sps_context_close(sps_ctx** ppsps_ctx)
{
	if (ppsps_ctx && *ppsps_ctx)
	{
        sps_ctx* psps_ctx = *ppsps_ctx;
        int len = psps_ctx->chroma_format_idc != 3 ? 8 : 12;
        if(psps_ctx->seq_scaling_list_present_flag)
        {
            free(psps_ctx->seq_scaling_list_present_flag);
        }
        
        if(psps_ctx->offset_for_ref_frame)
        {
            free(psps_ctx->offset_for_ref_frame);
            psps_ctx->offset_for_ref_frame = NULL;
        }
		
		free(psps_ctx);
		*ppsps_ctx = psps_ctx = NULL;
	}
}

static int is_h264_idr(char* pdata, int size)
{
	int nalu_type = 0;
	int start_code_size = 0;
	int offset = 0;
	while (offset < size)
	{
		int pos = find_start_code(pdata + offset, size - offset, &start_code_size, &nalu_type);
		if (pos < 0)
		{
			return 0;
		}
		else
		{
			if (5 == nalu_type)
			{
				return 1;
			}
			else if (1 == nalu_type)
			{
				return 0;
			}
			offset += pos + start_code_size;
		}
	}
	return 0;
}

static int h264_get_nalu(char* pdata, int size, int nalu_type, char** ppout, int* pout_len, int* pstart_code_size)
{
	int cur_nalu_type = 0;
	int start_code_size = 0;
	int offset = 0;
	int begin_pos = -1;
	int end_pos = -1;
	while (offset < size)
	{
		int pos = find_start_code(pdata + offset, size - offset, &start_code_size, &cur_nalu_type);
		if (pos < 0)
		{
			if (begin_pos >= 0)
			{
				end_pos = size;
			}
			break;
		}
		else
		{
			if (nalu_type == cur_nalu_type)
			{
				begin_pos = pos + offset;
                if(pstart_code_size)
                {
                    *pstart_code_size = start_code_size;
                }
			}
			else if (begin_pos >= 0)
			{
				end_pos = pos + offset;
				break;
			}
			offset += start_code_size + pos;
		}
	}
	if (begin_pos < 0 || end_pos < 0)
	{
		return -1;
	}
	if (ppout)
	{
		*ppout = pdata + begin_pos;
	}

	if (pout_len)
	{
		*pout_len = end_pos - begin_pos;
	}
    
	return 0;
}
/*
extradata/avcc format:
bits	desc					value
8		version					always 0x01
8		avc profile				sps[0][1]
8		avc compatibility		sps[0][2]
8		avc level				sps[0][3]
6		reserver1				0xFC
2		NALULengthSizeMinusOne	NALU长度字节数减1， 取值通常为0，1，3代码1，2，4字节的长度(unually 1)
3		reserver2				0xE0
5		number of SPS NALUS(usually 1) SPSNum
16		SPS size
N		sps data ...
8		number of PPS NALUs (usually 1)
16		PPS size
N		pps data ...
*/
static struct avcc_context* demux_avcc(char* pdata, int len)
{
	if (!pdata || len < 6)
	{
		return NULL;
	}
	struct avcc_context* pavcc_ctx = NULL;
	if (0x1 == pdata[0])
	{
		pavcc_ctx = (struct avcc_context*)malloc(sizeof(struct avcc_context));
		bitstream_ctx* pbsc = bitstream_open(pdata, len);
		pavcc_ctx->version = read_byte(pbsc, 1);
		pavcc_ctx->profile = read_byte(pbsc, 1);
		pavcc_ctx->compatibility = read_byte(pbsc, 1);
		pavcc_ctx->level = read_byte(pbsc, 1);
		pavcc_ctx->reserved = read_bits(pbsc, 6);
		pavcc_ctx->nalu_length_size_minus_one = read_bits(pbsc, 2);
		pavcc_ctx->reserved2 = read_bits(pbsc, 3);
		pavcc_ctx->num_of_sps_nalu = read_bits(pbsc, 5);
		pavcc_ctx->ppsps = (sps_ctx**)malloc(sizeof(sps_ctx*)*pavcc_ctx->num_of_sps_nalu);
		for (int i = 0; i < pavcc_ctx->num_of_sps_nalu; i++)
		{
			char* pbuf = NULL;
			int spslen = read_byte(pbsc, pavcc_ctx->nalu_length_size_minus_one + 1);
			int buflen = get_remain_buf(pbsc, &pbuf);
			if (buflen < spslen)
			{
				lbtrace("spslen:%d = read_byte(pbsc, pavcc_ctx->NALULengthSizeMinusOne:%d + 1) > buflen:%d ", spslen, pavcc_ctx->nalu_length_size_minus_one + 1, buflen);
				assert(0);
				return NULL;
			}
			pavcc_ctx->ppsps[i] = demux_sps(pbuf, spslen);
			if (NULL == pavcc_ctx->ppsps[i])
			{
				lbtrace("parser %d sps failed", i);
				return NULL;
			}
		}
		// read pps
		pavcc_ctx->num_of_pps_nalu = read_bits(pbsc, 8);
		pavcc_ctx->ppssize = read_bits(pbsc, 16);
		pavcc_ctx->pps = (char*)malloc(pavcc_ctx->ppssize);
		read_bytes(pbsc, (uint8_t*)pavcc_ctx->pps, pavcc_ctx->ppssize);
		return pavcc_ctx;
	}
	else
	{
		lbtrace("Invalid avcc data, avcc version should be 0x01, but it is %0x", pdata[0]);
		return NULL;
	}
}

static int sps_to_avcc(char* psps_pps, int len, char* pavcc, int avcclen)
{
	if(NULL == psps_pps || len < 8)
	{
		lberror("Invalid psps_pps:%p or len:%d\n", psps_pps, len);
		return -1;
	}
	memset(pavcc, 0, avcclen);
	sps_ctx* pspsctx = demux_sps(psps_pps, len);
	if(NULL == pspsctx)
	{
		lberror("demux sps failed!\n");
		return -1;
	}
	
	bitstream_ctx* pbs = bitstream_open(pavcc, avcclen);
	if(NULL == pbs)
	{
		lberror("bitstream open failed!\n");
		return -1;
	}
	// write avcc version 0x01
	write_bits(pbs, 0x01, 8);
	// write profile
	write_bits(pbs, pspsctx->profile_idc, 8);
	// write compatibility
	write_bits(pbs, 0, 8);
	// write level
	write_bits(pbs, pspsctx->level_idc, 8);
	// write reserved(6bit), all bits on
	write_bits(pbs, 63, 6);
	// write NALU Length Size - 1(2bit), all on
	write_bits(pbs, 3, 2);
	// write reserved2(3bit), all bits on
	write_bits(pbs, 7, 3);
	// write sps nalu(5bit), usually 1
	write_bits(pbs, 1, 5);
	int startcodelen = 0;
	// find sps data
    char* psps = NULL;
    int spslen = 0;
    int ret = h264_get_nalu(psps_pps, len, 7, &psps, &spslen, &startcodelen);
	if(ret < 0 || spslen < 0)
	{
		lberror("find sps failed, ret:%d, spslen:%d\n", ret, spslen);
		return -1;
	}
	write_bits(pbs, spslen - startcodelen, 16);
	write_bytes(pbs, psps + startcodelen, spslen - startcodelen);
    char* pps = NULL;
	int ppslen = 0;
	startcodelen = 0;
	// find pps data
    ret = h264_get_nalu(psps_pps, len, 7, &pps, &ppslen, &startcodelen);
	if(ret < 0 || ppslen < 0)
	{
		lberror("find sps failed, ret:%d, spslen:%d\n", ret, spslen);
		return -1;
	}
	// write pps count
	write_bits(pbs, 1, 8);
	// write pps size
	write_bits(pbs, ppslen - startcodelen, 16);
	// write pps data
	write_bytes(pbs, pps + startcodelen, ppslen - startcodelen);
	avcclen = pbs->bytesoffset;
	bitstream_close(&pbs);
	return avcclen;
}

static int sps_pps_to_avcc(sps_ctx* pspsctx, char* pps, int ppslen, char* avcc, int avcclen)
{
	if (!pspsctx || !pspsctx->psps || !pps || !avcc)
	{
		lberror("Invalid parameter, pspsctx:%p, pspsctx->psps:%p, pps:%p, avcc:%p\n", pspsctx, pspsctx->psps, pps, avcc);
		return -1;
	}
    char* sps = pspsctx->psps;
    int spslen = pspsctx->sps_len;
	/*sps_ctx* pspsctx = demux_sps(sps, spslen);
	if (NULL == pspsctx)
	{
		lberror("demux sps failed!\n");
		return -1;
	}*/
	memset(avcc, 0, avcclen);
	bitstream_ctx* pbs = bitstream_open(avcc, avcclen);
	if (NULL == pbs)
	{
		lberror("bitstream open failed!\n");
		return -1;
	}
	// write avcc version 0x01
	write_bits(pbs, 0x01, 8);
	// write profile
	write_bits(pbs, pspsctx->profile_idc, 8);
	// write compatibility
	write_bits(pbs, 0, 8);
	// write level
	write_bits(pbs, pspsctx->level_idc, 8);
	// write reserved(6bit), all bits on
	write_bits(pbs, 63, 6);
	// write NALU Length Size - 1(2bit), all on
	write_bits(pbs, 3, 2);
	// write reserved2(3bit), all bits on
	write_bits(pbs, 7, 3);
	// write sps nalu(5bit), usually 1
	write_bits(pbs, 1, 5);
	int offset = 0;
	while (0 == sps[offset])
	{
		offset++;
	}
	if (offset > 0)
	{
		assert(sps[offset] == 1);
		offset++;
	}
	write_bits(pbs, spslen - offset, 16);
	write_bytes(pbs, sps + offset, spslen - offset);

	offset = 0;
	while (0 == pps[offset])
	{
		offset++;
	}
	if (offset > 0)
	{
		assert(pps[offset] == 1);
		offset++;
	}
	// write pps count
	write_bits(pbs, 1, 8);
	write_bits(pbs, ppslen - offset, 16);
	write_bytes(pbs, pps + offset, ppslen - offset);
	avcclen = pbs->bytesoffset;
	bitstream_close(&pbs);

	return avcclen;
}

static int get_avcc_packet_data(char* psrc, int src_len, char** ppdst, int* pdst_len, char** sps, int* psps_len, char** pps, int* ppps_len)
{
    int start_code_size = 0;
    int offset = 0;
    char* pdata = psrc;
    int size = src_len;
    int nalu_type = -1;
    int last_nalu_type = -1;
    while (offset < size)
    {
        int pos = find_start_code(pdata + offset, size - offset, &start_code_size, &nalu_type);
        if (pos < 0)
        {
            break;
        }
        else
        {
            if ((5 == nalu_type || 1 == nalu_type) && ppdst)
            {
                *ppdst = pdata + offset + pos + start_code_size;
                if(pdst_len)
                {
                    *pdst_len = src_len + psrc - *ppdst;
                }
                break;
            }
            else if(7 == nalu_type && sps)
            {
                *sps = pdata + pos + offset + start_code_size;
            }
            else if(8 == nalu_type && pps)
            {
                *pps = pdata + pos + offset + start_code_size;
            }
            if(7 == last_nalu_type && psps_len)
            {
                *psps_len = pdata + offset + pos - *sps;
            }
            else if(8 == last_nalu_type && ppps_len)
            {
                *ppps_len = pdata + offset + pos - *pps;
            }
            offset += pos + start_code_size;
            last_nalu_type = nalu_type;
        }
    }
    return NULL == *ppdst ? -1 : 0;
}
