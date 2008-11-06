/*
	decode_ntom.c: N->M down/up sampling. Not optimized for speed.

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123lib_intern.h"
#include "debug.h"

int synth_ntom_8bit(real *bandPtr,int channel, mpg123_handle *fr, int final)
{
  short samples_tmp[8*64];
  short *tmp1 = samples_tmp + channel;
  int i,ret;

  int pnt = fr->buffer.fill;
  unsigned char *samples = fr->buffer.data;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_ntom(bandPtr, channel, fr, 1);
  fr->buffer.data = samples;

  samples += channel + pnt;
  for(i=0;i<(fr->buffer.fill>>2);i++) {
    *samples = fr->conv16to8[*tmp1>>AUSHIFT];
    samples += 2;
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + (final ? fr->buffer.fill>>1 : 0);

  return ret;
}

int synth_ntom_8bit_mono(real *bandPtr, mpg123_handle *fr)
{
  short samples_tmp[8*64];
  short *tmp1 = samples_tmp;
  int i,ret;

  int pnt = fr->buffer.fill;
  unsigned char *samples = fr->buffer.data;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_ntom(bandPtr, 0, fr, 1);
  fr->buffer.data = samples;

  samples += pnt;
  for(i=0;i<(fr->buffer.fill>>2);i++) {
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + (fr->buffer.fill>>2);
  
  return ret;
}

int synth_ntom_8bit_mono2stereo(real *bandPtr, mpg123_handle *fr)
{
  short samples_tmp[8*64];
  short *tmp1 = samples_tmp;
  int i,ret;

  int pnt = fr->buffer.fill;
  unsigned char *samples = fr->buffer.data;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_ntom(bandPtr, 0, fr, 1);
  fr->buffer.data = samples;

  samples += pnt;
  for(i=0;i<(fr->buffer.fill>>2);i++) {
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + (fr->buffer.fill>>1);

  return ret;
}

#define SAMPLE_T short
#define SYNTH_NAME       synth_ntom
#define MONO_NAME        synth_ntom_mono
#define MONO2STEREO_NAME synth_ntom_mono2stereo
#define WRITE_SAMPLE(samples,sum,clip) WRITE_SHORT_SAMPLE(samples,sum,clip)
#include "synth_ntom.h"
