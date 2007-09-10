/*
	sdl: audio output via SDL cross-platform API

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Nicholas J. Humfrey
*/

#include <math.h>

#include <SDL.h>

#include "audio.h"
#include "sfifo.h"
#include "mpg123.h"


#define SAMPLE_SIZE			(2)
#define FRAMES_PER_BUFFER	(256)
#define FIFO_DURATION		(0.5f)

static sfifo_t fifo;



/* The audio function callback takes the following parameters:
       stream:  A pointer to the audio buffer to be filled
       len:     The length (in bytes) of the audio buffer
*/
static void audio_callback_sdl(void *udata, Uint8 *stream, int len)
{
	/* audio_output_t *ao = udata; */
	int read;

	/* Only play if we have data left */
	if ( sfifo_used( &fifo ) < len ) {
		warning("Didn't have any audio data for SDL (buffer underflow)");
		SDL_PauseAudio(1);
		return;
	}

	/* Read audio from FIFO to SDL's buffer */
	read = sfifo_read( &fifo, stream, len );
	
	if (len!=read)
		warning2("Error reading from the FIFO (wanted=%u, read=%u).\n", len, read);

} 

static int open_sdl(audio_output_t *ao)
{
	
	/* Open an audio I/O stream. */
	if (ao->rate > 0 && ao->channels >0 ) {
		SDL_AudioSpec wanted;
		size_t ringbuffer_len;
	
		/* L16 uncompressed audio data, using 16-bit signed representation in twos 
		   complement notation - system endian-ness. */
		wanted.format = AUDIO_S16SYS;
		wanted.samples = 1024;  /* Good low-latency value for callback */ 
		wanted.callback = audio_callback_sdl; 
		wanted.userdata = ao; 
		wanted.channels = ao->channels; 
		wanted.freq = ao->rate; 
	
		/* Open the audio device, forcing the desired format */
		if ( SDL_OpenAudio(&wanted, NULL) ) {
			error1("Couldn't open SDL audio: %s\n", SDL_GetError());
			return -1;
		}
	
		/* Initialise FIFO */
		ringbuffer_len = ao->rate * FIFO_DURATION * SAMPLE_SIZE *ao->channels;
		debug2( "Allocating %d byte ring-buffer (%f seconds)", (int)ringbuffer_len, (float)FIFO_DURATION);
		sfifo_init( &fifo, ringbuffer_len );
									   
	}
	
	return(0);
}


static int get_formats_sdl(audio_output_t *ao)
{
	/* Only implemented Signed 16-bit audio for now */
	return AUDIO_FORMAT_SIGNED_16;
}


static int write_sdl(audio_output_t *ao, unsigned char *buf, int len)
{

	/* Sleep for half the length of the FIFO */
	while (sfifo_space( &fifo ) < len )
#ifdef WIN32
		Sleep( (FIFO_DURATION/2) * 1000);
#else
		usleep( (FIFO_DURATION/2) * 1000000 );
#endif
	
	/* Bung decoded audio into the FIFO 
		 SDL Audio locking probably isn't actually needed
		 as SFIFO claims to be thread safe...
	*/
	SDL_LockAudio();
	sfifo_write( &fifo, buf, len);
	SDL_UnlockAudio();
	
	
	/* Unpause once the buffer is 50% full */
	if (sfifo_used(&fifo) > (sfifo_size(&fifo)*0.5) ) SDL_PauseAudio(0);

	return len;
}

static int close_sdl(audio_output_t *ao)
{
	SDL_CloseAudio();
	
	sfifo_close( &fifo );
	
	return 0;
}

static void flush_sdl(audio_output_t *ao)
{
	SDL_PauseAudio(1);
	
	sfifo_flush( &fifo );	
}


static int init_sdl(audio_output_t* ao)
{
	if (ao==NULL) return -1;

	/* Initialise SDL */
	if (SDL_Init( SDL_INIT_AUDIO ) ) {
		error1("Failed to initialise SDL: %s\n", SDL_GetError());
		return -1;
	}
	
	/* Set callbacks */
	ao->open = open_sdl;
	ao->flush = flush_sdl;
	ao->write = write_sdl;
	ao->get_formats = get_formats_sdl;
	ao->close = close_sdl;

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
