/*
	mpg123 - Mpeg Audio Player
	Copyright (C) 1995-2005  The Mpg123 Project, All rights reserved.

	See the file 'AUTHORS' for a full list of contributors.
	
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#include <stdio.h>
#include <math.h>
#include <ao/ao.h>

#include "mpg123.h"


static int initialized=0;

static void audio_initialize()
{
	if (!initialized) {
		ao_initialize();
		initialized=1;
	}
}

static void audio_shutdown()
{
	if (initialized) {
		ao_shutdown();
		initialized=0;
	}
}


int audio_open(struct audio_info_struct *ai)
{
	ao_device *device = NULL;
	ao_sample_format format;
	int driver;

	if(!ai) return -1;

	// Return if already open
	if (ai->handle) {
		fprintf(stderr, "audio_open(): error, already open\n");
		return -1;
	}

	// Work out the sample size	
	switch (ai->format) {
		case AUDIO_FORMAT_SIGNED_16:
			format.bits = 16;
		break;
		
		case AUDIO_FORMAT_SIGNED_8:
			format.bits = 8;
		break;
		
		// For some reason we get called with format=-1 initially
		// Just prentend that it didn't happen
		case -1:
			return 0;
		break;
		
		default:
			fprintf(stderr, "audio_open(): Unsupported Audio Format: %d\n", ai->format);
			return -1;
		break;
	}
		

	// Set the reset of the format
	format.channels = ai->channels;
	format.rate = ai->rate;
	format.byte_format = AO_FMT_NATIVE;

	// Initialize libao
	audio_initialize();
	
	// Choose the driver to use
	if (ai->device) {
		driver = ao_driver_id( ai->device );
	} else {
		driver = ao_default_driver_id();
	}

	// Open driver
	device = ao_open_live(driver, &format, NULL /* no options */);
	if (device == NULL) {
		fprintf(stderr, "audio_open(): error opening device.\n");
		return 1;
	} 

	// Store it for later
	ai->handle = (void*)device;
	
	return(0);
}


// The two formats we support
int audio_get_formats(struct audio_info_struct *ai)
{
	return AUDIO_FORMAT_SIGNED_16 | AUDIO_FORMAT_SIGNED_8;
}

int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
	int res = 0;
	ao_device *device = (ao_device*)ai->handle;
	
	res = ao_play(device, buf, len);
	if (res==0) {
		fprintf(stderr, "audio_play_samples(): error playing samples\n");
		return -1;
	} 
	
	return len;
}

int audio_close(struct audio_info_struct *ai)
{
	ao_device *device = (ao_device*)ai->handle;

	// Close and shutdown
	if (device) {
		ao_close(device);
		ai->handle = NULL;
    }
    
	audio_shutdown();

	return 0;
}

void audio_queueflush(struct audio_info_struct *ai)
{
}

