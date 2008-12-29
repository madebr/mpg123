/*
	synth_real.c: The functions for synthesizing real (float) samples, at the end of decoding.

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp, heavily dissected and rearranged by Thomas Orgis
*/

#include "mpg123lib_intern.h"
#include "debug.h"

#ifdef REAL_IS_FIXED
#error "Do not build this file with fixed point math!"
#else
/* 
	Part 3: All synth functions that produce float output.
	What we need is just a special WRITE_SAMPLE. For the generic and i386 functions, that is.
	The optimized synths would need to be changed internally to support float output.
*/

#define SAMPLE_T real
#define WRITE_SAMPLE(samples,sum,clip) WRITE_REAL_SAMPLE(samples,sum,clip)

/* Part 3a: All straight 1to1 decoding functions */
#define BLOCK 0x40 /* One decoding block is 64 samples. */

#define SYNTH_NAME synth_1to1_real
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_1to1_real (could be generic, could be i386). */
#define SYNTH_NAME       opt_synth_1to1_real(fr)
#define MONO_NAME        synth_1to1_real_mono
#define MONO2STEREO_NAME synth_1to1_real_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_1to1_real_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

#ifndef NO_DOWNSAMPLE

/*
	Part 3b: 2to1 synth. Only generic and i386.
*/
#define BLOCK 0x20 /* One decoding block is 32 samples. */

#define SYNTH_NAME synth_2to1_real
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_2to1_real (could be generic, could be i386). */
#define SYNTH_NAME       opt_synth_2to1_real(fr)
#define MONO_NAME        synth_2to1_real_mono
#define MONO2STEREO_NAME synth_2to1_real_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_2to1_real_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

/*
	Part 3c: 4to1 synth. Only generic and i386.
*/
#define BLOCK 0x10 /* One decoding block is 16 samples. */

#define SYNTH_NAME synth_4to1_real
#include "synth.h"
#undef SYNTH_NAME

/* Mono-related synths; they wrap over _some_ synth_4to1_real (could be generic, could be i386). */
#define SYNTH_NAME       opt_synth_4to1_real(fr)
#define MONO_NAME        synth_4to1_real_mono
#define MONO2STEREO_NAME synth_4to1_real_mono2stereo
#include "synth_mono.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#ifdef OPT_X86
#define NO_AUTOINCREMENT
#define SYNTH_NAME synth_4to1_real_i386
#include "synth.h"
#undef SYNTH_NAME
/* i386 uses the normal mono functions. */
#undef NO_AUTOINCREMENT
#endif

#undef BLOCK

#endif /* NO_DOWNSAMPLE */

#ifndef NO_NTOM
/*
	Part 3d: ntom synth.
	Same procedure as above... Just no extra play anymore, straight synth that may use an optimized dct64.
*/

/* These are all in one header, there's no flexibility to gain. */
#define SYNTH_NAME       synth_ntom_real
#define MONO_NAME        synth_ntom_real_mono
#define MONO2STEREO_NAME synth_ntom_real_mono2stereo
#include "synth_ntom.h"
#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME

#endif

#undef SAMPLE_T
#undef WRITE_SAMPLE

#endif /* non-fixed type */
