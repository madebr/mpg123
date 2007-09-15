/*
	hp: audio output for HP-UX

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123.h"
#include <fcntl.h>
#include <sys/audio.h>



static int audio_set_rate(audio_output_t *ao)
{
	if(ao->rate >= 0) {
		return ioctl(ao->fn,AUDIO_SET_SAMPLE_RATE,ao->rate);
	} else {
		return 0;
	}
}

static int audio_set_channels(audio_output_t *ao)
{
	if(ao->channels<0) return 0;
	return ioctl(ao->fn,AUDIO_SET_CHANNELS,ao->channels);
}

static int audio_set_format(audio_output_t *ao)
{
	int fmt;
	
	switch(ao->format) {
		case -1:
		case AUDIO_FORMAT_SIGNED_16:
		default: 
			fmt = AUDIO_FORMAT_LINEAR16BIT;
		break;
		case AUDIO_FORMAT_UNSIGNED_8:
			fprintf(stderr,"unsigned 8 bit linear not supported\n");
			return -1;
		case AUDIO_FORMAT_SIGNED_8:
			fprintf(stderr,"signed 8 bit linear not supported\n");
			return -1;
		case AUDIO_FORMAT_ALAW_8:
			fmt = AUDIO_FORMAT_ALAW;
		break;
		case AUDIO_FORMAT_ULAW_8:
			fmt = AUDIO_FORMAT_ULAW;
		break;
	}
	return ioctl(ao->fn,AUDIO_SET_DATA_FORMAT,fmt);
}

static int audio_get_formats(audio_output_t *ao)
{
	return AUDIO_FORMAT_SIGNED_16;
}

static int audio_reset_parameters(audio_output_t *ao)
{
	int ret;
		ret = audio_set_format(ai);
	if(ret >= 0)
		ret = audio_set_channels(ai);
	if(ret >= 0)
		ret = audio_set_rate(ai);
	return ret;
}


int audio_open(audio_output_t *ao)
{
	struct audio_describe ades;
	struct audio_gain again;
	int i,audio;
	
	ao->fn = open("/dev/audio",O_RDWR);
	
	if(ao->fn < 0)
		return -1;
	
	
	ioctl(ao->fn,AUDIO_DESCRIBE,&ades);
	
	if(ao->gain != -1)
	{
		if(ao->gain > ades.max_transmit_gain)
		{
			fprintf(stderr,"your gainvalue was to high -> set to maximum.\n");
			ao->gain = ades.max_transmit_gain;
		}
		if(ao->gain < ades.min_transmit_gain)
		{
			fprintf(stderr,"your gainvalue was to low -> set to minimum.\n");
			ao->gain = ades.min_transmit_gain;
		}
		again.channel_mask = AUDIO_CHANNEL_0 | AUDIO_CHANNEL_1;
		ioctl(ao->fn,AUDIO_GET_GAINS,&again);
		again.cgain[0].transmit_gain = ao->gain;
		again.cgain[1].transmit_gain = ao->gain;
		again.channel_mask = AUDIO_CHANNEL_0 | AUDIO_CHANNEL_1;
		ioctl(ao->fn,AUDIO_SET_GAINS,&again);
	}
	
	if(param.output_flags != -1)
	{
		if(param.output_flags & AUDIO_OUT_INTERNAL_SPEAKER)
			ioctl(ao->fn,AUDIO_SET_OUTPUT,AUDIO_OUT_SPEAKER);
		else if(param.output_flags & AUDIO_OUT_HEADPHONES)
			ioctl(ao->fn,AUDIO_SET_OUTPUT,AUDIO_OUT_HEADPHONE);
		else if(param.output_flags & AUDIO_OUT_LINE_OUT)
			ioctl(ao->fn,AUDIO_SET_OUTPUT,AUDIO_OUT_LINE);
	}
	
	if(ao->rate == -1)
		ao->rate = 44100;
	
	for(i=0;i<ades.nrates;i++)
	{
		if(ao->rate == ades.sample_rate[i])
			break;
	}
	if(i == ades.nrates)
	{
		fprintf(stderr,"Can't set sample-rate to %ld.\n",ao->rate);
		i = 0;
	}
	
	if(audio_reset_parameters(ai) < 0)
		return -1;
	
	return ao->fn;
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

void audio_queueflush(audio_output_t *ao)
{
}

