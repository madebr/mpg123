/*
	decode.h: common definitions for decode functions

	This file is strongly tied with optimize.h concerning the synth functions.
	Perhaps one should restructure that a bit.

	copyright 2007-8 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, taking WRITE_SAMPLE from decode.c
*/
#ifndef MPG123_DECODE_H
#define MPG123_DECODE_H

/* The actual storage of a decoded sample is separated in the following macros.
   We can handle different types, we could also handle dithering here. */

/* Macro to produce a short (signed 16bit) output sample from internal representation,
   which may be float, double or indeed some integer for fixed point handling. */
#define WRITE_SHORT_SAMPLE(samples,sum,clip) \
  if( (sum) > REAL_PLUS_32767) { *(samples) = 0x7fff; (clip)++; } \
  else if( (sum) < REAL_MINUS_32768) { *(samples) = -0x8000; (clip)++; } \
  else { *(samples) = REAL_TO_SHORT(sum); }

/* Same for unsigned short. Not used yet. */
#define WRITE_USHORT_SAMPLE(samples,sum,clip) \
	if( (sum) > REAL_PLUS_32767) { *(samples) = 0xffff; (clip)++; } \
	else if( (sum) < REAL_MINUS_32768) { *(samples) = 0x0000; (clip)++; } \
	else { *(samples) = REAL_TO_SHORT(sum)+32768; }

/*
	32bit signed 
	We do clipping with the same old borders... but different conversion.
	We see here that we need extra work for non-16bit output... we optimized for 16bit.
*/
#define WRITE_S32_SAMPLE(samples,sum,clip) \
	{ \
		real tmpsum = REAL_MUL((sum),S32_RESCALE); \
		if( tmpsum > REAL_PLUS_S32 ){ *(samples) = 0x7fffffff; (clip)++; } \
		else if( tmpsum < REAL_MINUS_S32 ) { *(samples) = -0x80000000; (clip)++; } \
		else { *(samples) = (int32_t)tmpsum; } \
	}

/* Produce an 8bit sample, via 16bit intermediate. */
#define WRITE_8BIT_SAMPLE(samples,sum,clip) \
{ \
	short write_8bit_tmp; \
	if( (sum) > REAL_PLUS_32767) { write_8bit_tmp = 0x7fff; (clip)++; } \
	else if( (sum) < REAL_MINUS_32768) { write_8bit_tmp = -0x8000; (clip)++; } \
	else { write_8bit_tmp = REAL_TO_SHORT(sum); } \
	*(samples) = fr->conv16to8[write_8bit_tmp>>AUSHIFT]; \
}

/* Selection of class of output routines for basic format. */
#define OUT_16 0
#define OUT_8  1
#ifndef REAL_IS_FIXED
/* Write a floating point sample (that is, one matching the internal real type). */
#define WRITE_REAL_SAMPLE(samples,sum,clip) *(samples) = ((real)1./SHORT_SCALE)*(sum)
#define OUT_FORMATS 4 /* Basic output formats: 16bit, 8bit, real and s32 */
#define OUT_REAL 2
#define OUT_S32 3
#else
#define OUT_FORMATS 2 /* Basic output formats: 16bit and 8bit */
#endif

#define NTOM_MAX 8          /* maximum allowed factor for upsampling */
#define NTOM_MAX_FREQ 96000 /* maximum frequency to upsample to / downsample from */
#define NTOM_MUL (32768)

/* Let's collect all possible synth functions here, for an overview.
   If they are actually defined and used depends on preprocessor machinery.
   See synth.c and optimize.h for that, also some special C and assembler files. */

/* The signed-16bit-producing variants. */

int synth_1to1            (real*, int, mpg123_handle*, int);
int synth_1to1_dither     (real*, int, mpg123_handle*, int);
int synth_1to1_i386       (real*, int, mpg123_handle*, int);
int synth_1to1_i586       (real*, int, mpg123_handle*, int);
int synth_1to1_i586_dither(real*, int, mpg123_handle*, int);
int synth_1to1_mmx        (real*, int, mpg123_handle*, int);
int synth_1to1_3dnow      (real*, int, mpg123_handle*, int);
int synth_1to1_sse        (real*, int, mpg123_handle*, int);
int synth_1to1_3dnowext   (real*, int, mpg123_handle*, int);
int synth_1to1_altivec    (real*, int, mpg123_handle*, int);
/* This is different, special usage in layer3.c only.
   Hence, the name... and now forget about it.
   Never use it outside that special portion of code inside layer3.c! */
int absynth_1to1_i486(real*, int, mpg123_handle*, int);
/* These mono/stereo converters use one of the above for the grunt work. */
int synth_1to1_mono       (real*, mpg123_handle*);
int synth_1to1_mono2stereo(real*, mpg123_handle*);

/* Sample rate decimation comes in less flavours. */
int synth_2to1            (real*, int, mpg123_handle*, int);
int synth_2to1_dither     (real*, int, mpg123_handle*, int);
int synth_2to1_i386       (real*, int, mpg123_handle*, int);
int synth_2to1_mono       (real*, mpg123_handle*);
int synth_2to1_mono2stereo(real*, mpg123_handle*);
int synth_4to1            (real *,int, mpg123_handle*, int);
int synth_4to1_dither     (real *,int, mpg123_handle*, int);
int synth_4to1_i386       (real*, int, mpg123_handle*, int);
int synth_4to1_mono       (real*, mpg123_handle*);
int synth_4to1_mono2stereo(real*, mpg123_handle*);
/* NtoM is really just one implementation. */
int synth_ntom (real *,int, mpg123_handle*, int);
int synth_ntom_mono (real *, mpg123_handle *);
int synth_ntom_mono2stereo (real *, mpg123_handle *);

/* The 8bit-producing variants. */

/* There are direct 8-bit synths and wrappers over a possibly optimized 16bit one. */
int synth_1to1_8bit            (real*, int, mpg123_handle*, int);
int synth_1to1_8bit_i386       (real*, int, mpg123_handle*, int);
int synth_1to1_8bit_wrap       (real*, int, mpg123_handle*, int);
int synth_1to1_8bit_mono       (real*, mpg123_handle*);
int synth_1to1_8bit_mono2stereo(real*, mpg123_handle*);
int synth_1to1_8bit_wrap_mono       (real*, mpg123_handle*);
int synth_1to1_8bit_wrap_mono2stereo(real*, mpg123_handle*);
int synth_2to1_8bit            (real*, int, mpg123_handle*, int);
int synth_2to1_8bit_i386       (real*, int, mpg123_handle*, int);
int synth_2to1_8bit_mono       (real*, mpg123_handle*);
int synth_2to1_8bit_mono2stereo(real*, mpg123_handle*);
int synth_4to1_8bit            (real*, int, mpg123_handle*, int);
int synth_4to1_8bit_i386       (real*, int, mpg123_handle*, int);
int synth_4to1_8bit_mono       (real*, mpg123_handle*);
int synth_4to1_8bit_mono2stereo(real*, mpg123_handle*);
int synth_ntom_8bit            (real*, int, mpg123_handle*, int);
int synth_ntom_8bit_mono       (real*, mpg123_handle*);
int synth_ntom_8bit_mono2stereo(real*, mpg123_handle*);


#ifndef REAL_IS_FIXED
/* The real-producing variants. */

int synth_1to1_real            (real*, int, mpg123_handle*, int);
int synth_1to1_real_i386       (real*, int, mpg123_handle*, int);
int synth_1to1_real_mono       (real*, mpg123_handle*);
int synth_1to1_real_mono2stereo(real*, mpg123_handle*);
int synth_2to1_real            (real*, int, mpg123_handle*, int);
int synth_2to1_real_i386       (real*, int, mpg123_handle*, int);
int synth_2to1_real_mono       (real*, mpg123_handle*);
int synth_2to1_real_mono2stereo(real*, mpg123_handle*);
int synth_4to1_real            (real*, int, mpg123_handle*, int);
int synth_4to1_real_i386       (real*, int, mpg123_handle*, int);
int synth_4to1_real_mono       (real*, mpg123_handle*);
int synth_4to1_real_mono2stereo(real*, mpg123_handle*);
int synth_ntom_real            (real*, int, mpg123_handle*, int);
int synth_ntom_real_mono       (real*, mpg123_handle*);
int synth_ntom_real_mono2stereo(real*, mpg123_handle*);

/* 32bit integer */
int synth_1to1_s32            (real*, int, mpg123_handle*, int);
int synth_1to1_s32_i386       (real*, int, mpg123_handle*, int);
int synth_1to1_s32_mono       (real*, mpg123_handle*);
int synth_1to1_s32_mono2stereo(real*, mpg123_handle*);
int synth_2to1_s32            (real*, int, mpg123_handle*, int);
int synth_2to1_s32_i386       (real*, int, mpg123_handle*, int);
int synth_2to1_s32_mono       (real*, mpg123_handle*);
int synth_2to1_s32_mono2stereo(real*, mpg123_handle*);
int synth_4to1_s32            (real*, int, mpg123_handle*, int);
int synth_4to1_s32_i386       (real*, int, mpg123_handle*, int);
int synth_4to1_s32_mono       (real*, mpg123_handle*);
int synth_4to1_s32_mono2stereo(real*, mpg123_handle*);
int synth_ntom_s32            (real*, int, mpg123_handle*, int);
int synth_ntom_s32_mono       (real*, mpg123_handle*);
int synth_ntom_s32_mono2stereo(real*, mpg123_handle*);
#endif


/* Inside these synth functions, some dct64 variants may be used.
   The special optimized ones that only appear in assembler code are not mentioned here.
   And, generally, these functions are only employed in a matching synth function. */
void dct64        (real *,real *,real *);
void dct64_i386   (real *,real *,real *);
void dct64_altivec(real *,real *,real *);
void dct64_i486(int*, int* , real*); /* Yeah, of no use outside of synth_i486.c .*/

/* This is used by the layer 3 decoder, one generic function and 3DNow variants. */
void dct36         (real *,real *,real *,real *,real *);
void dct36_3dnow   (real *,real *,real *,real *,real *);
void dct36_3dnowext(real *,real *,real *,real *,real *);

/* Tools for NtoM resampling synth, defined in ntom.c . */
int synth_ntom_set_step(mpg123_handle *fr); /* prepare ntom decoding */
unsigned long ntom_val(mpg123_handle *fr, off_t frame); /* compute ntom_val for frame offset */
/* Frame and sample offsets. */
off_t ntom_frmouts(mpg123_handle *fr, off_t frame);
off_t ntom_ins2outs(mpg123_handle *fr, off_t ins);
off_t ntom_frameoff(mpg123_handle *fr, off_t soff);

/* Initialization of any static data that may be needed at runtime.
   Make sure you call these once before it is too late. */
void init_layer3(void);
void init_layer2(void);
void prepare_decode_tables(void);

extern real *pnts[5]; /* tabinit provides, dct64 needs */

/* Runtime (re)init functions; needed more often. */
void make_decode_tables(mpg123_handle *fr); /* For every volume change. */
/* Stuff needed after updating synth setup (see set_synth_functions()). */
real* init_layer2_table(mpg123_handle *fr, real *table, double m);
real init_layer3_gainpow2(mpg123_handle *fr, int i);
#ifdef OPT_MMXORSSE
/* Special treatment for mmx-like decoders, these functions go into the slots below. */
void make_decode_tables_mmx(mpg123_handle *fr);
real init_layer3_gainpow2_mmx(mpg123_handle *fr, int i);
real* init_layer2_table_mmx(mpg123_handle *fr, real *table, double m);
#endif
void init_layer2_stuff(mpg123_handle *fr, real* (*init_table)(mpg123_handle *fr, real *table, double m));
void init_layer3_stuff(mpg123_handle *fr, real (*gainpow2)(mpg123_handle *fr, int i));
/* Needed when switching to 8bit output. */
int make_conv16to8_table(mpg123_handle *fr);

/* These are the actual workers.
   They operate on the parsed frame data and handle decompression to audio samples.
   The synth functions defined above are called from inside the layer handlers. */

int do_layer3(mpg123_handle *fr);
int do_layer2(mpg123_handle *fr);
int do_layer1(mpg123_handle *fr);
/* There's an 3DNow counterpart in asm. */
void do_equalizer(real *bandPtr,int channel, real equalizer[2][32]);

#endif
