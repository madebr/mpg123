/*
	audio_alsa: sound output via Advanced Linux Audio Architecture, 0.9x API

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	
	initially written by Nicholas J. Humfrey <njh@ecs.soton.ac.uk>
*/

#include "config.h"
#include "mpg123.h"

#include <alsa/asoundlib.h>


/* only one instance... */
/* there is space in audio_info_struct at least for handle.. */
static snd_pcm_t *pcm_handle = NULL;
static unsigned int buffer_time = 500000;               /* ring buffer length in us */
static unsigned int period_time = 100000;               /* period time in us */


static int mpg123format2alsa( int format ) {

	switch( format ) {
		case AUDIO_FORMAT_SIGNED_16: return SND_PCM_FORMAT_S16;
		case AUDIO_FORMAT_UNSIGNED_16: return SND_PCM_FORMAT_U16;
		case AUDIO_FORMAT_UNSIGNED_8: return SND_PCM_FORMAT_U8;
		case AUDIO_FORMAT_SIGNED_8: return SND_PCM_FORMAT_S8;
		case AUDIO_FORMAT_ULAW_8: return SND_PCM_FORMAT_MU_LAW;
		case AUDIO_FORMAT_ALAW_8: return SND_PCM_FORMAT_A_LAW;
	}
	
	fprintf(stderr, "Error: unknown format: %x\n", format);
	return 0;
}


int audio_open(struct audio_info_struct *ai)
{
	snd_pcm_hw_params_t *hwparams = NULL;
	int err, dir;
	
	fprintf(stderr, "fn=%d\n", ai->fn);
	fprintf(stderr, "handle=%d\n", (int)ai->handle);
	fprintf(stderr, "rate=%ld\n", ai->rate);
	fprintf(stderr, "gain=%ld\n", ai->gain);
	fprintf(stderr, "output=%d\n", ai->output);
	fprintf(stderr, "device=%s\n", ai->device);
	fprintf(stderr, "channels=%d\n", ai->channels);
	fprintf(stderr, "format=0x%x\n", ai->format);
	
	
	/* already open? */
	if (pcm_handle!=NULL) {
		fprintf(stderr, "audio_open(): ALSA device already open");
		return(-1);
	}
	
	/* Allocate the snd_pcm_hw_params_t structure on the stack. */
	snd_pcm_hw_params_malloc(&hwparams);
	if(hwparams == NULL) {
		fprintf(stderr, "audio_open(): snd_pcm_hw_params_malloc() failed");
		return(-1);
	}
	
	/* Use default device ? */
	if(!ai->device) {
		ai->device = "plughw:0,0";
	}
	
	/* Open the PCM device */
	if (snd_pcm_open(&pcm_handle, ai->device, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		fprintf(stderr, "audio_open(): Error opening PCM device %s\n", ai->device);
		return(-1);
	}
	
	/* Initially configure with any supported configuration */
    if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
		fprintf(stderr, "audio_open(): Can not configure PCM device: %s\n", ai->device);
		return(-1);
    }
    
    /* Use interleaved audio */
    if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		fprintf(stderr, "audio_open(): Error setting access to Interleaved.\n");
		return(-1);
    }

    /* Set sample format */
    if (ai->format != 0xffffffff) {
    	int alsa_format = mpg123format2alsa( ai->format );
    	if (alsa_format) {
			if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
				fprintf(stderr, "audio_open(): Error setting format.\n");
				return(-1);
			}
		}
	}


    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */
    if (ai->rate >= 0) {
		unsigned int exact_rate = ai->rate;
		if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &exact_rate, 0) < 0) {
			fprintf(stderr, "audio_open(): Error setting sample rate.\n");
			return(-1);
		}
		if (ai->rate != exact_rate) {
			fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n"
							" ==> Using %d Hz instead.\n", (int)ai->rate, exact_rate);
		}
	}

    /* Set number of channels */
    if (ai->channels >= 0) {
		if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, ai->channels) < 0) {
		  fprintf(stderr, "audio_open(): Error setting number channels.\n");
		  return(-1);
		}
	}
	
	/* Apply HW parameter settings to PCM device and prepare device  */
	if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
		fprintf(stderr, "Error setting HW params.\n");
		return(-1);
	}
	
	/* set buffer time */
	err = snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams, &buffer_time, &dir);
	if (err < 0) {
		printf("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
		return err;
	}
	
	/* set period time */
	err = snd_pcm_hw_params_set_period_time_near(pcm_handle, hwparams, &period_time, &dir);
	if (err < 0) {
		printf("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
		return err;
	}
	
	/* Free the parameters structure */
	snd_pcm_hw_params_free( hwparams );

	/* Success */
	return 1;
}


int audio_get_formats(struct audio_info_struct *ai)
{
	snd_pcm_hw_params_t *hwparams = NULL;
	int supported_formats = 0;
	
	
	/* Allocate the snd_pcm_hw_params_t structure on the stack. */
	snd_pcm_hw_params_malloc(&hwparams);
	if(hwparams == NULL) {
		fprintf(stderr, "audio_get_formats(): snd_pcm_hw_params_malloc() failed");
		return(0);
	}

	/* Initially configure with any supported configuration */
    if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
		fprintf(stderr, "audio_get_formats(): Can not configure PCM device: %s\n", ai->device);
		return(-1);
    }

	if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16) == 0) {
		supported_formats |= AUDIO_FORMAT_SIGNED_16;
	}
		
	if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_U16) == 0) {
		supported_formats |= AUDIO_FORMAT_UNSIGNED_16;
	}

	if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_U8) == 0) {
		supported_formats |= AUDIO_FORMAT_UNSIGNED_8;
	}
	
	if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_S8) == 0) {
		supported_formats |= AUDIO_FORMAT_SIGNED_8;
	}
		
	if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_A_LAW) == 0) {
		supported_formats |= AUDIO_FORMAT_ALAW_8;
	}
		
	if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_MU_LAW) == 0) {
		supported_formats |= AUDIO_FORMAT_ULAW_8;
	}

	snd_pcm_hw_params_free( hwparams );

	return supported_formats;
}


int audio_play_samples(struct audio_info_struct *ai, unsigned char* buf, int bytes)
{
	snd_pcm_sframes_t frames_written;
	unsigned int frames;
	int sample_size;
	
	if ((ai->format&AUDIO_FORMAT_MASK)==AUDIO_FORMAT_16) {
		sample_size=2;
	} else {
		sample_size=1;
	}

	frames = (bytes/sample_size)/ai->channels;

    /* Returns the number of frames actually written. */
	frames_written = snd_pcm_writei(pcm_handle, buf, frames);
	if (frames_written < 0) {
		frames_written = snd_pcm_recover(pcm_handle, frames_written, 0);
	}
	if (frames_written < 0) {
		fprintf(stderr,"snd_pcm_writei() failed.\n");
		return -1;
	}                   
	if (frames_written > 0 && frames_written != frames) {
		fprintf(stderr,"Short write (expected %li, wrote %li)\n", (long)frames, frames_written);
		return -1;
	}                   
                        
	return frames_written*sample_size*ai->channels;
}

void audio_queueflush(struct audio_info_struct *ai)
{
	/* Stop a PCM dropping pending frames. */
	snd_pcm_drop(pcm_handle);
}

int audio_close(struct audio_info_struct *ai)
{
	/* Stop PCM device after pending frames have been played */ 
	snd_pcm_drain(pcm_handle);	
   
	/* Close PCM device */
    snd_pcm_close( pcm_handle );
    pcm_handle=NULL;
    
	return 0;
}


