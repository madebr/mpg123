/*
	decode_2to1.c: ...with 2to1 downsampling

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mpg123.h"
#include "decode.h"

int synth_2to1_8bit(real *bandPtr,int channel,unsigned char *samples,int *pnt)
{
  sample_t samples_tmp[32];
  sample_t *tmp1 = samples_tmp + channel;
  int i,ret;
  int pnt1 = 0;

  ret = synth_2to1(bandPtr,channel,(unsigned char *) samples_tmp,&pnt1);
  samples += channel + *pnt;

  for(i=0;i<16;i++) {
#ifdef FLOATOUT
    *samples = 0;
#else
    *samples = conv16to8[*tmp1>>AUSHIFT];
#endif
    samples += 2;
    tmp1 += 2;
  }
  *pnt += 32;

  return ret;
}

int synth_2to1_8bit_mono(real *bandPtr,unsigned char *samples,int *pnt)
{
  sample_t samples_tmp[32];
  sample_t *tmp1 = samples_tmp;
  int i,ret;
  int pnt1 = 0;

  ret = synth_2to1(bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
  samples += *pnt;

  for(i=0;i<16;i++) {
#ifdef FLOATOUT
    *samples++ = 0;
#else
    *samples++ = conv16to8[*tmp1>>AUSHIFT];
#endif
    tmp1 += 2;
  }
  *pnt += 16;

  return ret;
}


int synth_2to1_8bit_mono2stereo(real *bandPtr,unsigned char *samples,int *pnt)
{
  sample_t samples_tmp[32];
  sample_t *tmp1 = samples_tmp;
  int i,ret;
  int pnt1 = 0;

  ret = synth_2to1(bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
  samples += *pnt;

  for(i=0;i<16;i++) {
#ifdef FLOATOUT
    *samples++ = 0;
    *samples++ = 0;
#else
    *samples++ = conv16to8[*tmp1>>AUSHIFT];
    *samples++ = conv16to8[*tmp1>>AUSHIFT];
#endif
    tmp1 += 2;
  }
  *pnt += 32;

  return ret;
}

int synth_2to1_mono(real *bandPtr,unsigned char *samples,int *pnt)
{
  sample_t samples_tmp[32];
  sample_t *tmp1 = samples_tmp;
  int i,ret;
  int pnt1=0;

  ret = synth_2to1(bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
  samples += *pnt;

  for(i=0;i<16;i++) {
    *( (sample_t *) samples) = *tmp1;
    samples += sizeof(sample_t);
    tmp1 += 2;
  }
  *pnt += 16*sizeof(sample_t);

  return ret;
}

int synth_2to1_mono2stereo(real *bandPtr,unsigned char *samples,int *pnt)
{
  int i,ret;

  ret = synth_2to1(bandPtr,0,samples,pnt);
  samples = samples + *pnt - 32*sizeof(sample_t);

  for(i=0;i<16;i++) {
    ((sample_t *)samples)[1] = ((sample_t *)samples)[0];
    samples+=2*sizeof(sample_t);
  }
  
  return ret;
}

int synth_2to1(real *bandPtr,int channel,unsigned char *out,int *pnt)
{
  static real buffs[2][2][0x110];
  static const int step = 2;
  static int bo = 1;
  sample_t *samples = (sample_t *) (out + *pnt);

  real *b0,(*buf)[0x110];
  int clip = 0; 
  int bo1;

  if(have_eq_settings)
    do_equalizer(bandPtr,channel);

  if(!channel) {
    bo--;
    bo &= 0xf;
    buf = buffs[0];
  }
  else {
    samples++;
    buf = buffs[1];
  }

  if(bo & 0x1) {
    b0 = buf[0];
    bo1 = bo;
    opt_dct64(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
  }
  else {
    b0 = buf[1];
    bo1 = bo+1;
    opt_dct64(buf[0]+bo,buf[1]+bo+1,bandPtr);
  }

  {
    register int j;
    real *window = opt_decwin + 16 - bo1;

    for (j=8;j;j--,b0+=0x10,window+=0x30)
    {
      real sum;
      sum  = *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;

      WRITE_SAMPLE(samples,sum,clip); samples += step;
#if 0
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#endif
    }

    {
      real sum;
      sum  = window[0x0] * b0[0x0];
      sum += window[0x2] * b0[0x2];
      sum += window[0x4] * b0[0x4];
      sum += window[0x6] * b0[0x6];
      sum += window[0x8] * b0[0x8];
      sum += window[0xA] * b0[0xA];
      sum += window[0xC] * b0[0xC];
      sum += window[0xE] * b0[0xE];
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#if 0
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#endif
      b0-=0x20,window-=0x40;
    }
    window += bo1<<1;

    for (j=7;j;j--,b0-=0x30,window-=0x30)
    {
      real sum;
      sum = -*(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;

      WRITE_SAMPLE(samples,sum,clip); samples += step;
#if 0
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#endif
    }
  }

  *pnt += 32*sizeof(sample_t);

  return clip;
}


