/*
	audio_alsa: sound output with Advanced Linux Sound Architecture 1.x API

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de

	written by Clemens Ladisch <clemens@ladisch.de>
*/

#include "config.h"
#include "mpg123.h"
#include "debug.h"
#include <errno.h>

/* make ALSA 0.9.x compatible to the 1.0.x API */
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <alsa/asoundlib.h>

static const struct {
	snd_pcm_format_t alsa;
	int mpg123;
} format_map[] = {
	{ SND_PCM_FORMAT_S16,    AUDIO_FORMAT_SIGNED_16   },
	{ SND_PCM_FORMAT_U16,    AUDIO_FORMAT_UNSIGNED_16 },
	{ SND_PCM_FORMAT_U8,     AUDIO_FORMAT_UNSIGNED_8  },
	{ SND_PCM_FORMAT_S8,     AUDIO_FORMAT_SIGNED_8    },
	{ SND_PCM_FORMAT_A_LAW,  AUDIO_FORMAT_ALAW_8      },
	{ SND_PCM_FORMAT_MU_LAW, AUDIO_FORMAT_ULAW_8      },
};
#define NUM_FORMATS (sizeof format_map / sizeof format_map[0])

static int prepared = 0;

static int initialize_device(struct audio_info_struct *ai);

int audio_open(struct audio_info_struct *ai)
{
	const char *pcm_name;
	snd_pcm_t *pcm;

	pcm_name = ai->device ? ai->device : "default";
	if (snd_pcm_open(&pcm, pcm_name, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		fprintf(stderr, "audio_open(): cannot open device %s\n", pcm_name);
		return -1;
	}
	ai->handle = pcm;
	if (ai->format != -1) {
		/* we're going to play: initalize sample format */
		return initialize_device(ai);
	} else {
		/* query mode; sample format will be set for each query */
		return 0;
	}
}

static int rates_match(long int desired, unsigned int actual)
{
	return actual * 100 > desired * (100 - AUDIO_RATE_TOLERANCE) &&
	       actual * 100 < desired * (100 + AUDIO_RATE_TOLERANCE);
}

static int initialize_device(struct audio_info_struct *ai)
{
	snd_pcm_hw_params_t *hw;
	int i;
	snd_pcm_format_t format;
	unsigned int rate;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t period_size;
	snd_pcm_sw_params_t *sw;
	snd_pcm_uframes_t boundary;

	snd_pcm_hw_params_alloca(&hw);
	if (snd_pcm_hw_params_any(ai->handle, hw) < 0) {
		fprintf(stderr, "initialize_device(): no configuration available\n");
		return -1;
	}
	if (snd_pcm_hw_params_set_access(ai->handle, hw, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		fprintf(stderr, "initialize_device(): device does not support interleaved access\n");
		return -1;
	}
	format = SND_PCM_FORMAT_UNKNOWN;
	for (i = 0; i < NUM_FORMATS; ++i) {
		if (ai->format == format_map[i].mpg123) {
			format = format_map[i].alsa;
			break;
		}
	}
	if (format == SND_PCM_FORMAT_UNKNOWN) {
		fprintf(stderr, "initialize_device(): invalid sample format %d\n", ai->format);
		errno = EINVAL;
		return -1;
	}
	if (snd_pcm_hw_params_set_format(ai->handle, hw, format) < 0) {
		fprintf(stderr, "initialize_device(): cannot set format %s\n", snd_pcm_format_name(format));
		return -1;
	}
	if (snd_pcm_hw_params_set_channels(ai->handle, hw, ai->channels) < 0) {
		fprintf(stderr, "initialize_device(): cannot set %d channels\n", ai->channels);
		return -1;
	}
	rate = ai->rate;
	if (snd_pcm_hw_params_set_rate_near(ai->handle, hw, &rate, NULL) < 0) {
		fprintf(stderr, "initialize_device(): cannot set rate %u\n", rate);
		return -1;
	}
	if (!rates_match(ai->rate, rate)) {
		fprintf(stderr, "initialize_device(): rate %ld not available, using %u\n", ai->rate, rate);
		/* return -1; */
	}
	buffer_size = rate; /* one second */
	if (snd_pcm_hw_params_set_buffer_size_near(ai->handle, hw, &buffer_size) < 0) {
		fprintf(stderr, "initialize_device(): cannot set buffer size\n");
		return -1;
	}
	period_size = buffer_size / 4;
	if (snd_pcm_hw_params_set_period_size_near(ai->handle, hw, &period_size, NULL) < 0) {
		fprintf(stderr, "initialize_device(): cannot set period size\n");
		return -1;
	}
	if (snd_pcm_hw_params(ai->handle, hw) < 0) {
		fprintf(stderr, "initialize_device(): cannot set hw params\n");
		return -1;
	}

	snd_pcm_sw_params_alloca(&sw);
	if (snd_pcm_sw_params_current(ai->handle, sw) < 0) {
		fprintf(stderr, "initialize_device(): cannot get sw params\n");
		return -1;
	}
	/* start playing after the first write */
	if (snd_pcm_sw_params_set_start_threshold(ai->handle, sw, 1) < 0) {
		fprintf(stderr, "initialize_device(): cannot set start threshold\n");
		return -1;
	}
	if (snd_pcm_sw_params_get_boundary(sw, &boundary) < 0) {
		fprintf(stderr, "initialize_device(): cannot get boundary\n");
		return -1;
	}
	/* never stop on underruns */
	if (snd_pcm_sw_params_set_stop_threshold(ai->handle, sw, boundary) < 0) {
		fprintf(stderr, "initialize_device(): cannot set stop threshold\n");
		return -1;
	}
	/* wake up on every interrupt */
	if (snd_pcm_sw_params_set_avail_min(ai->handle, sw, 1) < 0) {
		fprintf(stderr, "initialize_device(): cannot set min avail\n");
		return -1;
	}
	/* always write as many frames as possible */
	if (snd_pcm_sw_params_set_xfer_align(ai->handle, sw, 1) < 0) {
		fprintf(stderr, "initialize_device(): cannot set transfer alignment\n");
		return -1;
	}
	/* play silence when there is an underrun */
	if (snd_pcm_sw_params_set_silence_size(ai->handle, sw, boundary) < 0) {
		fprintf(stderr, "initialize_device(): cannot set silence size\n");
		return -1;
	}
	if (snd_pcm_sw_params(ai->handle, sw) < 0) {
		fprintf(stderr, "initialize_device(): cannot set sw params\n");
		return -1;
	}
	prepared = 1;
	return 0;
}

int audio_get_formats(struct audio_info_struct *ai)
{
	snd_pcm_hw_params_t *hw;
	unsigned int rate;
	int supported_formats, i;

	snd_pcm_hw_params_alloca(&hw);
	if (snd_pcm_hw_params_any(ai->handle, hw) < 0) {
		fprintf(stderr, "audio_get_formats(): no configuration available\n");
		return -1;
	}
	if (snd_pcm_hw_params_set_access(ai->handle, hw, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
		return -1;
	if (snd_pcm_hw_params_set_channels(ai->handle, hw, ai->channels) < 0)
		return 0;
	rate = ai->rate;
	if (snd_pcm_hw_params_set_rate_near(ai->handle, hw, &rate, NULL) < 0)
		return -1;
	if (!rates_match(ai->rate, rate))
		return 0;
	supported_formats = 0;
	for (i = 0; i < NUM_FORMATS; ++i) {
		if (snd_pcm_hw_params_test_format(ai->handle, hw, format_map[i].alsa) == 0)
			supported_formats |= format_map[i].mpg123;
	}
	return supported_formats;
}

int audio_play_samples(struct audio_info_struct *ai, unsigned char *buf, int bytes)
{
	if(!prepared)
	{
		if((prepared = snd_pcm_prepare(ai->handle)) < 0)
		{
			error1("cannot prepared device: %s", snd_strerror(prepared));
			prepared = 0;
		}
		else prepared = 1;
	}
	snd_pcm_uframes_t frames = snd_pcm_bytes_to_frames(ai->handle, bytes);
	snd_pcm_sframes_t written = snd_pcm_writei(ai->handle, buf, frames);
	if (written >= 0)
		return snd_pcm_frames_to_bytes(ai->handle, written);
	else
		return written;
}

void audio_queueflush(struct audio_info_struct *ai)
{
	/*
		that one causes trouble (bug 1536513):
		- buffer chokes on it in terminal control mode
		- also without buffer output is ceased after seeking back in terminal control mode
	*/
	/* if(!param.usebuffer)*/
	if(prepared)
	{
		snd_pcm_drop(ai->handle);
		prepared = 0;
	}
	/*else warning("alsa output together with buffer mode is buggy atm!"); */
}

int audio_close(struct audio_info_struct *ai)
{
	if (prepared && snd_pcm_state(ai->handle) == SND_PCM_STATE_RUNNING)
		snd_pcm_drain(ai->handle);
	return snd_pcm_close(ai->handle);
}
