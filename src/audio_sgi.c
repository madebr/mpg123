/*
	audio_sgi.c: audio output on sgi boxen

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written (as it seems) by Thomas Woerner
*/



#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

/* #include <audio.h> */
#include <dmedia/audio.h>

#include "config.h"
#include "mpg123.h"


/* Analog output constant */
static const char analog_output_res_name[] = ".AnalogOut";


static int audio_set_rate(struct audio_info_struct *ai, ALconfig config)
{
  int dev = alGetDevice(config);
  ALpv params[1];
  
  /* Make sure the device is OK */
  if (dev < 0)
    {
      fprintf(stderr,"audio_set_rate : %s\n",alGetErrorString(oserror()));
      return 1;      
    }

  params[0].param = AL_OUTPUT_RATE;
  params[0].value.ll = alDoubleToFixed(ai->rate);
  
  if (alSetParams(dev, params,1) < 0)
    fprintf(stderr,"audio_set_rate : %s\n",alGetErrorString(oserror()));
  
  return 0;
}

static int audio_set_channels(struct audio_info_struct *ai, ALconfig config)
{
  int ret;
  
  if(ai->channels == 2)
    ret = alSetChannels(config, AL_STEREO);
  else
    ret = alSetChannels(config, AL_MONO);

  if (ret < 0)
    fprintf(stderr,"audio_set_channels : %s\n",alGetErrorString(oserror()));
  
  return 0;
}

static int audio_set_format(struct audio_info_struct *ai, ALconfig config)
{
  if (alSetSampFmt(config,AL_SAMPFMT_TWOSCOMP) < 0)
    fprintf(stderr,"audio_set_format : %s\n",alGetErrorString(oserror()));
  
  if (alSetWidth(config,AL_SAMPLE_16) < 0)
    fprintf(stderr,"audio_set_format : %s\n",alGetErrorString(oserror()));
  
  return 0;
}


int audio_open(struct audio_info_struct *ai)
{
  int dev = AL_DEFAULT_OUTPUT;
  ALconfig config = alNewConfig();
  ALport port = NULL;
  
  /* Test for correct completion */
  if (config == 0) {
    fprintf(stderr,"audio_open : %s\n",alGetErrorString(oserror()));
    return -1;
  }
  
  /* Set port parameters */
  if(ai->channels == 2)
    alSetChannels(config, AL_STEREO);
  else
    alSetChannels(config, AL_MONO);

  alSetWidth(config, AL_SAMPLE_16);
  alSetSampFmt(config,AL_SAMPFMT_TWOSCOMP);
  alSetQueueSize(config, 131069);

  /* Setup output device to specified module. If there is no module
     specified in ai structure, use the default four output */
  if ((ai->device) != NULL) {
    
    char *dev_name;
    
    dev_name=malloc((strlen(ai->device) + strlen(analog_output_res_name) + 1) *
                  sizeof(char));
    
    strcpy(dev_name,ai->device);
    strcat(dev_name,analog_output_res_name);
    
    /* Find the asked device resource */
    dev=alGetResourceByName(AL_SYSTEM,dev_name,AL_DEVICE_TYPE);

    /* Free allocated space */
    free(dev_name);

    if (!dev) {
      fprintf(stderr,"Invalid audio resource: %s (%s)\n",dev_name,
            alGetErrorString(oserror()));
      return -1;
    }
  }
  
  /* Set the device */
  if (alSetDevice(config,dev) < 0)
    {
      fprintf(stderr,"audio_open : %s\n",alGetErrorString(oserror()));
      return -1;
    }
  
  /* Open the audio port */
  port = alOpenPort("mpg123-VSC", "w", config);
  if(port == NULL) {
    fprintf(stderr, "Unable to open audio channel: %s\n",
          alGetErrorString(oserror()));
    return -1;
  }
  
  ai->handle = (void*)port;
  
  
  audio_set_format(ai, config);
  audio_set_channels(ai, config);
  audio_set_rate(ai, config);
    

  alFreeConfig(config);
 
  return 1;
}


int audio_get_formats(struct audio_info_struct *ai)
{
  return AUDIO_FORMAT_SIGNED_16;
}


int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
  ALport port = (ALport)ai->handle;

  if(ai->format == AUDIO_FORMAT_SIGNED_8)
    alWriteFrames(port, buf, len>>1);
  else
    alWriteFrames(port, buf, len>>2);

  return len;
}

int audio_close(struct audio_info_struct *ai)
{
  ALport port = (ALport)ai->handle;

  if (port) {
    while(alGetFilled(port) > 0)
      sginap(1);  
    alClosePort(port);
    ai->handle=NULL;
  }
  
  return 0;
}

void audio_queueflush(struct audio_info_struct *ai)
{
}
