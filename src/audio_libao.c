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

#include "config.h"
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
	int driver = -1;
	int err = 0;
	char* filename = NULL;

	if(!ai) return -1;

	/* Return if already open */
	if (ai->handle) {
		fprintf(stderr, "audio_open(): error, already open\n");
		return -1;
	}

	/* Work out the sample size	 */
	switch (ai->format) {
		case AUDIO_FORMAT_SIGNED_16:
			format.bits = 16;
		break;
		
		case AUDIO_FORMAT_SIGNED_8:
			format.bits = 8;
		break;
		
		/* For some reason we get called with format=-1 initially */
		/* Just prentend that it didn't happen */
		case -1:
			return 0;
		break;
		
		default:
			fprintf(stderr, "audio_open(): Unsupported Audio Format: %d\n", ai->format);
			return -1;
		break;
	}
		

	/* Set the reset of the format */
	format.channels = ai->channels;
	format.rate = ai->rate;
	format.byte_format = AO_FMT_NATIVE;

	/* Initialize libao */
	audio_initialize();
	
	/* Choose the driver to use */
	if (ai->device) {
		/* parse device:filename; remember to free stuff before bailing out */ 
		char* search_ptr;
		if( (search_ptr = strchr(ai->device, ':')) != NULL )
		{
			/* going to split up the info in new memory to preserve the original string */
			size_t devlen = search_ptr-ai->device+1;
			size_t filelen = strlen(ai->device)-devlen+1;
			fprintf(stderr, "going to allocate %zu:%zu bytes\n", devlen, filelen);
			char* devicename = malloc(devlen*sizeof(char));
			devicename[devlen-1] = 0;
			filename = malloc(filelen*sizeof(char));
			filename[filelen-1] = 0;
			if((devicename != NULL) && (filename != NULL))
			{
				strncpy(devicename, ai->device, devlen-1);
				strncpy(filename, search_ptr+1, filelen-1);
				if(filename[0] == 0){ free(filename); filename = NULL; }
			}
			else
			{
				if(filename != NULL) free(filename);
				filename = NULL;
				fprintf(stderr, "audio_open(): out of memory!\n");
				err = -1;
			}
			driver = ao_driver_id( devicename );
			if(devicename != NULL) free(devicename);
		}
		else driver = ao_driver_id( ai->device );
	} else {
		driver = ao_default_driver_id();
	}

	if(!err)
	{
		if(driver < 0)
		{
			fprintf(stderr, "audio_open(): bad driver, try one of these with the -a option:\n");
			int count = 0;
			ao_info** aolist = ao_driver_info_list(&count);
			int c;
			for(c=0; c < count; ++c)
			fprintf(stderr, "%s%s\t(%s)\n",
			        aolist[c]->short_name,
			        aolist[c]->type == AO_TYPE_FILE ? ":<filename>" : "",
			        aolist[c]->name);
			fprintf(stderr, "\n");
			err = -1;
		}
	}

	if(!err)
	{
		ao_info* driverinfo = ao_driver_info(driver);
		if(driverinfo != NULL)
		{
			/* Open driver, files are overwritten - the multiple audio_open calls force it... */
			if(driverinfo->type == AO_TYPE_FILE)
			{
				if(filename != NULL) device = ao_open_file(driver, filename, 1, &format, NULL);
				else fprintf(stderr, "audio_open(): please specify a filename via -a driver:file (even just - for stdout)\n");
			}
			else device = ao_open_live(driver, &format, NULL /* no options */);

			if (device == NULL) {
				fprintf(stderr, "audio_open(): error opening device.\n");
				err = -1;
			}

		}
		else
		{
			fprintf(stderr, "audio_open(): somehow I got an invalid driver id!\n");
			err = -1;
		}
	}
	if(!err)
	{
		/* Store it for later */
		ai->handle = (void*)device;
	}
	/* always do this here! */
	if(filename != NULL) free(filename);
	/* _then_ return */
	return err;
}


/* The two formats we support */
int audio_get_formats(struct audio_info_struct *ai)
{
	return AUDIO_FORMAT_SIGNED_16 | AUDIO_FORMAT_SIGNED_8;
}

int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
	int res = 0;
	ao_device *device = (ao_device*)ai->handle;
	
	res = ao_play(device, (char*)buf, len);
	if (res==0) {
		fprintf(stderr, "audio_play_samples(): error playing samples\n");
		return -1;
	} 
	
	return len;
}

int audio_close(struct audio_info_struct *ai)
{
	ao_device *device = (ao_device*)ai->handle;

	/* Close and shutdown */
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

