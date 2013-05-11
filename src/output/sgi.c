/*
	sgi: audio output on SGI boxen

	copyright ?-2013 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written (as it seems) by Thomas Woerner
*/

#include <fcntl.h>

#include <dmedia/audio.h>

#include "mpg123app.h"
#include "errno.h"
#include "debug.h"

/* struct audiolib_info alinfo; */
int current_dev;
char *current_device;

static int set_rate(audio_output_t *ao, ALconfig config)
{
	int dev = alGetDevice(config);
	ALpv params[1];

	/* Make sure the device is OK */
	if (dev < 0)
	{
		error1("set_rate: %s", alGetErrorString(oserror()));
		return 1;      
	}
	
	if (ao->rate > 0) {
		params[0].param = AL_OUTPUT_RATE;
		params[0].value.ll = alDoubleToFixed(ao->rate);
		
		if (alSetParams(dev, params,1) < 0)
			error1("set_rate: %s", alGetErrorString(oserror()));
	}
	
	return 0;
}


static int set_channels(audio_output_t *ao, ALconfig config)
{
	int ret;
	
	if(ao->channels == 2) {
		ret = alSetChannels(config, AL_STEREO);
	} else {
		ret = alSetChannels(config, AL_MONO);
	}
	
	if (ret < 0)
		error1("set_channels : %s", alGetErrorString(oserror()));
	
	return 0;
}

static int set_format(audio_output_t *ao, ALconfig config)
{
	if (alSetSampFmt(config,AL_SAMPFMT_TWOSCOMP) < 0)
		error1("set_format : %s", alGetErrorString(oserror()));
	
	if (alSetWidth(config,AL_SAMPLE_16) < 0)
		error1("set_format : %s", alGetErrorString(oserror()));
	
	return 0;
}


static int open_sgi(audio_output_t *ao)
{
	ALport port = NULL;
	ALconfig config = alNewConfig();

	ao->userptr = NULL;

	/* Test for correct completion */
	if (config == 0) {
		error1("open_sgi: %s", alGetErrorString(oserror()));
		return -1;
	}

	/* Setup output device to specified device name. If there is no device name
	specified in ao structure, use the default for output */
	if ((ao->device) != NULL) {
		current_dev = alGetResourceByName(AL_SYSTEM, ao->device, AL_OUTPUT_DEVICE_TYPE);

		debug2("Dev: %s %i", ao->device, current_dev);

		if (!current_dev) {
			int i, numOut;
			char devname[32];
			ALpv pv[1];
			ALvalue *alvalues;

			error2("Invalid audio resource: %s (%s)", ao->device, alGetErrorString(oserror()));

			if ((numOut= alQueryValues(AL_SYSTEM,AL_DEFAULT_OUTPUT,0,0,0,0))>=0) {
				fprintf(stderr, "There are %d output devices on this system.\n", numOut);
			}
			else {
				fprintf(stderr, "Can't find output devices. alQueryValues failed: %s\n", alGetErrorString(oserror()));
				return -1;
			}

			alvalues = malloc(sizeof(ALvalue) * numOut);
			i = alQueryValues(AL_SYSTEM, AL_DEFAULT_OUTPUT, alvalues, numOut, pv, 0);
			if (i == -1) {
				error1("alQueryValues: %s", alGetErrorString(oserror()));
			} else {
				for (i=0; i < numOut; i++){ 
					pv[0].param = AL_NAME;
					pv[0].value.ptr = devname;
					pv[0].sizeIn = 32;
					alGetParams(alvalues[i].i, pv, 1);

					fprintf(stderr, "%i: %s\n", i, devname);
				}
			}
			free(alvalues);

			return -1;
		}

		if (alSetDevice(config, current_dev) < 0) {
			error1("open: alSetDevice : %s",alGetErrorString(oserror()));
			return -1;
		}
	} else {
		current_dev = AL_DEFAULT_OUTPUT;
	}

	/* Set the device */
	if (alSetDevice(config, current_dev) < 0)
	{
		error1("open_sgi: %s", alGetErrorString(oserror()));
		return -1;
	}

	/* Set port parameters */

	if (alSetQueueSize(config, 131069) < 0) {
		error1("open_sgi: setting audio buffer failed: %s", alGetErrorString(oserror()));
		return -1;
	}
	
	set_format(ao, config);
	set_rate(ao, config);
	set_channels(ao, config);
	
	/* Open the audio port */
	port = alOpenPort("mpg123-VSC", "w", config);
	if (port == NULL) {
		error1("Unable to open audio channel: %s", alGetErrorString(oserror()));
		return -1;
	}
	
	ao->userptr = (void*)port;

	alFreeConfig(config);
	
	return 1;
}


static int get_formats_sgi(audio_output_t *ao)
{
	return MPG123_ENC_SIGNED_16;
}


static int write_sgi(audio_output_t *ao, unsigned char *buf, int len)
{
	if (ao->userptr == NULL) {error1("foo %i",1);return -1;}

	ALport port = (ALport)ao->userptr;
	ALconfig alconfig = alGetConfig(port);

	if(ao->channels == 2) {
		alWriteFrames(port, buf, len>>2);
	} else {
		alWriteFrames(port, buf, len>>1);
	}
	
	return len;
}


static int close_sgi(audio_output_t *ao)
{
	ALport port = (ALport)ao->userptr;
	
	if (port) {
		// play all remaining samples
		while(alGetFilled(port) > 0) sginap(1);  
		alClosePort(port);
		ao->userptr=NULL;
	}
	
	return 0;
}

static void flush_sgi(audio_output_t *ao)
{
	ALport port = (ALport)ao->userptr;

	if (port) {
		alDiscardFrames(port, alGetFilled(port));
	}
}

static int init_sgi(audio_output_t* ao)
{
	if (ao == NULL) return -1;

	/* Set callbacks */
	ao->open = open_sgi;
	ao->flush = flush_sgi;
	ao->write = write_sgi;
	ao->get_formats = get_formats_sgi;
	ao->close = close_sgi;

	/* Success */
	return 0;
}

/* 
	Module information data structure
*/
mpg123_module_t mpg123_output_module_info = {
	/* api_version */	MPG123_MODULE_API_VERSION,
	/* name */			"sgi",						
	/* description */	"Audio output for SGI.",
	/* revision */		"$Rev:$",						
	/* handle */		NULL,
	
	/* init_output */	init_sgi,						
};
