/*
	audio_aix.c: Driver for IBM RS/6000 with AIX Ultimedia Services

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Juergen Schoew and Tomas Oegren
*/

#include <errno.h>
#include <fcntl.h>
#include <sys/audio.h>
#include <stropts.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "mpg123.h"

/* use AUDIO_BSIZE to set the msec for audio buffering in Ultimedia library
 */
/* #define AUDIO_BSIZE AUDIO_IGNORE */
#define AUDIO_BSIZE 200

int audio_rate_best_match(struct audio_info_struct *ai)
{
  static long valid [ ] = {  5510,  6620,  8000,  9600, 11025, 16000, 18900,
                            22050, 27420, 32000, 33075, 37800, 44100, 48000, 0 };
  int  i = 0;
  long best = 8000;

  if(!ai || ai->fn < 0 || ai->rate < 0) {
    return -1;
  } 

  while (valid [i])
  {
    if (abs(valid[i] - ai->rate) < abs(best - ai->rate))
    {
      best = valid [i];
    }
    i = i + 1;
  }

  ai->rate = best;
  return best;
}


int audio_open(struct audio_info_struct *ai)
{
  audio_init ainit;
  int ret;

  if(!ai->device) {
    if(getenv("AUDIODEV")) {
      if(param.verbose > 1) 
         fprintf(stderr,"Using audio-device value from AUDIODEV environmentvariable!\n");
      ai->device = getenv("AUDIODEV");
      ai->fn = open(ai->device,O_WRONLY);
    }
    else {
      ai->device = "/dev/paud0/1";                   /* paud0 for PCI */
      ai->fn = open(ai->device,O_WRONLY);
      if ((ai->fn == -1) & (errno == ENOENT)) {
        ai->device = "/dev/baud0/1";                 /* baud0 for MCA */
        ai->fn = open(ai->device,O_WRONLY);
      }   
    }
  } else ai->fn = open(ai->device,O_WRONLY);

  if(ai->fn < 0){
     fprintf(stderr,"Can't open audio device!\n");
     return ai->fn;
  }

  /* Init to default values */
  memset ( & ainit, '\0', sizeof (ainit));
  ainit.srate            = 44100;
  ainit.channels         = 2;
  ainit.mode             = PCM;
  ainit.bits_per_sample  = 16;
  ainit.flags            = BIG_ENDIAN | TWOS_COMPLEMENT;
  ainit.operation        = PLAY;
  ainit.bsize            = AUDIO_BSIZE;
 
  ret = ioctl (ai->fn, AUDIO_INIT, & ainit);
  if (ret < 0)
     return ret;
  audio_reset_parameters(ai);
  return ai->fn;
}

int audio_reset_parameters(struct audio_info_struct *ai)
{
  audio_control  acontrol;
  audio_change   achange;
  audio_init     ainit;
  int ret;

  memset ( & achange, '\0', sizeof (achange));
  memset ( & acontrol, '\0', sizeof (acontrol));
  
  achange.balance        = 0x3fff0000;
  achange.balance_delay  = 0;
  achange.volume         = (long) (0x7fff << 16);
  achange.volume_delay   = 0;
  achange.input          = AUDIO_IGNORE;
  if (ai->output == -1) achange.output = INTERNAL_SPEAKER;
  else
  achange.output      = 0;
  if(ai->output & AUDIO_OUT_INTERNAL_SPEAKER)
     achange.output     |= INTERNAL_SPEAKER;
  if(ai->output & AUDIO_OUT_HEADPHONES)
     achange.output     |= EXTERNAL_SPEAKER;
  if(ai->output & AUDIO_OUT_LINE_OUT)
     achange.output     |= OUTPUT_1;
  if(ai->output == 0)
     achange.output      = AUDIO_IGNORE;
  achange.treble         = AUDIO_IGNORE;
  achange.bass          = AUDIO_IGNORE;
  achange.pitch          = AUDIO_IGNORE;
  achange.monitor        = AUDIO_IGNORE;
  achange.dev_info       = (char *) NULL;

  acontrol.ioctl_request = AUDIO_CHANGE;
  acontrol.position      = 0;
  acontrol.request_info  = (char *) & achange;

  ret = ioctl (ai->fn, AUDIO_CONTROL, & acontrol);
  if (ret < 0)
    return ret;

  /* Init Device for new values */
  if (ai->rate >0) {
    memset ( & ainit, '\0', sizeof (ainit));
    ainit.srate                 = audio_rate_best_match(ai);
    if (ai->channels > 0)
       ainit.channels          = ai->channels;
    else
      ainit.channels           = 1;
    switch (ai->format) {
      default :
        ainit.mode             = PCM;
        ainit.bits_per_sample  = 8;
        ainit.flags            = BIG_ENDIAN | TWOS_COMPLEMENT;
        break;
      case AUDIO_FORMAT_SIGNED_16:
        ainit.mode             = PCM;
        ainit.bits_per_sample  = 16;
        ainit.flags            = BIG_ENDIAN | TWOS_COMPLEMENT;
        break;
      case AUDIO_FORMAT_SIGNED_8:
        ainit.mode             = PCM;
        ainit.bits_per_sample  = 8;
        ainit.flags            = BIG_ENDIAN | TWOS_COMPLEMENT;
        break;
      case AUDIO_FORMAT_UNSIGNED_16:
        ainit.mode             = PCM;
        ainit.bits_per_sample  = 16;
        ainit.flags            = BIG_ENDIAN | TWOS_COMPLEMENT | SIGNED;
        break;
      case AUDIO_FORMAT_UNSIGNED_8:
        ainit.mode             = PCM;
        ainit.bits_per_sample  = 8;
        ainit.flags            = BIG_ENDIAN | TWOS_COMPLEMENT | SIGNED;
        break;
      case AUDIO_FORMAT_ULAW_8:
        ainit.mode             = MU_LAW;
        ainit.bits_per_sample  = 8;
        ainit.flags            = BIG_ENDIAN | TWOS_COMPLEMENT;
        break;
      case AUDIO_FORMAT_ALAW_8:
        ainit.mode             = A_LAW;
        ainit.bits_per_sample  = 8;
        ainit.flags            = BIG_ENDIAN | TWOS_COMPLEMENT;
        break;
    }
    ainit.operation            = PLAY;
    ainit.bsize                = AUDIO_BSIZE;
 
    ret = ioctl (ai->fn, AUDIO_INIT, & ainit);
    if (ret < 0) {
      fprintf(stderr,"Can't set new audio parameters!\n");
      return ret;
    }
  }

  acontrol.ioctl_request   = AUDIO_START;
  acontrol.request_info    = NULL;
  acontrol.position        = 0;

  ret = ioctl (ai->fn, AUDIO_CONTROL, & acontrol);
  if (ret < 0) {
    fprintf(stderr,"Can't reset audio!\n");
    return ret;
  }
  return 0;
}

int audio_get_formats(struct audio_info_struct *ai)
{
/* ULTIMEDIA DOCUMENTATION SAYS:
   The Ultimedia Audio Adapter supports fourteen sample rates you can use to
   capture and playback audio data. The rates are (in kHz): 5.51, 6.62, 8.0,
   9.6, 11.025, 16.0, 18.9, 22.050, 27.42, 32.0, 33.075, 37.8, 44.1, and 48.0.
   These rates are supported for mono and stereo PCM (8- and 16-bit), mu-law,
   and A-law. 
*/

  long rate;
  
  rate = ai->rate;
  audio_rate_best_match(ai);
  if (ai->rate == rate)
     return (AUDIO_FORMAT_SIGNED_16|AUDIO_FORMAT_UNSIGNED_16|
             AUDIO_FORMAT_UNSIGNED_8|AUDIO_FORMAT_SIGNED_8|
             AUDIO_FORMAT_ULAW_8|AUDIO_FORMAT_ALAW_8);
  else
    return 0;
}

int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
    return write(ai->fn,buf,len);
}

int audio_close(struct audio_info_struct *ai)
{
    audio_control acontrol;
    audio_buffer  abuffer;
    int           ret,i;

    /* Don't close the audio-device until it's played all its contents */
    memset ( & acontrol, '\0', sizeof ( acontrol ) );
    acontrol.request_info = &abuffer;
    acontrol.position = 0;
    i=50;   /* Don't do this forever on a bad day :-) */
    while (i-- > 0) {
            if ((ioctl(ai->fn, AUDIO_BUFFER, &acontrol))< 0) {
                    fprintf(stderr, "buffer read failed: %d\n", errno);
                    break;
            } else {
                    if (abuffer.flags <= 0)
                            break;
            }
            usleep(200000); /* sleep 0.2 sec */
    }

    memset ( & acontrol, '\0', sizeof ( acontrol ) );
    acontrol.ioctl_request = AUDIO_STOP;
    acontrol.request_info  = NULL;
    acontrol.position      = 0;

    ret = ioctl ( ai->fn, AUDIO_CONTROL, & acontrol );
    if (ret < 0)
       fprintf(stderr,"Can't close audio!\n");

    ret = close (ai->fn);
    if (ret < 0)
      fprintf(stderr,"Can't close audio!\n");

    return 0;
}

void audio_queueflush(struct audio_info_struct *ai)
{
}

