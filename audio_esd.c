#include <esd.h>
#include <errno.h>
#include <assert.h>

#include "mpg123.h"

#ifdef SOLARIS
#include <stropts.h>
#include <sys/conf.h>
#endif

static unsigned esd_rate = 0, esd_format = 0, esd_channels = 0;
static char *esdserver = NULL;

int audio_open(struct audio_info_struct *ai)
{
  esd_format_t format = ESD_STREAM | ESD_PLAY;

  if (!esd_rate)
  {
    int esd;
    esd_server_info_t *info;
    esd_format_t fmt;

    if ((esd = esd_open_sound(NULL)) >= 0)
    {
      info = esd_get_server_info(esd);
      esd_rate = info->rate;
      fmt = info->format;
      esd_free_server_info(info);
      esd_close(esd);
    }
    else
    {
      esd_rate = esd_audio_rate;
      fmt = esd_audio_format;
    }
    esd_format = AUDIO_FORMAT_UNSIGNED_8;
    if ((fmt & ESD_MASK_BITS) == ESD_BITS16)
      esd_format |= AUDIO_FORMAT_SIGNED_16;
    esd_channels = fmt & ESD_MASK_CHAN;
  }

  if (ai->format == -1)
    ai->format = esd_format;
  else if (!(ai->format & esd_format))
  {
    fprintf(stderr, "audio: Unsupported audio format: %d\n", ai->format);
    errno = EINVAL;
    return -1;
  }
  if (ai->format & AUDIO_FORMAT_SIGNED_16)
    format |= ESD_BITS16;
  else if (ai->format & AUDIO_FORMAT_UNSIGNED_8)
    format |= ESD_BITS8;
  else
    assert(0);

  if (ai->channels == -1)
    ai->channels = 2;
  else if (ai->channels <= 0 || ai->channels > esd_channels)
  {
    fprintf(stderr, "audio: Unsupported no of channels: %d\n", ai->channels);
    errno = EINVAL;
    return -1;
  }
  if (ai->channels == 1)
    format |= ESD_MONO;
  else if (ai->channels == 2)
    format |= ESD_STEREO;
  else
    assert(0);
  
  if (ai->rate == -1)
    ai->rate = esd_rate;
  else if (ai->rate > esd_rate)
    return -1;

  ai->fn = esd_play_stream_fallback(format, ai->rate, esdserver, "mpg123");
  return (ai->fn);
}

int audio_reset_parameters(struct audio_info_struct *ai)
{
  return 0;
}

int audio_rate_best_match(struct audio_info_struct *ai)
{
  return 0;
}

int audio_set_rate(struct audio_info_struct *ai)
{
  audio_close(ai);
  return audio_open(ai);
}

int audio_set_channels(struct audio_info_struct *ai)
{
  audio_close(ai);
  return audio_open(ai);
}

int audio_set_format(struct audio_info_struct *ai)
{
  audio_close(ai);
  return audio_open(ai);
}

int audio_get_formats(struct audio_info_struct *ai)
{
  if (0 < ai->channels && ai->channels <= esd_channels
      && 0 < ai->rate && ai->rate <= esd_rate)
    return esd_format;
  else
    return -1;
}

int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
  return write(ai->fn,buf,len);
}

int audio_close(struct audio_info_struct *ai)
{
  close (ai->fn);
  return 0;
}

#ifdef SOLARIS
void audio_queueflush (struct audio_info_struct *ai)
{
        ioctl (ai->fn, I_FLUSH, FLUSHRW);
}
#endif

#ifdef NETBSD
void audio_queueflush (struct audio_info_struct *ai)
{
        ioctl (ai->fn, AUDIO_FLUSH, 0);
}
#endif

