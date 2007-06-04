/*
	audio_dummy.c: dummy audio output

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123.h"

int audio_open(struct audio_info_struct *ai)
{
  fprintf(stderr,"No audio device support compiled into this binary (use -s).\n");
  return -1;
}

int audio_get_formats(struct audio_info_struct *ai)
{
  return AUDIO_FORMAT_SIGNED_16;
}

int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
  return len;
}

int audio_close(struct audio_info_struct *ai)
{
  return 0;
}

void audio_queueflush(struct audio_info_struct *ai)
{
}
