/*
	synth.c: The functions for synthesizing samples, at the end of decoding.

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp, heavily dissected and rearranged by Thomas Orgis
*/

#include "mpg123lib_intern.h"
#include "debug.h"

/*
	Part 1: All synth functions that produce signed short.
	That is:
		- synth_1to1 with cpu-specific variants (synth_1to1_i386, synth_1to1_i586 ...)
		- synth_1to1_mono and synth_1to1_mono2stereo; which use opt_synth_1to1(fr).
	Nearly every decoder variant has it's own synth_1to1, while the mono conversion is shared.
*/

#define SAMPLE_T short
#define WRITE_SAMPLE(samples,sum,clip) WRITE_SHORT_SAMPLE(samples,sum,clip)

/* Part 1a: All straight 1to1 decoding functions */
#define BLOCK 0x40 /* One decoding block is 64 samples. */

#define SYNTH_NAME synth_1to1
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_1to1. */
#define SYNTH_NAME       opt_synth_1to1(fr)
#define MONO_NAME        synth_1to1_mono
#define MONO2STEREO_NAME synth_1to1_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

/* Now we have possibly some special synth_1to1 ...
   ... they produce signed short; the mono functions defined above work on the special synths, too. */

#ifdef OPT_GENERIC_DITHER
#define SYNTH_NAME synth_1to1_dither
#define USE_DITHER
#include "synth.h"
#undef USE_DITHER
#undef SYNTH_NAME
#endif

#ifdef OPT_X86
/* The i386-specific C code, here as short variant, later 8bit and float. */
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_1to1_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK /* Following functions are so special that they don't need this. */

#ifdef OPT_I586
/* This is defined in assembler. */
int synth_1to1_i586_asm(real *bandPtr, int channel, unsigned char *out, unsigned char *buffs, int *bo, real *decwin);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_i586(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	int ret;
	if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

	ret = synth_1to1_i586_asm(bandPtr, channel, fr->buffer.data+fr->buffer.fill, fr->rawbuffs, &fr->bo, fr->decwin);
	if(final) fr->buffer.fill += 128;
	return ret;
}
#endif

#ifdef OPT_I586_DITHER
/* This is defined in assembler. */
int synth_1to1_i586_asm_dither(real *bandPtr, int channel, unsigned char *out, unsigned char *buffs, int *bo, real *decwin, float *dithernoise);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_i586_dither(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	int ret;
	int bo_dither[2]; /* Temporary workaround? Could expand the asm code. */
	if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

	/* Applying this hack, to change the asm only bit by bit (adding dithernoise pointer). */
	bo_dither[0] = fr->bo;
	bo_dither[1] = fr->ditherindex;
	ret = synth_1to1_i586_asm_dither(bandPtr, channel, fr->buffer.data+fr->buffer.fill, fr->rawbuffs, bo_dither, fr->decwin, fr->dithernoise);
	fr->bo          = bo_dither[0];
	fr->ditherindex = bo_dither[1];

	if(final) fr->buffer.fill += 128;
	return ret;
}
#endif

#ifdef OPT_3DNOW
/* Those are defined in assembler. */
void do_equalizer_3dnow(real *bandPtr,int channel, real equalizer[2][32]);
int synth_1to1_3dnow_asm(real *bandPtr, int channel, unsigned char *out, unsigned char *buffs, int *bo, real *decwin);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_3dnow(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	int ret;

	if(fr->have_eq_settings) do_equalizer_3dnow(bandPtr,channel,fr->equalizer);

	/* this is in asm, can be dither or not */
	/* uh, is this return from pointer correct? */ 
	ret = (int) synth_1to1_3dnow_asm(bandPtr, channel, fr->buffer.data+fr->buffer.fill, fr->rawbuffs, &fr->bo, fr->decwin);
	if(final) fr->buffer.fill += 128;
	return ret;
}
#endif

#ifdef OPT_MMX
/* This is defined in assembler. */
int synth_1to1_MMX(real *bandPtr, int channel, short *out, short *buffs, int *bo, float *decwins);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_mmx(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

	/* in asm */
	synth_1to1_MMX(bandPtr, channel, (short*) (fr->buffer.data+fr->buffer.fill), (short *) fr->rawbuffs, &fr->bo, fr->decwins);
	if(final) fr->buffer.fill += 128;
	return 0;
}
#endif

#ifdef OPT_SSE
/* This is defined in assembler. */
void synth_1to1_sse_asm(real *bandPtr, int channel, short *samples, short *buffs, int *bo, real *decwin);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_sse(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

	synth_1to1_sse_asm(bandPtr, channel, (short*) (fr->buffer.data+fr->buffer.fill), (short *) fr->rawbuffs, &fr->bo, fr->decwins);
	if(final) fr->buffer.fill += 128;
	return 0;
}
#endif

#ifdef OPT_3DNOWEXT
/* This is defined in assembler. */
void synth_1to1_3dnowext_asm(real *bandPtr, int channel, short *samples, short *buffs, int *bo, real *decwin);
/* This is just a hull to use the mpg123 handle. */
int synth_1to1_3dnowext(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
	if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

	synth_1to1_3dnowext_asm(bandPtr, channel, (short*) (fr->buffer.data+fr->buffer.fill), (short *) fr->rawbuffs, &fr->bo, fr->decwins);
	if(final) fr->buffer.fill += 128;
	return 0;
}
#endif

/*
	Part 1b: 2to1 synth.
	Only generic and i386 functions this time.
*/
#define BLOCK 0x20 /* One decoding block is 32 samples. */

#define SYNTH_NAME synth_2to1
#include "synth.h"
#undef SYNTH_NAME

#ifdef OPT_DITHER /* Used for generic_dither and as fallback for i586_dither. */
#define SYNTH_NAME synth_2to1_dither
#define USE_DITHER
#include "synth.h"
#undef USE_DITHER
#undef SYNTH_NAME
#endif

#define SYNTH_NAME       opt_synth_2to1(fr)
#define MONO_NAME        synth_2to1_mono
#define MONO2STEREO_NAME synth_2to1_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_2to1_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

/*
	Part 1c: 4to1 synth.
	Same procedure as above...
*/
#define BLOCK 0x10 /* One decoding block is 16 samples. */

#define SYNTH_NAME synth_4to1
#include "synth.h"
#undef SYNTH_NAME

#ifdef OPT_DITHER
#define SYNTH_NAME synth_4to1_dither
#define USE_DITHER
#include "synth.h"
#undef USE_DITHER
#undef SYNTH_NAME
#endif

#define SYNTH_NAME       opt_synth_4to1(fr) /* This is just for the _i386 one... gotta check if it is really useful... */
#define MONO_NAME        synth_4to1_mono
#define MONO2STEREO_NAME synth_4to1_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_4to1_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

/*
	Part 1d: ntom synth.
	Same procedure as above... Just no extra play anymore, straight synth that uses the plain dct64.
*/

/* These are all in one header, there's no flexibility to gain. */
#define SYNTH_NAME       synth_ntom
#define MONO_NAME        synth_ntom_mono
#define MONO2STEREO_NAME synth_ntom_mono2stereo
#include "synth_ntom.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

/* Done with short output. */
#undef SAMPLE_T
#undef WRITE_SAMPLE
