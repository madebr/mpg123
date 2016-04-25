/*
	sdl: audio output via SDL cross-platform API

	copyright 2006-2016 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Nicholas J. Humfrey
*/

#include "out123_int.h"
#include <math.h>

#include <SDL.h>

#ifdef WIN32
#include <windows.h>
#endif

/* Including the sfifo code locally, to avoid module linkage issues. */
#define SFIFO_STATIC
#include "sfifo.c"

#include "debug.h"


#define SAMPLE_SIZE			(2)
#define FRAMES_PER_BUFFER	(256)
/* Performance of SDL with ALSA is a bit of a mystery to me. Regardless
   of buffer size here, I just cannot avoid buffer underruns on my system.
   SDL always chooses 1024x2 periods, which seems to be just not quite
   enough on the Thinkpad:-/ Choosing 0.2 s as a plentiful default instead
   of 0.5 s which is just a lie. */
#define FIFO_DURATION		(ao->device_buffer > 0. ? ao->device_buffer : 0.2)
#define BUFFER_SAMPLES		((FIFO_DURATION*ao->rate)/2)

/* The audio function callback takes the following parameters:
       stream:  A pointer to the audio buffer to be filled
       len:     The length (in bytes) of the audio buffer
*/
static void audio_callback_sdl(void *udata, Uint8 *stream, int len)
{
	out123_handle *ao = (out123_handle*)udata;
	sfifo_t *fifo = (sfifo_t*)ao->userptr;
	int bytes_read;
	int bytes_avail;

	bytes_avail = sfifo_used(fifo);
	if(bytes_avail < len) len = bytes_avail;

	/* Read audio from FIFO to SDL's buffer */
	bytes_read = sfifo_read( fifo, stream, len );

	if (len!=bytes_read)
	warning2("Error reading from the FIFO (wanted=%u, bytes_read=%u).\n", len, bytes_read);
}

static int open_sdl(out123_handle *ao)
{
	sfifo_t *fifo = (sfifo_t*)ao->userptr;
	
	/* Open an audio I/O stream. */
	if (ao->rate > 0 && ao->channels >0 ) {
		size_t ringbuffer_len;
		SDL_AudioSpec wanted;
	
		/* L16 uncompressed audio data, using 16-bit signed representation in twos 
		   complement notation - system endian-ness. */
		wanted.format = AUDIO_S16SYS;
		/* Seems reasonable to demand a buffer size related to the device
		   buffer. */
		wanted.samples = BUFFER_SAMPLES;
		wanted.callback = audio_callback_sdl; 
		wanted.userdata = ao; 
		wanted.channels = ao->channels; 
		wanted.freq = ao->rate; 

		/* Open the audio device, forcing the desired format
		   Actually, it is still subject to constraints by hardware.
		   Need to have sample rate checked beforehand! SDL will
		   happily play 22 kHz files with 44 kHz hardware rate!
		   Same with channel count. No conversion. The manual is a bit
		   misleading on that (only talking about sample format, I guess). */
		if ( SDL_OpenAudio(&wanted, NULL) )
		{
			if(!AOQUIET)
				error1("Couldn't open SDL audio: %s\n", SDL_GetError());
			return -1;
		}
		
		/* Initialise FIFO */
		ringbuffer_len = ao->rate * FIFO_DURATION * SAMPLE_SIZE *ao->channels;
		debug2( "Allocating %d byte ring-buffer (%f seconds)", (int)ringbuffer_len, (float)FIFO_DURATION);
		if (sfifo_init( fifo, ringbuffer_len ) && !AOQUIET)
			error1( "Failed to initialise FIFO of size %d bytes", (int)ringbuffer_len );
	}
	
	return(0);
}


static int get_formats_sdl(out123_handle *ao)
{
	/* Got no better idea than to just take 16 bit and run with it */
	return MPG123_ENC_SIGNED_16;
#if 0
	/*
		This code would "properly" test audio format support.
		But thing is, SDL will always say yes and amen to everything, but it takes
		an awful amount of time to get all the variants tested (about 2 seconds,
		for example). I have seen SDL builds that do proper format conversion
		behind your back, I have seen builds that do not. Every build seems to
		claim that it does, though. Just hope you're lucky and your SDL works.
		Otherwise, use a proper audio output API.
	*/
	SDL_AudioSpec wanted, got;

	/* Only implemented Signed 16-bit audio for now.
	   The SDL manual doesn't suggest more interesting formats
	   like S24 or S32 anyway. */
	wanted.format = AUDIO_S16SYS;
	wanted.samples = BUFFER_SAMPLES;
	wanted.callback = audio_callback_sdl;
	wanted.userdata = ao;
	wanted.channels = ao->channels;
	wanted.freq = ao->rate;

	if(SDL_OpenAudio(&wanted, &got)) return 0;
	SDL_CloseAudio();
fprintf(stderr, "wanted rate: %li got rate %li\n", (long)wanted.freq, (long)got.freq);
	return (got.freq == ao->rate && got.channels == ao->channels)
		? MPG123_ENC_SIGNED_16
		: 0;
#endif
}


static int write_sdl(out123_handle *ao, unsigned char *buf, int len)
{
	sfifo_t *fifo = (sfifo_t*)ao->userptr;
	int len_remain = len;

	/* Some busy waiting, but feed what is possible. */
	while(len_remain) /* Note: input len is multiple of framesize! */
	{
		int block = sfifo_space(fifo);
		block -= block % ao->framesize;
		if(block > len_remain)
			block = len_remain;
		if(block)
		{
			sfifo_write(fifo, buf, block);
			len_remain -= block;
			buf += block;
			/* Unpause once the buffer is 50% full */
			if (sfifo_used(fifo) > (sfifo_size(fifo)/2) )
				SDL_PauseAudio(0);
		}
		if(len_remain)
		{
#ifdef WIN32
		Sleep( (0.1*FIFO_DURATION) * 1000);
#else
		usleep( (0.1*FIFO_DURATION) * 1000000 );
#endif
		}
	}
	return len;
}

static int close_sdl(out123_handle *ao)
{
	int stuff;
	sfifo_t *fifo = (sfifo_t*)ao->userptr;

	/* Wait at least until SDL emptied the FIFO. */
	while((stuff = sfifo_used(fifo))>0)
	{
		int msecs = stuff*1000/ao->rate;
		debug1("still stuff for about %i ms there", msecs);
#ifdef WIN32
		Sleep(msecs/2);
#else
		usleep(msecs*1000/2);
#endif
	}

	SDL_CloseAudio();
	
	/* Free up the memory used by the FIFO */
	sfifo_close( fifo );
	
	return 0;
}

static void flush_sdl(out123_handle *ao)
{
	sfifo_t *fifo = (sfifo_t*)ao->userptr;

	SDL_PauseAudio(1);
	
	sfifo_flush( fifo );	
}


static int deinit_sdl(out123_handle* ao)
{
	/* Free up memory */
	if (ao->userptr) {
		free( ao->userptr );
		ao->userptr = NULL;
	}

	/* Shut down SDL */
	SDL_Quit();

	/* Success */
	return 0;
}


static int init_sdl(out123_handle* ao)
{
	if (ao==NULL) return -1;
	
	/* Set callbacks */
	ao->open = open_sdl;
	ao->flush = flush_sdl;
	ao->write = write_sdl;
	ao->get_formats = get_formats_sdl;
	ao->close = close_sdl;
	ao->deinit = deinit_sdl;
	
	/* Allocate memory */
	ao->userptr = malloc( sizeof(sfifo_t) );
	if (ao->userptr==NULL)
	{
		if(!AOQUIET)
			error( "Failed to allocated memory for FIFO structure" );
		return -1;
	}
	memset( ao->userptr, 0, sizeof(sfifo_t) );

	/* Initialise SDL */
	if (SDL_Init( SDL_INIT_AUDIO ) )
	{
		if(!AOQUIET)
			error1("Failed to initialise SDL: %s\n", SDL_GetError());
		return -1;
	}

	/* Success */
	return 0;
}


/* 
	Module information data structure
*/
mpg123_module_t mpg123_output_module_info = {
	/* api_version */	MPG123_MODULE_API_VERSION,
	/* name */			"sdl",
	/* description */	"Output audio using SDL (Simple DirectMedia Layer).",
	/* revision */		"$Rev:$",
	/* handle */		NULL,
	
	/* init_output */	init_sdl,
};
