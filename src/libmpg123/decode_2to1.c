/*
	decode_2to1.c: ...with 2to1 downsampling

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123lib_intern.h"

int synth_2to1_8bit(real *bandPtr, int channel, mpg123_handle *fr, int final)
{
  short samples_tmp[32];
  short *tmp1 = samples_tmp + channel;
  int i,ret;

  unsigned char *samples = fr->buffer.data;
  int pnt = fr->buffer.fill;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_2to1(bandPtr,channel, fr, 0);
  fr->buffer.data = samples;

  samples += channel + pnt;
  for(i=0;i<16;i++) {
    *samples = fr->conv16to8[*tmp1>>AUSHIFT];
    samples += 2;
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + (final ? 32 : 0);

  return ret;
}

int synth_2to1_8bit_mono(real *bandPtr, mpg123_handle *fr)
{
  short samples_tmp[32];
  short *tmp1 = samples_tmp;
  int i,ret;

  unsigned char *samples = fr->buffer.data;
  int pnt = fr->buffer.fill;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_2to1(bandPtr, 0, fr, 0);
  fr->buffer.data = samples;

  samples += pnt;
  for(i=0;i<16;i++) {
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + 16;

  return ret;
}


int synth_2to1_8bit_mono2stereo(real *bandPtr, mpg123_handle *fr)
{
  short samples_tmp[32];
  short *tmp1 = samples_tmp;
  int i,ret;

  unsigned char *samples = fr->buffer.data;
  int pnt = fr->buffer.fill;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_2to1(bandPtr,0, fr, 0);
  fr->buffer.data = samples;

  samples += pnt;
  for(i=0;i<16;i++) {
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + 32;

  return ret;
}

#define SAMPLE_T short
#define SYNTH_NAME       synth_2to1
#define MONO_NAME        synth_2to1_mono
#define MONO2STEREO_NAME synth_2to1_mono2stereo
#define WRITE_SAMPLE(samples,sum,clip) WRITE_SHORT_SAMPLE(samples,sum,clip)
#include "synth_2to1.h"
