/*
	synth_4to1.h: 4to1-downsampling synth functions

	This header is used multiple times to create different variants of this function.
	Hint: MONO_NAME, MONO2STEREO_NAME, SYNTH_NAME and SAMPLE_T as well as WRITE_SAMPLE do vary.

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp

	Well, this is very simple downsampling... you may or may not like what you hear.
	But it's cheap.
*/

int MONO_NAME(real *bandPtr, mpg123_handle *fr)
{
  SAMPLE_T samples_tmp[16];
  SAMPLE_T *tmp1 = samples_tmp;
  int i,ret;

  unsigned char *samples = fr->buffer.data;
  int pnt = fr->buffer.fill;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_4to1(bandPtr, 0, fr, 0);
  fr->buffer.data = samples;

  samples += pnt;
  for(i=0;i<8;i++) {
    *( (SAMPLE_T *)samples) = *tmp1;
    samples += sizeof(SAMPLE_T);
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + 8*sizeof(SAMPLE_T);

  return ret;
}

int MONO2STEREO_NAME(real *bandPtr, mpg123_handle *fr)
{
  int i,ret;
	unsigned char *samples = fr->buffer.data;

  ret = synth_4to1(bandPtr, 0, fr, 1);
  samples += fr->buffer.fill - 16*sizeof(SAMPLE_T);

  for(i=0;i<8;i++) {
    ((SAMPLE_T *)samples)[1] = ((SAMPLE_T *)samples)[0];
    samples+=2*sizeof(SAMPLE_T);
  }

  return ret;
}

int SYNTH_NAME(real *bandPtr,int channel, mpg123_handle *fr, int final)
{
  static const int step = 2;
  SAMPLE_T *samples = (SAMPLE_T *) (fr->buffer.data + fr->buffer.fill);

  real *b0, **buf; /* (*buf)[0x110]; */
  int clip = 0; 
  int bo1;

  if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

  if(!channel) {
    fr->bo[0]--;
    fr->bo[0] &= 0xf;
    buf = fr->real_buffs[0];
  }
  else {
    samples++;
    buf = fr->real_buffs[1];
  }

  if(fr->bo[0] & 0x1) {
    b0 = buf[0];
    bo1 = fr->bo[0];
    opt_dct64(fr)(buf[1]+((fr->bo[0]+1)&0xf),buf[0]+fr->bo[0],bandPtr);
  }
  else {
    b0 = buf[1];
    bo1 = fr->bo[0]+1;
    opt_dct64(fr)(buf[0]+fr->bo[0],buf[1]+fr->bo[0]+1,bandPtr);
  }

  {
    register int j;
    real *window = opt_decwin(fr) + 16 - bo1;

    for (j=4;j;j--,b0+=0x30,window+=0x70)
    {
      real sum;
      sum  = REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);
      sum += REAL_MUL(*window++, *b0++);
      sum -= REAL_MUL(*window++, *b0++);

      WRITE_SAMPLE(samples,sum,clip); samples += step;
#if 0
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#endif
    }

    {
      real sum;
      sum  = REAL_MUL(window[0x0], b0[0x0]);
      sum += REAL_MUL(window[0x2], b0[0x2]);
      sum += REAL_MUL(window[0x4], b0[0x4]);
      sum += REAL_MUL(window[0x6], b0[0x6]);
      sum += REAL_MUL(window[0x8], b0[0x8]);
      sum += REAL_MUL(window[0xA], b0[0xA]);
      sum += REAL_MUL(window[0xC], b0[0xC]);
      sum += REAL_MUL(window[0xE], b0[0xE]);
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#if 0
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#endif
      b0-=0x40,window-=0x80;
    }
    window += bo1<<1;

    for (j=3;j;j--,b0-=0x50,window-=0x70)
    {
      real sum;
      sum = REAL_MUL(-*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);
      sum -= REAL_MUL(*(--window), *b0++);

      WRITE_SAMPLE(samples,sum,clip); samples += step;
#if 0
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#endif
    }
  }
  
  if(final) fr->buffer.fill += 16*sizeof(SAMPLE_T);

  return clip;
}


