/*
	libao: audio output via libao

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Nicholas J. Humfrey
*/

#include <stdio.h>
#include <math.h>
#include <ao/ao.h>

#include "mpg123app.h"


static int open_libao(audio_output_t *ao)
{
	ao_device *device = NULL;
	ao_sample_format format;
	int driver = -1;
	int err = 0;
	char* filename = NULL;

	if(!ai) return -1;

	/* Return if already open */
	if (ao->handle) {
		error("open_libao(): error, already open");
		return -1;
	}

	/* Work out the sample size	 */
	switch (ao->format) {
		case MPG123_ENC_SIGNED_16:
			format.bits = 16;
		break;
		
		case MPG123_ENC_SIGNED_8:
			format.bits = 8;
		break;
		
		/* For some reason we get called with format=-1 initially */
		/* Just prentend that it didn't happen */
		case -1:
			return 0;
		break;
		
		default:
			error("open_libao(): Unsupported Audio Format: %d", ao->format);
			return -1;
		break;
	}
		

	/* Set the reset of the format */
	format.channels = ao->channels;
	format.rate = ao->rate;
	format.byte_format = AO_FMT_NATIVE;

	/* Initialize libao */
	audio_initialize();
	
	/* Choose the driver to use */
	if (ao->device) {
		/* parse device:filename; remember to free stuff before bailing out */ 
		char* search_ptr;
		if( (search_ptr = strchr(ao->device, ':')) != NULL )
		{
			/* going to split up the info in new memory to preserve the original string */
			size_t devlen = search_ptr-ao->device+1;
			size_t filelen = strlen(ao->device)-devlen+1;
			debug("going to allocate %lu:%lu bytes", (unsigned long)devlen, (unsigned long)filelen);
			char* devicename = malloc(devlen*sizeof(char));
			devicename[devlen-1] = 0;
			filename = malloc(filelen*sizeof(char));
			filename[filelen-1] = 0;
			if((devicename != NULL) && (filename != NULL))
			{
				strncpy(devicename, ao->device, devlen-1);
				strncpy(filename, search_ptr+1, filelen-1);
				if(filename[0] == 0){ free(filename); filename = NULL; }
			}
			else
			{
				if(filename != NULL) free(filename);
				filename = NULL;
				error("open_libao(): out of memory!");
				err = -1;
			}
			driver = ao_driver_id( devicename );
			if(devicename != NULL) free(devicename);
		}
		else driver = ao_driver_id( ao->device );
	} else {
		driver = ao_default_driver_id();
	}

	if(!err)
	{
		if(driver < 0)
		{
			error("open_libao(): bad driver, try one of these with the -a option:");
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
				else error("open_libao(): please specify a filename via -a driver:file (even just - for stdout)");
			}
			else device = ao_open_live(driver, &format, NULL /* no options */);

			if (device == NULL) {
				error("open_libao(): error opening device.");
				err = -1;
			}

		}
		else
		{
			error("open_libao(): somehow I got an invalid driver id!");
			err = -1;
		}
	}
	if(!err)
	{
		/* Store it for later */
		ao->handle = (void*)device;
	}
	/* always do this here! */
	if(filename != NULL) free(filename);
	/* _then_ return */
	return err;
}


/* The two formats we support */
static int get_formats_libao(audio_output_t *ao)
{
	return MPG123_ENC_SIGNED_16 | MPG123_ENC_SIGNED_8;
}

static int write_libao(audio_output_t *ao,unsigned char *buf,int len)
{
	int res = 0;
	ao_device *device = (ao_device*)ao->handle;
	
	res = ao_play(device, (char*)buf, len);
	if (res==0) {
		error("write_libao(): error playing samples");
		return -1;
	} 
	
	return len;
}

static int close_libao(audio_output_t *ao)
{
	ao_device *device = (ao_device*)ao->handle;

	/* Close and shutdown */
	if (device) {
		ao_close(device);
		ao->handle = NULL;
    }
    
	audio_shutdown();

	return 0;
}

static void flush_libao(audio_output_t *ao)
{
}

static static int deinit_libao(audio_output_t* ao)
{
	ao_shutdown();
}

static int init_libao(audio_output_t* ao)
{
	if (ao==NULL) return -1;

	/* Initialise LibAO */
	ao_initialize();

	/* Set callbacks */
	ao->open = open_libao;
	ao->flush = flush_libao;
	ao->write = write_libao;
	ao->get_formats = get_formats_libao;
	ao->close = close_libao;
	ao->deinit = deinit_libao;

	/* Success */
	return 0;
}





/* 
	Module information data structure
*/
mpg123_module_t mpg123_output_module_info = {
	/* api_version */	MPG123_MODULE_API_VERSION,
	/* name */			"libao",						
	/* description */	"Output audio using LibAO.",
	/* revision */		"$Rev:$",						
	/* handle */		NULL,
	
	/* init_output */	init_libao,						
};


