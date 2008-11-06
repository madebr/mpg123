/*
	decode_float.c: decoding floating point samples

	This file pulls in 1to1, 2to1, 4to1 and ntom synth functions.

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis.
*/

#include "mpg123lib_intern.h"

#ifdef REAL_IS_FIXED
#error "Still, no floating point output with fixed point decoder!"
#endif

/* No 8bit functions here! */

const real float_scale = 1./SHORT_SCALE;

#define SAMPLE_T real
#define WRITE_SAMPLE(samples,sum,clip) *(samples) = float_scale*(sum)

#define SYNTH_NAME       synth_1to1_real
#define MONO_NAME        synth_1to1_mono_real
#define MONO2STEREO_NAME synth_1to1_mono2stereo_real
#include "synth_1to1.h"

#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME
#define SYNTH_NAME       synth_2to1_real
#define MONO_NAME        synth_2to1_mono_real
#define MONO2STEREO_NAME synth_2to1_mono2stereo_real
#include "synth_2to1.h"

#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME
#define SYNTH_NAME       synth_4to1_real
#define MONO_NAME        synth_4to1_mono_real
#define MONO2STEREO_NAME synth_4to1_mono2stereo_real
#include "synth_4to1.h"

#undef SYNTH_NAME
#undef MONO_NAME
#undef MONO2STEREO_NAME
#define SYNTH_NAME       synth_ntom_real
#define MONO_NAME        synth_ntom_mono_real
#define MONO2STEREO_NAME synth_ntom_mono2stereo_real
#include "synth_ntom.h"
