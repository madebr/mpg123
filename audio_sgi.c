#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "mpg123.h"


/* Analog output constant */
static const char analog_output_res_name[] = ".AnalogOut";


int audio_open(struct audio_info_struct *ai)
{
  int dev = AL_DEFAULT_OUTPUT;

  ai->config = alNewConfig();

  /* Test for correct completion */
  if (ai->config == 0) {
    fprintf(stderr,"audio_open : %s\n",alGetErrorString(oserror()));
    exit(-1);
  }
  
  /* Set port parameters */
  if(ai->channels == 2)
    alSetChannels(ai->config, AL_STEREO);
  else
    alSetChannels(ai->config, AL_MONO);

  alSetWidth(ai->config, AL_SAMPLE_16);
  alSetSampFmt(ai->config,AL_SAMPFMT_TWOSCOMP);
  alSetQueueSize(ai->config, 131069);

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
      exit(-1);
    }
  }
  
  /* Set the device */
  if (alSetDevice(ai->config,dev) < 0)
    {
      fprintf(stderr,"audio_open : %s\n",alGetErrorString(oserror()));
      exit(-1);
    }
  
  /* Open the audio port */
  ai->port = alOpenPort("mpg123-VSC", "w", ai->config);
  if(ai->port == NULL) {
    fprintf(stderr, "Unable to open audio channel: %s\n",
          alGetErrorString(oserror()));
    exit(-1);
  }
  
  audio_reset_parameters(ai);
    
  return 1;
}

int audio_reset_parameters(struct audio_info_struct *ai)
{
  int ret;
  ret = audio_set_format(ai);
  if(ret >= 0)
    ret = audio_set_channels(ai);
  if(ret >= 0)
    ret = audio_set_rate(ai);

/* todo: Set new parameters here */

  return ret;
}

int audio_rate_best_match(struct audio_info_struct *ai)
{
  return 0;
}

int audio_set_rate(struct audio_info_struct *ai)
{
  int dev = alGetDevice(ai->config);
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

int audio_set_channels(struct audio_info_struct *ai)
{
  int ret;
  
  if(ai->channels == 2)
    ret = alSetChannels(ai->config, AL_STEREO);
  else
    ret = alSetChannels(ai->config, AL_MONO);

  if (ret < 0)
    fprintf(stderr,"audio_set_channels : %s\n",alGetErrorString(oserror()));
  
  return 0;
}

int audio_set_format(struct audio_info_struct *ai)
{
  if (alSetSampFmt(ai->config,AL_SAMPFMT_TWOSCOMP) < 0)
    fprintf(stderr,"audio_set_format : %s\n",alGetErrorString(oserror()));
  
  if (alSetWidth(ai->config,AL_SAMPLE_16) < 0)
    fprintf(stderr,"audio_set_format : %s\n",alGetErrorString(oserror()));
  
  return 0;
}

int audio_get_formats(struct audio_info_struct *ai)
{
  return AUDIO_FORMAT_SIGNED_16;
}


int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
  if(ai->format == AUDIO_FORMAT_SIGNED_8)
    alWriteFrames(ai->port, buf, len>>1);
  else
    alWriteFrames(ai->port, buf, len>>2);

  return len;
}

int audio_close(struct audio_info_struct *ai)
{
  if (ai->port) {
    while(alGetFilled(ai->port) > 0)
      sginap(1);  
    alClosePort(ai->port);
    alFreeConfig(ai->config);
  }
  
  return 0;
}
