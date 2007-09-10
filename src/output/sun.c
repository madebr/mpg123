/*
	sun: audio output for Sun systems

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "config.h"
#include "mpg123.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SUN_AUDIOIO_H
#include <sun/audioio.h>
#endif

#ifdef HAVE_SYS_AUDIOIO_H
#include <sys/audioio.h>
#endif

#ifdef HAVE_SYS_AUDIO_H
#include <sys/audio.h>
#endif

#ifdef HAVE_ASM_AUDIOIO_H
#include <asm/audioio.h>
#endif





static void audio_set_format_helper(audio_output_t *ao, audio_info_t *ainfo)
{

  switch(ao->format) {
    case -1:
    case AUDIO_FORMAT_SIGNED_16:
    default:
#ifndef AUDIO_ENCODING_LINEAR	/* not supported */
#define AUDIO_ENCODING_LINEAR 3
#endif
      ainfo->play.encoding = AUDIO_ENCODING_LINEAR;
      ainfo->play.precision = 16;
      break;
    case AUDIO_FORMAT_UNSIGNED_8:
#if defined(SOLARIS) || defined(SPARCLINUX)
      ainfo->play.encoding = AUDIO_ENCODING_LINEAR8;
      ainfo->play.precision = 8;
      break;
#endif
    case AUDIO_FORMAT_SIGNED_8:
      fprintf(stderr,"Linear signed 8 bit not supported!\n");
      return;
    case AUDIO_FORMAT_ULAW_8:
      ainfo->play.encoding = AUDIO_ENCODING_ULAW;
      ainfo->play.precision = 8;
      break;
    case AUDIO_FORMAT_ALAW_8:
      ainfo->play.encoding = AUDIO_ENCODING_ALAW;
      ainfo->play.precision = 8;
      break;
  }
  
}


static int audio_reset_parameters(audio_output_t *ao)
{
  audio_info_t ainfo;

  AUDIO_INITINFO(&ainfo);

  if(ao->rate != -1)
    ainfo.play.sample_rate = ao->rate;
  if(ao->channels >= 0)
    ainfo.play.channels = ao->channels;
  audio_set_format_helper(ai,&ainfo);

  if(ioctl(ao->fn, AUDIO_SETINFO, &ainfo) == -1)
    return -1;
  return 0;
}

static int audio_rate_best_match(audio_output_t *ao)
{
  audio_info_t ainfo;
  AUDIO_INITINFO(&ainfo);
 
  ainfo.play.sample_rate = ao->rate;
  if(ioctl(ao->fn, AUDIO_SETINFO, &ainfo) < 0) {
    ao->rate = 0;
    return 0;
  }
  if(ioctl(ao->fn, AUDIO_GETINFO, &ainfo) < 0) {
    return -1;
  }
  ao->rate = ainfo.play.sample_rate;
  return 0;
}

static int audio_set_rate(audio_output_t *ao)
{
  audio_info_t ainfo;

  if(ao->rate != -1) {
    AUDIO_INITINFO(&ainfo);
    ainfo.play.sample_rate = ao->rate;
    if(ioctl(ao->fn, AUDIO_SETINFO, &ainfo) == -1)
      return -1;
    return 0;
  }
  return -1;
}

static int audio_set_channels(audio_output_t *ao)
{
  audio_info_t ainfo;

  AUDIO_INITINFO(&ainfo);
  ainfo.play.channels = ao->channels;
  if(ioctl(ao->fn, AUDIO_SETINFO, &ainfo) == -1)
    return -1;
  return 0;
}

static int audio_set_format(audio_output_t *ao)
{
  audio_info_t ainfo;

  AUDIO_INITINFO(&ainfo);
  audio_set_format_helper(ai,&ainfo);
  if(ioctl(ao->fn, AUDIO_SETINFO, &ainfo) == -1)
    return -1;

  return 0;
}

int audio_open(audio_output_t *ao)
{
  audio_info_t ainfo;

  if(!ao->device) {
    if(getenv("AUDIODEV")) {
      if(param.verbose > 1) 
         fprintf(stderr,"Using audio-device value from AUDIODEV environment variable!\n");
      ao->device = getenv("AUDIODEV");
    }
    else 
      ao->device = "/dev/audio";
  }

  ao->fn = open(ao->device,O_WRONLY);
  if(ao->fn < 0)
     return ao->fn;

#if defined(SUNOS)  &&  defined(AUDIO_GETDEV)
  {
    int type;
    if(ioctl(ao->fn, AUDIO_GETDEV, &type) == -1)
      return -1;
    if(type == AUDIO_DEV_UNKNOWN || type == AUDIO_DEV_AMD)
      return -1;
  }
#else
#if defined(SOLARIS) || defined(SPARCLINUX)
  {
    struct audio_device ad;
    if(ioctl(ao->fn, AUDIO_GETDEV, &ad) == -1)
      return -1;
    if(param.verbose > 1)
      fprintf(stderr,"Audio device type: %s\n",ad.name);
    if(!strstr(ad.name,"dbri") && !strstr(ad.name,"CS4231") && param.verbose)
      fprintf(stderr,"Warning: Unknown sound system %s. But we try it.\n",ad.name);
  }
#endif
#endif

  if(audio_reset_parameters(ao) < 0) {
    return -1;
  }

  AUDIO_INITINFO(&ainfo);

  if(ao->output > 0)
    ainfo.play.port = 0;
  if(ao->output & AUDIO_OUT_INTERNAL_SPEAKER)
    ainfo.play.port |= AUDIO_SPEAKER;
  if(ao->output & AUDIO_OUT_HEADPHONES)
    ainfo.play.port |= AUDIO_HEADPHONE;
#ifdef AUDIO_LINE_OUT
  if(ao->output & AUDIO_OUT_LINE_OUT)
    ainfo.play.port |= AUDIO_LINE_OUT;
#endif

  if(ao->gain != -1)
    ainfo.play.gain = ao->gain;

  if(ioctl(ao->fn, AUDIO_SETINFO, &ainfo) == -1)
    return -1;

  return ao->fn;
}




int audio_get_formats(audio_output_t *ao)
{
  static int tab[][3] = {
    { AUDIO_ENCODING_ULAW , 8,  AUDIO_FORMAT_ULAW_8 } ,
    { AUDIO_ENCODING_ALAW , 8,  AUDIO_FORMAT_ALAW_8 } ,
    { AUDIO_ENCODING_LINEAR , 16,  AUDIO_FORMAT_SIGNED_16 } ,
#if defined(SOLARIS) || defined(SPARCLINUX)
    { AUDIO_ENCODING_LINEAR8 , 8,  AUDIO_FORMAT_UNSIGNED_8 } ,
#endif
  };

  audio_info_t ainfo;
  int i,fmts=0;

  for(i=0;i<4;i++) {
    AUDIO_INITINFO(&ainfo);
    ainfo.play.encoding = tab[i][0];
    ainfo.play.precision = tab[i][1];
#if 1
    ainfo.play.sample_rate = ao->rate;
    ainfo.play.channels = ao->channels;
#endif
    if(ioctl(ao->fn, AUDIO_SETINFO, &ainfo) >= 0) {
      fmts |= tab[i][2];
    }
  }
  return fmts;
}

int audio_play_samples(audio_output_t *ao,unsigned char *buf,int len)
{
  return write(ao->fn,buf,len);
}

int audio_close(audio_output_t *ao)
{
  close (ao->fn);
  return 0;
}

void audio_queueflush (audio_output_t *ao)
{
	/*ioctl (ao->fn, I_FLUSH, FLUSHRW);*/
}
