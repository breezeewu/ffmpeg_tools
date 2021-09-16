#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
// bitstream context struct
typedef struct bitstream_context
{
	char*	pdata;		// ���������ݻ���ָ��
	int		datalen;	// ���������ݳ���
	int		bitsoffset; // ����ƫ��
	int		bytesoffset;// �ֽ�ƫ��
	int		little_endian; // С�α�ʶ�� 1��ΪС�ˣ�0��Ϊ���, Ĭ��ΪС��

} bitstream_ctx;

// open bitstream context 
static bitstream_ctx* bitstream_open(char* pbuf, int len)
{
	struct bitstream_context* pbsc = (struct bitstream_context*)malloc(sizeof(struct bitstream_context));
	pbsc->pdata			= pbuf;
	pbsc->datalen		= len;
	pbsc->bitsoffset	= 0;
	pbsc->bytesoffset	= 0;
	pbsc->little_endian = 1;

	return pbsc;
}

// close bitsteam context and free resource
static void bitstream_close(bitstream_ctx** ppbsc)
{
	if (ppbsc && *ppbsc)
	{
		bitstream_ctx* pbsc = *ppbsc;
		free(pbsc);
		pbsc = NULL;
		*ppbsc = NULL;
	}
}

static int move(bitstream_ctx* pbsc, int bytes_offset, int bits_offset)
{
	pbsc->bytesoffset += bytes_offset;
	pbsc->bitsoffset += bits_offset;
	pbsc->bytesoffset += pbsc->bitsoffset / 8;
	pbsc->bitsoffset = pbsc->bitsoffset % 8;
	assert(pbsc->bytesoffset <= pbsc->datalen);
	return pbsc->bytesoffset;
}

static int read_bits(bitstream_ctx* pbsc, int bits)
{
	if (pbsc && pbsc->pdata && pbsc->datalen > pbsc->bytesoffset)
	{
		int readval = 0;
		while (bits > 0)
		{
			int i;
			int readbits = 8 - pbsc->bitsoffset;
			readbits = readbits >= bits ? bits : readbits;
			for (i = 0; i < readbits; i++)
			{
				readval = (readval << 1) | ((pbsc->pdata[pbsc->bytesoffset]>> (7 - pbsc->bitsoffset)) & 0x1);
				//pbsc->bitsoffset++;
				move(pbsc, 0, 1);
			}

			if (pbsc->bitsoffset >= 8)
			{
				assert(8 == pbsc->bitsoffset);
				pbsc->bytesoffset++;
				pbsc->bitsoffset = 0;
			}
			bits -= readbits;
		}

		return readval;
	}
	else
	{
		assert(0);
		return -1;
	}
}

static int64_t read_byte(bitstream_ctx* pbsc, int bytes)
{
	if (pbsc && pbsc->pdata && pbsc->datalen > pbsc->bytesoffset)
	{
		int64_t value = 0;
		char* pp = (char*)&value;
		int i;
		assert(0 == pbsc->bitsoffset);
		for (i = 0; i < bytes; i++)
		{
			if (pbsc->little_endian)
			{
				pp[bytes - i - 1] = pbsc->pdata[pbsc->bytesoffset++];
			}
			else
			{
				pp[i] = pbsc->pdata[pbsc->bytesoffset++];
			}
		}
		return value;
	}
	else
	{
		assert(0);
		return -1;
	}
}

static int read_bytes(bitstream_ctx* pbsc, uint8_t* pbuf, int len)
{
	if (pbsc && pbsc->datalen - pbsc->bytesoffset >= len)
	{
		memcpy(pbuf, pbsc->pdata + pbsc->bytesoffset, len);
		return len;
	}

	return 0;
}

static int write_bits(bitstream_ctx* pbsc, int64_t val, int bits)//(char* data, int val, int bits, int& offset)
{
	int bytesoff = 0;
	//bytesoff = pbsc->bitsoffset / 8;
	//int bit_offset = offset % 8;
	//int write_bits = bits;
	while (bits > 0)
	{
		char cval = val >> (bits - 1);
		cval = (cval & 0x1) << (7 - pbsc->bitsoffset);
		cval = (char)cval & 0xff;
		pbsc->pdata[pbsc->bytesoffset] = cval | pbsc->pdata[pbsc->bytesoffset];

		//val <<= 1;
		//val += (data[bytesoff] >> (7 - offset)) & 0x1;
		//pbsc->bytesoffset += (pbsc->bitsoffset + 1) / 8;
		move(pbsc, 0, 1);
		//pbsc->bitsoffset = (pbsc->bitsoffset + 1) % 8;
		bits--;
	}

	return val;
}

/*static int write_bits(bitstream_ctx* pbsc, int64_t val, int bits)
{
	if (pbsc && pbsc->pdata && pbsc->datalen > pbsc->bytesoffset && bits <= sizeof(int64_t)*8)
	{
		for (int i = bits - 1; i >= 0; i--)
		{
			int8_t byteval = 0xff & (((val >> i) & 0x01) << pbsc->bitsoffset);
			pbsc->pdata[pbsc->bytesoffset] = pbsc->pdata[pbsc->bytesoffset] & byteval;
			move(pbsc, 0, 1);
		}

		return bits;
	}
	else
	{
		assert(0);
		return -1;
	}
}*/

static int write_bytes(bitstream_ctx* pbsc, char* pbuf, int bytes)
{
	if (pbsc && pbsc->pdata && pbsc->datalen >= pbsc->bytesoffset + bytes)
	{
		memcpy(&pbsc->pdata[pbsc->bytesoffset], pbuf, bytes);
		pbsc->bytesoffset += bytes;
		return bytes;
	}

	return -1;
}
/*static int write_bytes(bitstream_ctx* pbsc, int64_t value, int bytes)
{
	if (pbsc && pbsc->pdata && pbsc->datalen > pbsc->bytesoffset + bytes && bytes <= sizeof(int64_t))
	{
		//int64_t value = 0;
		char* pp = (char*)&value;
		assert(0 == pbsc->bitsoffset);
		for (int i = 0; i < bytes; i++)
		{
			if (pbsc->little_endian)
			{
				pbsc->pdata[pbsc->bytesoffset] = pp[bytes - i - 1];
			}
			else
			{
				pbsc->pdata[pbsc->bytesoffset] = pp[i];
			}
			move(pbsc, 1, 0);
		}
		return bytes;
	}
	else
	{
		assert(0);
		return -1;
	}
}*/
static int bs_eof(bitstream_ctx* pbsc)
{
	return pbsc->bytesoffset >= pbsc->datalen ? 1 : 0;
}
static int64_t
read_ue(bitstream_ctx* pbsc)
{
	int i = 0;
	while (read_bits(pbsc, 1) == 0 && !bs_eof(pbsc) && i < 32)
		i++;
	return ((1 << i) - 1 + read_bits(pbsc, i));;
	/*while (nal_bs_read(bs, 1) == 0 && !nal_bs_eos(bs) && i < 32)
		i++;

	return ((1 << i) - 1 + nal_bs_read(bs, i));*/
}
/*static int read_ue(bitstream_ctx* pbsc)
{
	uint32_t nZeroNum = 0;
	while (pbsc->bitsoffset < pbsc->datalen * 8)
	{
		if (pbsc->pdata[pbsc->bitsoffset / 8] & (0x80 >> (pbsc->bitsoffset % 8)))
		{
			break;
		}
		nZeroNum++;
		//pbsc->bitsoffset++;
		move(pbsc, 0, 1);
	}
	move(pbsc, 0, 1);

	uint32_t dwRet = 0;
	for (uint32_t i = 0; i < nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pbsc->pdata[pbsc->bitsoffset / 8] & (0x80 >> (pbsc->bitsoffset % 8)))
		{
			dwRet += 1;
		}
		move(pbsc, 0, 1);
	}
	return (1 << nZeroNum) - 1 + dwRet;
}*/

static int read_se(bitstream_ctx* pbsc)
{
	int32_t ueval = read_ue(pbsc);
	double k = ueval;
	int32_t nvalue = ceil(k / 2);
	if (ueval % 2 == 0)
	{
		nvalue = -nvalue;
	}

	return nvalue;
}

static int get_remain_buf(bitstream_ctx* pbsc, char** ppbuf)
{
	if (!pbsc || !ppbuf || pbsc->datalen - pbsc->bytesoffset <= 0)
	{
		return 0;
	}

	*ppbuf = pbsc->pdata + pbsc->bytesoffset;
	return pbsc->datalen - pbsc->bytesoffset;
}
