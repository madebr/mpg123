/*
	decode.c: decoding samples...

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123lib_intern.h"

/* 8bit functions are only for non-float output... */

int synth_1to1_8bit(real *bandPtr,int channel, mpg123_handle *fr, int final)
{
  short samples_tmp[64];
  short *tmp1 = samples_tmp + channel;
  int i,ret;

  /* save buffer stuff, trick samples_tmp into there, decode, restore */
  unsigned char *samples = fr->buffer.data;
  int pnt = fr->buffer.fill;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_1to1(bandPtr, channel, fr, 0);
  fr->buffer.data = samples; /* restore original value */

  samples += channel + pnt;
  for(i=0;i<32;i++) {
    *samples = fr->conv16to8[*tmp1>>AUSHIFT];
    samples += 2;
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + (final ? 64 : 0 );

  return ret;
}

int synth_1to1_8bit_mono(real *bandPtr, mpg123_handle *fr)
{
  short samples_tmp[64];
  short *tmp1 = samples_tmp;
  int i,ret;

  /* save buffer stuff, trick samples_tmp into there, decode, restore */
  unsigned char *samples = fr->buffer.data;
  int pnt = fr->buffer.fill;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_1to1(bandPtr,0, fr, 0);
  fr->buffer.data = samples; /* restore original value */

  samples += pnt;
  for(i=0;i<32;i++) {
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + 32;

  return ret;
}

int synth_1to1_8bit_mono2stereo(real *bandPtr, mpg123_handle *fr)
{
  short samples_tmp[64];
  short *tmp1 = samples_tmp;
  int i,ret;

  /* save buffer stuff, trick samples_tmp into there, decode, restore */
  unsigned char *samples = fr->buffer.data;
  int pnt = fr->buffer.fill;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_1to1(bandPtr, 0, fr, 0);
  fr->buffer.data = samples; /* restore original value */

  samples += pnt;
  for(i=0;i<32;i++) {
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + 64;

  return ret;
}

/* The real meat is outside, for flexible use here and for real (float) output. */
#define SAMPLE_T short
#define SYNTH_NAME       synth_1to1
#define MONO_NAME        synth_1to1_mono
#define MONO2STEREO_NAME synth_1to1_mono2stereo
#define WRITE_SAMPLE(samples,sum,clip) WRITE_SHORT_SAMPLE(samples,sum,clip)
#include "synth_1to1.h"
