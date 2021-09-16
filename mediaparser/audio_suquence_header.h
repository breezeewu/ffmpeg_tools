#ifndef AUDIO_SEQ_HDR_H_
#define AUDIO_SEQ_HDR_H_
#include <stdio.h>
#include <stdint.h>
#include "bitstream.h"
#include <assert.h>
//#include "lazylog.h"
//#include "bitstream.h"
//#define lbtrace AVTRACE
//#define lberror AVTRACE
#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif
enum e_aac_object_type
{
	e_aac_object_type_reserved = 0,
	e_aac_object_type_main		= 1,
	e_aac_object_type_lc		= 2,
	e_aac_object_type_ssr		= 3,
    e_aac_object_type_ltp       = 4,
	e_aac_object_type_sbr		= 5,
    e_aac_object_type_scalable  = 6,
    e_aac_object_type_twinvq    = 7,
    e_aac_object_type_celp      = 8,
    e_aac_object_type_hvxc      = 9,
    e_aac_object_type_reserved1 = 10,
    e_aac_object_type_reserved2 = 11,
    e_aac_object_type_ttsi      = 12,
    e_aac_object_type_main_synthetic = 13,
    e_aac_object_type_wavetable_synthesis = 14,
    e_aac_object_type_general_midi = 15,
    e_aac_object_type_algorithmic_synthesis = 16,
	// AAC HEv2 = LC+SBR+PS
	e_aac_object_type_hev2		= 29
};
//0:AAC Main, 1: AAC LC(low complexity), 2:AAC SSR(Scalable Sample Rate), 3: AAC LTP(Long Term Prediction)
enum e_aac_profile
{
	e_aac_profile_main		= 0,
	e_aac_profile_lc		= 1,
	e_aac_profile_ssr		= 2,
	e_aac_profile_ltp		= 3
};

typedef struct adts_context
{
    uint16_t syncword;
    uint8_t id;
    uint8_t layer;
    uint8_t protection_absent;
    uint8_t profile;
    uint8_t sampleing_frequency_index;
    uint8_t private_bit;
    uint8_t channel_configuration;
    uint8_t original_copy;
    uint8_t home;
    uint32_t samplerate;

    // adts variable header
    uint8_t copyright_identification_bit;
    uint8_t copyright_identification_start;
    uint16_t aac_frame_length;
    uint16_t adts_buffer_fullness;
    uint16_t number_of_raw_data_blocks_in_frame;

    uint16_t crc;
    uint16_t adts_header_size;
} adts_ctx;
/*static int lbread_bits(const char* data, int bits, int* pbit_offset)
{
    int bytesoff = 0;
    int byte_bit_off  = 0;
    int val = 0;
    int bit_offset = *pbit_offset;
    if(bit_offset > 7)
    {
        bytesoff = bit_offset/8;
        byte_bit_off = bit_offset % 8;
    }
    
    for(int i = 0; i < bits; i++)
    {
        val <<= 1;
        val += (data[bytesoff] >> (7 - byte_bit_off)) & 0x1;
        bytesoff += (byte_bit_off + 1)/8;
        byte_bit_off = (byte_bit_off + 1)%8;
    }
    bit_offset += bits;
    *pbit_offset = bit_offset;
    return val;
}

static int lbwrite_bits(char* data, int val, int bits, int* poffset)
{
    int bytesoff = 0;
    int offset = *poffset;
    bytesoff = offset / 8;
    int bit_offset = offset % 8;
    int write_bits = bits;
    while (bits > 0)
    {
        char cval = val >> (bits - 1);
        cval = (cval & 0x1) << (7 - bit_offset);
        cval = (char)cval & 0xff;
        data[bytesoff] = cval | data[bytesoff];

        //val <<= 1;
        //val += (data[bytesoff] >> (7 - offset)) & 0x1;
        bytesoff += (bit_offset + 1) / 8;
        bit_offset = (bit_offset + 1) % 8;
        bits--;
    }
    offset += write_bits;
    *poffset = offset;
    return val;
}*/

static int get_samplerate(adts_ctx* padtsctx)
{
    if(!padtsctx)
    {
        return -1;
    }
    
   int samplerate = 0;
   switch(padtsctx->sampleing_frequency_index)
   {
       case 0:
           samplerate = 96000;
           break;
       case 1:
           samplerate = 88200;
           break;
       case 2:
           samplerate = 64000;
           break;
       case 3:
           samplerate = 48000;
           break;
       case 4:
           samplerate = 44100;
           break;
       case 5:
           samplerate = 32000;
           break;
       case 6:
           samplerate = 24000;
           break;
       case 7:
           samplerate = 22050;
           break;
       case 8:
           samplerate = 16000;
           break;
       case 9:
           samplerate = 12000;
           break;
       case 10:
           samplerate = 11025;
           break;
       case 11:
           samplerate = 8000;
           break;
       case 12:
           samplerate = 7350;
           break;
       case 13:
           samplerate = 0;
           break;
       case 14:
           samplerate = 0;
           break;
       case 15:
           samplerate = 0;
           break;
       default:
           samplerate = -1;
           break;
   }

   return samplerate;
}
static int code_type_from_aac_profile(int profile)
{
    switch(profile)
    {
        case e_aac_profile_main:
            return e_aac_object_type_main;
        case e_aac_profile_lc:
            return e_aac_object_type_lc;
        case e_aac_profile_ssr:
            return e_aac_object_type_ssr;
        default:
            return e_aac_object_type_reserved;
    }
}
static int profile_from_aac_obj_type(int obj_type)
{
    switch (obj_type) {
        case e_aac_object_type_main:
            return e_aac_profile_main;
        case e_aac_object_type_lc:
            return e_aac_profile_lc;
        case e_aac_object_type_ssr:
            return e_aac_profile_ssr;
        default:
            return -1;
            break;
    }
}

static adts_ctx* adts_demux_sequence_header(const char* seq, int len)
{
    if(NULL == seq || len < 2)
    {
        lberror("adts_demux_sequence_header invalid parameter seq:%p, len:%d\n", seq, len);
        return NULL;
    }
    adts_ctx* padtsctx = (adts_ctx*)malloc(sizeof(adts_ctx));
    memset(padtsctx, 0, sizeof(adts_ctx));
    padtsctx->syncword = 0xfff;
    // 0:mpeg-4, 1 mpeg-2
    padtsctx->id = 0;
    // always:00
    padtsctx->layer = 0;
    // no crc 1, crc : 0, adts header is 9 bytes, with 2 bytes of crc
    padtsctx->protection_absent = 1;
    if(padtsctx->protection_absent)
    {
        padtsctx->adts_header_size = 7;
    }
    else
    {
        padtsctx->adts_header_size = 9;
    }
    bitstream_ctx* pbs = bitstream_open((char*)seq, len);
    int objtype = read_bits(pbs, 5);
    padtsctx->profile = profile_from_aac_obj_type(objtype);
    padtsctx->sampleing_frequency_index = read_bits(pbs, 4);
    if(15 == padtsctx->sampleing_frequency_index)
    {
        padtsctx->samplerate = read_bits(pbs, 24);
    }
    padtsctx->private_bit = 0;
    padtsctx->channel_configuration = read_bits(pbs, 4);
    padtsctx->original_copy = 0;
    int reserved = read_bits(pbs, 3);
    assert(0==reserved);
    padtsctx->home = 0;
    padtsctx->copyright_identification_bit = 0;
    padtsctx->copyright_identification_start = 0;
    padtsctx->adts_buffer_fullness = 0x7ff;
    padtsctx->number_of_raw_data_blocks_in_frame = 0;
    
    return padtsctx;
    /*int objtype = code_type_from_aac_profile((int)padtsctx->profile);
    // 5 bits audio object type
    //write_bits(pbs, objtype, 5);
    write_bits(pbs, objtype, 5);//write_bits(pseq, objtype, 5, bit_offset);
    // 4 bits sample frequence index
    write_bits(pbs, padtsctx->sampleing_frequency_index, 4);////write_bits(pseq, padtsctx->sampleing_frequency_index, 4, bit_offset);
    //write_bits(pbs, m_sampleing_frequency_index, 4);
    // if custom samplerate
    if(15 == padtsctx->sampleing_frequency_index)
    {
        write_bits(pbs, padtsctx->samplerate, 24);//write_bits(pseq, padtsctx->samplerate, 24, bit_offset);
        //write_bits(pbs, m_samplerate, 24);
    }
    // 4 bits channel configuration,
    //write_bits(pbs, m_channel_configuration, 4);
    write_bits(pbs, padtsctx->channel_configuration, 4);////write_bits(pseq, padtsctx->channel_configuration, 4, bit_offset);
    // 3 bits 0
    //write_bits(pbs, 0, 3);
    write_bits(pbs, 0, 3);//write_bits(pseq, 0, 3, bit_offset);*/
}
static adts_ctx* adts_demux_open(const char* padts_header, int len)
{
    int bit_offset = 0;
    if(!padts_header || len < 7)
    {
        lberror("Invalid adts header padts_header:%p, len:%d\n", padts_header, len);
        lberror("header %0x, %0x", (int)padts_header[0], (int)padts_header[1]);
        return NULL;
    }
    
    adts_ctx* padtsctx = malloc(sizeof(adts_ctx));
    const uint8_t* ptmp = (const uint8_t*)padts_header;
    // read adts fixed header
    // read sync word 12 bit
    struct bitstream_ctx* pbs = bitstream_open(padts_header, len);
    uint16_t syncword = read_bits(pbs, 12);
    //uint16_t syncword = read_bits(padts_header, 12, bit_offset); // 12
    if(0xfff != syncword)
    {
        lberror("Invalid adts header, syncword:%03x",syncword);
        return NULL;
    }
    // read id(1 bit), MPEG flag, 0:mpeg-4, 1:mpeg-2
    padtsctx->id = read_bits(pbs, 1);//lbread_bits(padts_header, 1, bit_offset); // 13
    // read layer(2 bit)(alaws 00),
    padtsctx->layer = read_bits(pbs, 2);//lbread_bits(padts_header, 2, bit_offset); // 15
    // read protection_absent(1bit), crc check flag, 0:crc check, 1: no crc check, if 1, adts header length = 9bytes, include 2 bytes crc check
    padtsctx->protection_absent = read_bits(pbs, 1);//lbread_bits(padts_header, 1, bit_offset); // 16
    // read aac profile, 2bit, 0:AAC Main, 1: AAC LC(low complexity), 2:AAC SSR(Scalable Sample Rate), 3: AAC LTP(Long Term Prediction)
    padtsctx->profile = read_bits(pbs, 2);//lbread_bits(padts_header, 2, bit_offset); // 18
    // read sampleing frequency index for aac sampling rate index
    padtsctx->sampleing_frequency_index = read_bits(pbs, 4);//lbread_bits(padts_header, 4, bit_offset); // 22
    // private bit, 1bit, encoder set to 0, decoder ingore it
    padtsctx->private_bit = read_bits(pbs, 1);//lbread_bits(padts_header, 1, bit_offset); // 23
    // channele count, 3bit, indicate channel count
    padtsctx->channel_configuration = read_bits(pbs, 3);//read_bits(padts_header, 3, bit_offset); // 26
    // origin copy, 1bit, encoder set to 0, decoder ingore it
    padtsctx->original_copy = read_bits(pbs, 1);//read_bits(padts_header, 1, bit_offset); // 27
    // home, 1bit, encoder set to 0, decoder ingore it
    padtsctx->home = read_bits(pbs, 1);//read_bits(padts_header, 1, bit_offset); // 28

    // read adts variable header
    // copyright idenitfication bit, 1bit, encoder set to 0, decoder ingore it
    padtsctx->copyright_identification_bit  = read_bits(pbs, 1);//read_bits(padts_header, 1, bit_offset); // 29
    // copyright idenitfication start, 1bit, encoder set to 0, decoder ingore it
    padtsctx->copyright_identification_start = read_bits(pbs, 1);//read_bits(padts_header, 1, bit_offset); // 30
    // aac frame length(include adts header and aac frame data), 13 bit, encoder set to 0, decoder ingore it
    padtsctx->aac_frame_length = read_bits(pbs, 13);//read_bits(padts_header, 13, bit_offset); // 43
    // adts buffer fullness, 3bit, (alaws 0x7ff)
    padtsctx->adts_buffer_fullness = read_bits(pbs, 11);//read_bits(padts_header, 11, bit_offset); // 54
    // number of raw data block in frame, 2bit, current frame's raw sample count - 1
    padtsctx->number_of_raw_data_blocks_in_frame = read_bits(pbs, 2);//read_bits(padts_header, 2, bit_offset); // 56
    padtsctx->samplerate = get_samplerate(padtsctx);
    if(!padtsctx->protection_absent)
    {
        padtsctx->crc = read_bits(pbs, 16);//read_bits(padts_header, 16, bit_offset); // 72
    }
    return padtsctx;
}



static void adts_demux_close(adts_ctx** ppadtsctx)
{
    if(ppadtsctx && *ppadtsctx)
    {
        adts_ctx* padtsctx = *ppadtsctx;
        free(padtsctx);
        *ppadtsctx = padtsctx = NULL;
    }
}

static int mux_sequence_header(adts_ctx* padtsctx, char* pseq, int len)
{
    if(!padtsctx || !pseq || len < 2)
    {
        lberror("Invalid parameter, padtsctx:%p, pseq:%p, len:%d\n", padtsctx, pseq, len);
        return -1;
    }
    memset(pseq, 0, len);
    bitstream_ctx* pbs = bitstream_open(pseq, len);
    int objtype = code_type_from_aac_profile((int)padtsctx->profile);
    // 5 bits audio object type
    //write_bits(pbs, objtype, 5);
    write_bits(pbs, objtype, 5);//write_bits(pseq, objtype, 5, bit_offset);
    // 4 bits sample frequence index
    write_bits(pbs, padtsctx->sampleing_frequency_index, 4);////write_bits(pseq, padtsctx->sampleing_frequency_index, 4, bit_offset);
    //write_bits(pbs, m_sampleing_frequency_index, 4);
    // if custom samplerate
    if(15 == padtsctx->sampleing_frequency_index)
    {
        write_bits(pbs, padtsctx->samplerate, 24);//write_bits(pseq, padtsctx->samplerate, 24, bit_offset);
        //write_bits(pbs, m_samplerate, 24);
    }
    // 4 bits channel configuration,
    //write_bits(pbs, m_channel_configuration, 4);
    write_bits(pbs, padtsctx->channel_configuration, 4);////write_bits(pseq, padtsctx->channel_configuration, 4, bit_offset);
    // 3 bits 0
    //write_bits(pbs, 0, 3);
    write_bits(pbs, 0, 3);//write_bits(pseq, 0, 3, bit_offset);
    return 2;
}

static int muxer_adts_header(adts_ctx* padtsctx, int aac_data_len, char* padts_hdr, int adts_hdr_len)
{
    if(NULL == padtsctx || NULL == padts_hdr || adts_hdr_len < 7)
    {
        lberror("muxer_adts_header Invalid parameter, padtsctx:%p, padts_hdr:%p, adts_hdr_len:%d\n", padtsctx, padts_hdr, adts_hdr_len);
        return -1;
    }
    memset(padts_hdr, 0, padtsctx->adts_header_size);
    bitstream_ctx* pbs = bitstream_open(padts_hdr, adts_hdr_len);
    // adts fixed header
    write_bits(pbs, 0xfff, 12); // syncword 12 bits
    write_bits(pbs, 0, 1);      // ID, 1 bit
    write_bits(pbs, 0, 2);      // layer 2 bit
    write_bits(pbs, 1, 1);      // protection absent 1 bit, 1:no crc, 0: have crc
    write_bits(pbs, padtsctx->profile, 2);      // profile 2 bits
    write_bits(pbs, padtsctx->sampleing_frequency_index, 4);      // sampleing_frequency_index 4 bits
    write_bits(pbs, 0, 1);  // private bit, 1 bit, 0
    write_bits(pbs, padtsctx->channel_configuration, 3);  // channel_configuration 3 bits
    write_bits(pbs, 0, 1);  // original_copy, 1 bit, must be 0
    write_bits(pbs, 0, 1);  // channel_configuration 1 bits, must be 0
    
    // adts variable header
    write_bits(pbs, 0, 1);  // copyright_identification_bit, 1 bit, must be 0
    write_bits(pbs, 0, 1);  // copyright_identification_start 1 bits, must be 0
    write_bits(pbs, padtsctx->adts_header_size + aac_data_len, 13);  // aac_frame_length, 13 bit
    write_bits(pbs, 0x7ff, 11);  // adts buffer fullness 11 bits, must be 0x7ff
    write_bits(pbs, 0, 2);  // number_of_raw_data_blocks_in_frame, 2 bits, must be 0
    
    return padtsctx->adts_header_size;
}
/*static int codec_type_from_aac_profile(int profile)
{
    switch(profile)
    {
        case e_aac_profile_main:
            return e_aac_object_type_main;
        case e_aac_profile_lc:
            return e_aac_object_type_lc;
        case e_aac_profile_ssr:
            return e_aac_object_type_ssr;
        default:
            return e_aac_object_type_reserved;
    }
}*/

/*class adts_demux
{
public:
    adts_demux()
    {
        m_id    = 0;
        m_layer = 0;
        m_protection_absent = 0;
        m_profile = 0;
        m_sampleing_frequency_index = 0;
        m_private_bit = 0;
        m_channel_configuration = 0;
        m_original_copy = 0;
        m_home = 0;
        m_copyright_identification_bit = 0;
        m_copyright_identification_start = 0;
        m_aac_frame_length = 0;
        m_adts_buffer_fullness = 0;
        m_number_of_raw_data_blocks_in_frame = 0;
        m_crc = 0;
		//= {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0,0,0};
    }
    ~adts_demux()
    {

    }

    
    
	int get_samplerate_frequence_index()
	{
		return m_sampleing_frequency_index;
	}
    int channel()
    {
        return m_channel_configuration;
    }
protected:
	
protected:
    // adts fixed header
    uint8_t m_id;
    uint8_t m_layer;
    uint8_t m_protection_absent;
    uint8_t m_profile;
    uint8_t m_sampleing_frequency_index;
    uint8_t m_private_bit;
    uint8_t m_channel_configuration;
    uint8_t m_original_copy;
    uint8_t m_home;
	uint32_t	m_samplerate;

    // adts variable header
    uint8_t m_copyright_identification_bit;
    uint8_t m_copyright_identification_start;
    uint16_t m_aac_frame_length;
    uint16_t m_adts_buffer_fullness;
    uint16_t m_number_of_raw_data_blocks_in_frame;

    uint16_t m_crc;
    uint32_t m_sampling_frequency_index[16];// = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0,0,0};
};*/
#endif
