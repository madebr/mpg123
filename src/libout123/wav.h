/*
	wav.c: write wav/au/cdr files (and headerless raw)

	copyright ?-2015 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially extracted of out123_int.h, formerly audio.h, by Thomas Orgis
*/

#ifndef _MPG123_WAV_H_
#define _MPG123_WAV_H_

/* Could get away without any header, as only pointers declared. */
#include "out123.h"

/* Interfaces from wav.c, variants of file writing, to be combined into
   fake modules by the main library code.  */

int au_open(audio_output_t *);
int cdr_open(audio_output_t *);
int raw_open(audio_output_t *);
int wav_open(audio_output_t *);
int wav_write(audio_output_t *, unsigned char *buf, int len);
int wav_close(audio_output_t *);
int au_close(audio_output_t *);
int raw_close(audio_output_t *);
int cdr_formats(audio_output_t *);
int au_formats(audio_output_t *);
int raw_formats(audio_output_t *);
int wav_formats(audio_output_t *);
void wav_drain(audio_output_t *);

#endif

