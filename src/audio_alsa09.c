/*
	INCOMPLETE!!!
	audio_alsa09: sound output via Advanced Linux Audio Architecture, 0.9x API

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Thomas Orgis <thomas@orgis.org>
*/

#include "config.h"
#include "mpg123.h"
#include <alsa/asoundlib.h>

/* only one instance... */
/* there is place in audio_info_struct at least for handle, but since I need special handling of hwparams anyway... */
snd_pcm_hw_params_t *hwparams = NULL;
snd_pcm_t *pcm_handle = NULL;
snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

int audio_open(struct audio_info_struct *)
{
	if(hwparams == NULL) snd_pcm_hw_params_malloc(&hwparams);
	if(hwparams != NULL)
	{
		
		return 0;
	}
	else return -1
}

int audio_get_formats(struct audio_info_struct *)
{
	if(hwparams == NULL) snd_pcm_hw_params_malloc(&hwparams);
	if(hwparams != NULL)
	{
		
		return 0;
	}
	else return -1
}


int audio_play_samples(struct audio_info_struct *, unsigned char* buf, int bytes)
{
	snd_pcm_write(pcm_handle, buf, bytes);
}

void audio_queueflush(struct audio_info_struct *ai)
{
	/* I _guess_ this is right here. */
	snd_pcm_pause(pcm_handle,1);
}

int audio_close(struct audio_info_struct *)
{
	/* close assumes that open was successful! */
	snd_pcm_drain(pcm_handle); /* hm, does that ensure or prevent playback of all samples? */
	snd_pcm_close(pcm_handle);
	snd_pcm_hw_params_free(hwparams);
}
