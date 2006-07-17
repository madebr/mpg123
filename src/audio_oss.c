/*
	audio_oss.c: audio output via Open Sound System

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp
*/

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "config.h"
#include "mpg123.h"


#ifdef HAVE_LINUX_SOUNDCARD_H
#include <linux/soundcard.h>
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif

#ifdef HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#endif

#ifndef AFMT_S16_NE
# ifdef OSS_BIG_ENDIAN
#  define AFMT_S16_NE AFMT_S16_BE
# else
#  define AFMT_S16_NE AFMT_S16_LE
# endif
#endif

#ifndef AFMT_U16_NE
# ifdef OSS_BIG_ENDIAN
#  define AFMT_U16_NE AFMT_U16_BE
# else
#  define AFMT_U16_NE AFMT_U16_LE
# endif
#endif

extern int outburst;


static int audio_rate_best_match(struct audio_info_struct *ai)
{
  int ret,dsp_rate;

  if(!ai || ai->fn < 0 || ai->rate < 0)
    return -1;
  dsp_rate = ai->rate;
  ret = ioctl(ai->fn, SNDCTL_DSP_SPEED,&dsp_rate);
  if(ret < 0)
    return ret;
  ai->rate = dsp_rate;
  return 0;
}

static int audio_set_rate(struct audio_info_struct *ai)
{
  int dsp_rate;
  int ret = 0;

  if(ai->rate >= 0) {
    dsp_rate = ai->rate;
    ret = ioctl(ai->fn, SNDCTL_DSP_SPEED,&dsp_rate);
  }
  return ret;
}

static int audio_set_channels(struct audio_info_struct *ai)
{
  int chan = ai->channels - 1;
  int ret;

  if(ai->channels < 0)
    return 0;

  ret = ioctl(ai->fn, SNDCTL_DSP_STEREO, &chan);
  if(chan != (ai->channels-1)) {
    return -1;
  }
  return ret;
}

static int audio_set_format(struct audio_info_struct *ai)
{
  int sample_size,fmts;
  int sf,ret;

  if(ai->format == -1)
    return 0;

  switch(ai->format) {
    case AUDIO_FORMAT_SIGNED_16:
    default:
      fmts = AFMT_S16_NE;
      sample_size = 16;
      break;
    case AUDIO_FORMAT_UNSIGNED_8:
      fmts = AFMT_U8;
      sample_size = 8;
      break;
    case AUDIO_FORMAT_SIGNED_8:
      fmts = AFMT_S8;
      sample_size = 8;
      break;
    case AUDIO_FORMAT_ULAW_8:
      fmts = AFMT_MU_LAW;
      sample_size = 8;
      break;
    case AUDIO_FORMAT_ALAW_8:
      fmts = AFMT_A_LAW;
      sample_size = 8;
      break;
    case AUDIO_FORMAT_UNSIGNED_16:
      fmts = AFMT_U16_NE;
      break;
  }
#if 0
  if(ioctl(ai->fn, SNDCTL_DSP_SAMPLESIZE,&sample_size) < 0)
    return -1;
#endif
  sf = fmts;
  ret = ioctl(ai->fn, SNDCTL_DSP_SETFMT, &fmts);
  if(sf != fmts) {
    return -1;
  }
  return ret;
}


static int audio_reset_parameters(struct audio_info_struct *ai)
{
  int ret;
  ret = ioctl(ai->fn, SNDCTL_DSP_RESET, NULL);
  if(ret < 0)
    fprintf(stderr,"Can't reset audio!\n");
  ret = audio_set_format(ai);
  if (ret == -1)
    goto err;
  ret = audio_set_channels(ai);
  if (ret == -1)
    goto err;
  ret = audio_set_rate(ai);
  if (ret == -1)
    goto err;

  /* Careful here.  As per OSS v1.1, the next ioctl() commits the format
   * set above, so we must issue SNDCTL_DSP_RESET before we're allowed to
   * change it again. [dk]
   */
  if (ioctl(ai->fn, SNDCTL_DSP_GETBLKSIZE, &outburst) == -1 ||
      outburst > MAXOUTBURST)
    outburst = MAXOUTBURST;

err:
  return ret;
}


int audio_open(struct audio_info_struct *ai)
{
  char usingdefdev = 0;

  if(!ai)
    return -1;

  if(!ai->device) {
    ai->device = "/dev/dsp";
    usingdefdev = 1;
  }

  ai->fn = open(ai->device,O_WRONLY);  

  if(ai->fn < 0)
  {
    if(usingdefdev) {
      ai->device = "/dev/sound/dsp";
      ai->fn = open(ai->device,O_WRONLY);
      if(ai->fn < 0) {
	fprintf(stderr,"Can't open default sound device!\n");
	exit(1);
      }
    } else {
      fprintf(stderr,"Can't open %s!\n",ai->device);
      exit(1);
    }
  }

  if(audio_reset_parameters(ai) < 0) {
    close(ai->fn);
    return -1;
  }

  if(ai->gain >= 0) {
    int e,mask;
    e = ioctl(ai->fn , SOUND_MIXER_READ_DEVMASK ,&mask);
    if(e < 0) {
      fprintf(stderr,"audio/gain: Can't get audio device features list.\n");
    }
    else if(mask & SOUND_MASK_PCM) {
      int gain = (ai->gain<<8)|(ai->gain);
      e = ioctl(ai->fn, SOUND_MIXER_WRITE_PCM , &gain);
    }
    else if(!(mask & SOUND_MASK_VOLUME)) {
      fprintf(stderr,"audio/gain: setable Volume/PCM-Level not supported by your audio device: %#04x\n",mask);
    }
    else { 
      int gain = (ai->gain<<8)|(ai->gain);
      e = ioctl(ai->fn, SOUND_MIXER_WRITE_VOLUME , &gain);
    }
  }

  return ai->fn;
}



/*
 * get formats for specific channel/rate parameters
 */
int audio_get_formats(struct audio_info_struct *ai)
{
  int fmt = 0;
  int r = ai->rate;
  int c = ai->channels;
  int i;

  static int fmts[] = { 
	AUDIO_FORMAT_ULAW_8 , AUDIO_FORMAT_SIGNED_16 ,
	AUDIO_FORMAT_UNSIGNED_8 , AUDIO_FORMAT_SIGNED_8 ,
	AUDIO_FORMAT_UNSIGNED_16 , AUDIO_FORMAT_ALAW_8 };
	
  /* Reset is required before we're allowed to set the new formats. [dk] */
  ioctl(ai->fn, SNDCTL_DSP_RESET, NULL);

  for(i=0;i<6;i++) {
	ai->format = fmts[i];
	if(audio_set_format(ai) < 0) {
		continue;
        }
	ai->channels = c;
	if(audio_set_channels(ai) < 0) {
		continue;
	}
	ai->rate = r;
	if(audio_rate_best_match(ai) < 0) {
		continue;
	}
	if( (ai->rate*100 > r*(100-AUDIO_RATE_TOLERANCE)) && (ai->rate*100 < r*(100+AUDIO_RATE_TOLERANCE)) ) {
		fmt |= fmts[i];
	}
  }


#if 0
  if(ioctl(ai->fn,SNDCTL_DSP_GETFMTS,&fmts) < 0) {
fprintf(stderr,"No");
    return -1;
  }

  if(fmts & AFMT_MU_LAW)
    ret |= AUDIO_FORMAT_ULAW_8;
  if(fmts & AFMT_S16_NE)
    ret |= AUDIO_FORMAT_SIGNED_16;
  if(fmts & AFMT_U8)
    ret |= AUDIO_FORMAT_UNSIGNED_8;
  if(fmts & AFMT_S8)
    ret |= AUDIO_FORMAT_SIGNED_8;
  if(fmts & AFMT_U16_NE)
    ret |= AUDIO_FORMAT_UNSIGNED_16;
  if(fmts & AFMT_A_LAW)
    ret |= AUDIO_FORMAT_ALAW_8;
#endif

  return fmt;
}

int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
#ifdef WORDS_BIGENDIAN
#define BYTE0(n) ((unsigned char)(n) & (0xFF))
#define BYTE1(n) BYTE0((unsigned int)(n) >> 8)
#define BYTE2(n) BYTE0((unsigned int)(n) >> 16)
#define BYTE3(n) BYTE0((unsigned int)(n) >> 24)
   {
     /* that smells like int=32bit! */
     register int i;
     int swappedInt;
     int *intPtr;

     intPtr = (int *)buf;

     for (i = 0; i < len / sizeof(int); i++)
       {
         swappedInt = (BYTE0(*intPtr) << 24 |
                       BYTE1(*intPtr) << 16 |
                       BYTE2(*intPtr) <<  8 |
                       BYTE3(*intPtr)         );

         *intPtr = swappedInt;
         intPtr++;
       }
    }
#endif /* WORDS_BIGENDIAN */

  return write(ai->fn,buf,len);
}

int audio_close(struct audio_info_struct *ai)
{
  close (ai->fn);
  return 0;
}

void audio_queueflush(struct audio_info_struct *ai)
{
}
