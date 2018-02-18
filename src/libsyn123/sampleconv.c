/*
	sampleconv: libsyn123 sample conversion functions

	copyright 2018 by the mpg123 project
	licensed under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	initially written by Thomas Orgis

	This code directly contains wave generators and the generic entry
	code for libsyn123. The waves started the whole thing and stay here
	for now. Other signal generators go into separate files.

	This is the part of the mpg123 codebase where C99 first got into use
	for real, but still with some caution.
*/

#define NO_GROW_BUF
#include "syn123_int.h"
#include "sample.h"
#include "debug.h"

/* Conversions between native byte order encodings. */

#include "g711_impl.h"

/* Some trivial conversion functions. No need for another library. */

/* 1. From double/float to various. */

/* All symmetric, +/- 2^n-1. Clipping and conversion. */
/* One might optimize here. Or at least try to, in vain. */
#define CONV(name, ftype, type, maxval) \
static type name(ftype d) \
{ \
	type imax = maxval; \
	d *= imax; \
	if(d>=0) \
	{ \
		d += 0.5; \
		return d > imax \
		?	imax \
		:	(type)d; \
	} \
	else \
	{ \
		d -= 0.5; \
		return d < -imax \
		?	-imax \
		:	(type)d; \
	} \
}

/* If there is an actual 24 bit integer with cleared last byte, */
/* single precision should be accurate. Otherwise, double is needed. */
CONV(d_s32, double, int32_t, 2147483647)
CONV(f_s32, float, int32_t, 2147483647)
CONV(f_s16, float, int16_t, 32767)
CONV(f_s8,  float, int8_t,  127)

static uint8_t         f_u8(float f) { return CONV_SU8(f_s8(f));     }
static uint16_t       f_u16(float f) { return CONV_SU16(f_s16(f));   }
static uint32_t       d_u32(double d){ return CONV_SU32(d_s32(d));   }
static uint32_t       f_u32(float f){ return CONV_SU32(f_s32(f));   }
static unsigned char f_alaw(float f) { return linear2alaw(f_s16(f)); }
static unsigned char f_ulaw(float f) { return linear2ulaw(f_s16(f)); }

/* 2. From various to double/float. */

static double s32_d(int32_t n)      { return (double)n/2147483647.; }
static float  s32_f(int32_t n)      { return (float)n/2147483647.;  }
static float  s16_f(int16_t n)      { return (float)n/32767.;       }
static float  s8_f (int8_t n)       { return (float)n/127.;         }
static float  u8_f (uint8_t u)      { return s8_f(CONV_US8(u));     }
static float  u16_f(uint16_t u)     { return s16_f(CONV_US16(u));   }
static double u32_d(uint32_t u)     { return s32_d(CONV_US32(u));   }
static float  u32_f(uint32_t u)     { return s32_f(CONV_US32(u));   }
static float alaw_f(unsigned char n){ return s16_f(alaw2linear(n)); }
static float ulaw_f(unsigned char n){ return s16_f(ulaw2linear(n)); }

// All together in a happy matrix game, but only directly to/from
// double or float.

#define FROM_FLT(type) \
{ type *tsrc = src; type *tend = tsrc+samples; char *tdest = dest; \
switch(dest_enc) \
{ \
	case MPG123_ENC_SIGNED_16: \
		for(; tsrc!=tend; ++tsrc, tdest+=2) \
			*(int16_t*)tdest = f_s16(*tsrc); \
	break; \
	case MPG123_ENC_SIGNED_32: \
		for(; tsrc!=tend; ++tsrc, tdest+=4) \
			*(int32_t*)tdest = d_s32(*tsrc); \
	break; \
	case MPG123_ENC_SIGNED_24: \
		for(; tsrc!=tend; ++tsrc, tdest+=3) \
		{ \
			int32_t tmp = f_s32(*tsrc); \
			DROP4BYTE((char*)tdest, (char*)&tmp); \
		} \
	break; \
	case MPG123_ENC_SIGNED_8: \
		for(; tsrc!=tend; ++tsrc, tdest+=1) \
			*(int8_t*)tdest = f_s8(*tsrc); \
	break; \
	case MPG123_ENC_ULAW_8: \
		for(; tsrc!=tend; ++tsrc, tdest+=1) \
			*(unsigned char*)tdest = f_ulaw(*tsrc); \
	break; \
	case MPG123_ENC_ALAW_8: \
		for(; tsrc!=tend; ++tsrc, tdest+=1) \
			*(unsigned char*)tdest = f_alaw(*tsrc); \
	break; \
	case MPG123_ENC_UNSIGNED_8: \
		for(; tsrc!=tend; ++tsrc, tdest+=1) \
			*(uint8_t*)tdest = f_u8(*tsrc); \
	break; \
	case MPG123_ENC_UNSIGNED_16: \
		for(; tsrc!=tend; ++tsrc, tdest+=2) \
			*(uint16_t*)tdest = f_u16(*tsrc); \
	break; \
	case MPG123_ENC_UNSIGNED_24: \
		for(; tsrc!=tend; ++tsrc, tdest+=3) \
		{ \
			uint32_t tmp = f_u32(*tsrc); \
			DROP4BYTE((char*)tdest, (char*)&tmp); \
		} \
	break; \
	case MPG123_ENC_UNSIGNED_32: \
		for(; tsrc!=tend; ++tsrc, tdest+=4) \
			*(uint32_t*)tdest = d_u32(*tsrc); \
	break; \
	case MPG123_ENC_FLOAT_32: \
		for(; tsrc!=tend; ++tsrc, tdest+=4) \
			*((float*)tdest) = *tsrc; \
	break; \
	case MPG123_ENC_FLOAT_64: \
		for(; tsrc!=tend; ++tsrc, tdest+=8) \
			*(double*)tdest = *tsrc; \
	break; \
	default: \
		return SYN123_BAD_CONV; \
}}

#define TO_FLT(type) \
{ type* tdest = dest; type* tend = tdest + samples; char * tsrc = src; \
switch(src_enc) \
{ \
	case MPG123_ENC_SIGNED_16: \
		for(; tdest!=tend; ++tdest, tsrc+=2) \
			*tdest = s16_f(*(int16_t*)tsrc); \
	break; \
	case MPG123_ENC_SIGNED_32: \
		for(; tdest!=tend; ++tdest, tsrc+=4) \
			*tdest = s32_d(*(int32_t*)tsrc); \
	break; \
	case MPG123_ENC_SIGNED_24: \
		for(; tdest!=tend; ++tdest, tsrc+=3) \
		{ \
			int32_t tmp; \
			ADD4BYTE((char*)&tmp, (char*)tsrc); \
			*tdest = s32_f(tmp); \
		} \
	break; \
	case MPG123_ENC_SIGNED_8: \
		for(; tdest!=tend; ++tdest, tsrc+=1) \
			*tdest = s8_f(*(int8_t*)tsrc); \
	break; \
	case MPG123_ENC_ULAW_8: \
		for(; tdest!=tend; ++tdest, tsrc+=1) \
			*tdest = ulaw_f(*(unsigned char*)tsrc); \
	break; \
	case MPG123_ENC_ALAW_8: \
		for(; tdest!=tend; ++tdest, tsrc+=1) \
			*tdest = alaw_f(*(unsigned char*)tsrc); \
	break; \
	case MPG123_ENC_UNSIGNED_8: \
		for(; tdest!=tend; ++tdest, tsrc+=1) \
			*tdest = u8_f(*(uint8_t*)tsrc); \
	break; \
	case MPG123_ENC_UNSIGNED_16: \
		for(; tdest!=tend; ++tdest, tsrc+=2) \
			*tdest = u16_f(*(uint16_t*)tsrc); \
	break; \
	case MPG123_ENC_UNSIGNED_24: \
		for(; tdest!=tend; ++tdest, tsrc+=3) \
		{ \
			uint32_t tmp; \
			ADD4BYTE((char*)&tmp, (char*)tsrc); \
			*tdest = u32_f(tmp); \
		} \
	break; \
	case MPG123_ENC_UNSIGNED_32: \
		for(; tdest!=tend; ++tdest, tsrc+=4) \
			*tdest = u32_d(*(uint32_t*)tsrc); \
	break; \
	case MPG123_ENC_FLOAT_32: \
		for(; tdest!=tend; ++tdest, tsrc+=4) \
			*tdest = *(float*)tsrc; \
	break; \
	case MPG123_ENC_FLOAT_64: \
		for(; tdest!=tend; ++tdest, tsrc+=4) \
			*tdest = *(double*)tsrc; \
	break; \
	default: \
		return SYN123_BAD_CONV; \
}}

int attribute_align_arg
syn123_conv( void * MPG123_RESTRICT dest, int dest_enc, size_t dest_size
,	void * MPG123_RESTRICT src, int src_enc, size_t src_bytes
,	size_t *dest_bytes)
{
	size_t inblock  = MPG123_SAMPLESIZE(src_enc);
	size_t outblock = MPG123_SAMPLESIZE(dest_enc);
	size_t samples = src_bytes/inblock;
debug2("conv from %i to %i", src_enc, dest_enc);
	if(!inblock || !outblock)
		return SYN123_BAD_ENC;
	if(!dest || !src)
		return SYN123_BAD_BUF;
	if(samples*inblock != src_bytes)
		return SYN123_BAD_CHOP;
	if(samples*outblock > dest_size)
		return SYN123_BAD_SIZE;
	if(src_enc == dest_enc)
		memcpy(dest, src, samples*outblock);
	else if(src_enc & MPG123_ENC_FLOAT)
	{
		if(src_enc == MPG123_ENC_FLOAT_64)
			FROM_FLT(double)
		else if(src_enc == MPG123_ENC_FLOAT_32)
			FROM_FLT(float)
		else
			return SYN123_BAD_CONV;
	}
	else if(dest_enc & MPG123_ENC_FLOAT)
	{
		if(dest_enc == MPG123_ENC_FLOAT_64)
			TO_FLT(double)
		else if(dest_enc == MPG123_ENC_FLOAT_32)
			TO_FLT(float)
		else
			return SYN123_BAD_CONV;
	}
	else
		return SYN123_BAD_CONV;
	if(dest_bytes)
		*dest_bytes = outblock*samples;
	return SYN123_OK;
}

// Any compiler worth its salt should be able to unroll the
// fixed loops resultung from this macro. And yes, I should
// measure that. Goal is the hardware memory bandwidth.
// Any impact from putting the channel loop to the outside?

#define BYTEMULTIPLY(dest, src, bytes, count, channels) \
{ \
	for(size_t i=0; i<count; ++i) \
	{ \
		for(int j=0; j<channels; ++j) \
		for(size_t b=0; b<bytes; ++b) \
			((char*)dest)[(i*channels+j)*bytes+b] = ((char*)src)[i*bytes+b]; \
	} \
}

#define BYTEINTERLEAVE(dest, src, bytes, count, channels) \
{ \
	for(size_t i=0; i<count; ++i) \
	{ \
		for(int j=0; j<channels; ++j) \
		for(size_t b=0; b<bytes; ++b) \
			((char*)dest)[(i*channels+j)*bytes+b] = ((char**)src)[j][i*bytes+b]; \
	} \
}

#define BYTEDEINTERLEAVE(dest, src, bytes, count, channels) \
{ \
	for(size_t i=0; i<count; ++i) \
	{ \
		for(int j=0; j<channels; ++j) \
		for(size_t b=0; b<bytes; ++b) \
			((char**)dest)[j][i*bytes+b] = ((char*)src)[(i*channels+j)*bytes+b]; \
	} \
}

/* Special case of multiplying a mono stream. */
void attribute_align_arg
syn123_mono2many( void * MPG123_RESTRICT dest, void * MPG123_RESTRICT src
, int channels, size_t samplesize, size_t samplecount )
{
	switch(channels)
	{
		case 1:
			memcpy(dest, src, samplesize*samplecount);
		break;
		case 2:
			switch(samplesize)
			{
				case 1:
					BYTEMULTIPLY(dest, src, 1, samplecount, 2)
				break;
				case 2:
					BYTEMULTIPLY(dest, src, 2, samplecount, 2)
				break;
				case 3:
					BYTEMULTIPLY(dest, src, 3, samplecount, 2)
				break;
				case 4:
					BYTEMULTIPLY(dest, src, 4, samplecount, 2)
				break;
				default:
					BYTEMULTIPLY(dest, src, samplesize, samplecount, 2)
				break;
			}
		break;
		default:
			switch(samplesize)
			{
				case 1:
					BYTEMULTIPLY(dest, src, 1, samplecount, channels)
				break;
				case 2:
					BYTEMULTIPLY(dest, src, 2, samplecount, channels)
				break;
				case 3:
					BYTEMULTIPLY(dest, src, 3, samplecount, channels)
				break;
				case 4:
					BYTEMULTIPLY(dest, src, 4, samplecount, channels)
				break;
				default:
					BYTEMULTIPLY(dest, src, samplesize, samplecount, channels)
				break;
			}
		break;
	}
}

void attribute_align_arg
syn123_interleave(void * MPG123_RESTRICT dest, void ** MPG123_RESTRICT src
,	int channels, size_t samplesize, size_t samplecount)
{
	switch(channels)
	{
		case 1:
			memcpy(dest, src, samplesize*samplecount);
		break;
		case 2:
			switch(samplesize)
			{
				case 1:
					BYTEINTERLEAVE(dest, src, 1, samplecount, 2)
				break;
				case 2:
					BYTEINTERLEAVE(dest, src, 2, samplecount, 2)
				break;
				case 3:
					BYTEINTERLEAVE(dest, src, 3, samplecount, 2)
				break;
				case 4:
					BYTEINTERLEAVE(dest, src, 4, samplecount, 2)
				break;
				default:
					BYTEINTERLEAVE(dest, src, samplesize, samplecount, 2)
				break;
			}
		break;
		default:
			switch(samplesize)
			{
				case 1:
					BYTEINTERLEAVE(dest, src, 1, samplecount, channels)
				break;
				case 2:
					BYTEINTERLEAVE(dest, src, 2, samplecount, channels)
				break;
				case 3:
					BYTEINTERLEAVE(dest, src, 3, samplecount, channels)
				break;
				case 4:
					BYTEINTERLEAVE(dest, src, 4, samplecount, channels)
				break;
				default:
					BYTEINTERLEAVE(dest, src, samplesize, samplecount, channels)
				break;
			}
		break;
	}
}

void attribute_align_arg
syn123_deinterleave(void ** MPG123_RESTRICT dest, void * MPG123_RESTRICT src
,	int channels, size_t samplesize, size_t samplecount)
{
	switch(channels)
	{
		case 1:
			memcpy(dest[0], src, samplesize*samplecount);
		break;
		case 2:
			switch(samplesize)
			{
				case 1:
					BYTEDEINTERLEAVE(dest, src, 1, samplecount, 2)
				break;
				case 2:
					BYTEDEINTERLEAVE(dest, src, 2, samplecount, 2)
				break;
				case 3:
					BYTEDEINTERLEAVE(dest, src, 3, samplecount, 2)
				break;
				case 4:
					BYTEDEINTERLEAVE(dest, src, 4, samplecount, 2)
				break;
				default:
					BYTEDEINTERLEAVE(dest, src, samplesize, samplecount, 2)
				break;
			}
		break;
		default:
			switch(samplesize)
			{
				case 1:
					BYTEDEINTERLEAVE(dest, src, 1, samplecount, channels)
				break;
				case 2:
					BYTEDEINTERLEAVE(dest, src, 2, samplecount, channels)
				break;
				case 3:
					BYTEDEINTERLEAVE(dest, src, 3, samplecount, channels)
				break;
				case 4:
					BYTEDEINTERLEAVE(dest, src, 4, samplecount, channels)
				break;
				default:
					BYTEDEINTERLEAVE(dest, src, samplesize, samplecount, channels)
				break;
			}
		break;
	}
}

/* All the byte-swappery for those little big endian boxes. */

#include "swap_bytes_impl.h"

void attribute_align_arg
syn123_swap_bytes(void* buf, size_t samplesize, size_t samplecount)
{
	swap_bytes(buf, samplesize, samplecount);
}

void attribute_align_arg
syn123_host2le(void *buf, size_t samplesize, size_t samplecount)
{
#ifdef WORDS_BIGENDIAN
	swap_bytes(buf, samplesize, samplecount);
#endif
}

void attribute_align_arg
syn123_host2be(void *buf, size_t samplesize, size_t samplecount)
{
#ifndef WORDS_BIGENDIAN
	swap_bytes(buf, samplesize, samplecount);
#endif
}

void attribute_align_arg
syn123_le2host(void *buf, size_t samplesize, size_t samplecount)
{
#ifdef WORDS_BIGENDIAN
	swap_bytes(buf, samplesize, samplecount);
#endif
}

void attribute_align_arg
syn123_be2host(void *buf, size_t samplesize, size_t samplecount)
{
#ifndef WORDS_BIGENDIAN
	swap_bytes(buf, samplesize, samplecount);
#endif
}
