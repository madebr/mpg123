/*
	synth_1to1.h: non-downsampling synth functions

	This header is used multiple times to create different variants of this function.
	See decode.c .
	Hint: MONO_NAME, MONO2STEREO_NAME, SYNTH_NAME and SAMPLE_T as well as WRITE_SAMPLE do vary.

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#define BLOCK 0x40
#include "synth.h"
