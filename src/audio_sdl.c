/*
	audio_portaudio.c: audio output via PortAudio cross-platform audio API

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Nicholas J. Humfrey
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <SDL.h>

#include "config.h"
#include "debug.h"
#include "audio.h"
#include "sfifo.h"
#include "mpg123.h"


#define SAMPLE_SIZE			(2)
#define FRAMES_PER_BUFFER	(256)
#define FIFO_DURATION		(0.5f)

static int sdl_initialised=0;
static sfifo_t fifo;



/* The audio function callback takes the following parameters:
       stream:  A pointer to the audio buffer to be filled
       len:     The length (in bytes) of the audio buffer
*/
static void
audio_callback_sdl(void *udata, Uint8 *stream, int len)
{
	/* struct audio_info_struct *ai = udata; */
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


int audio_open(struct audio_info_struct *ai)
{
	
	/* Initalise SDL */
	if (!sdl_initialised)  {
		if (SDL_Init( SDL_INIT_AUDIO ) ) {
			error1("Failed to initialise SDL: %s\n", SDL_GetError());
			return -1;
		}
		sdl_initialised=1;
	}
	
	

	/* Open an audio I/O stream. */
	if (ai->rate > 0 && ai->channels >0 ) {
		SDL_AudioSpec wanted;
		size_t ringbuffer_len;
	
		/* L16 uncompressed audio data, using 16-bit signed representation in twos 
		   complement notation - system endian-ness. */
		wanted.format = AUDIO_S16SYS;
		wanted.samples = 1024;  /* Good low-latency value for callback */ 
		wanted.callback = audio_callback_sdl; 
		wanted.userdata = ai; 
		wanted.channels = ai->channels; 
		wanted.freq = ai->rate; 
	
		/* Open the audio device, forcing the desired format */
		if ( SDL_OpenAudio(&wanted, NULL) ) {
			error1("Couldn't open SDL audio: %s\n", SDL_GetError());
			return -1;
		}
	
		/* Initialise FIFO */
		ringbuffer_len = ai->rate * FIFO_DURATION * SAMPLE_SIZE *ai->channels;
		debug2( "Allocating %d byte ring-buffer (%f seconds)", ringbuffer_len, (float)FIFO_DURATION);
		sfifo_init( &fifo, ringbuffer_len );
									   
	}
	
	return(0);
}


int audio_get_formats(struct audio_info_struct *ai)
{
	/* Only implemented Signed 16-bit audio for now */
	return AUDIO_FORMAT_SIGNED_16;
}


int audio_play_samples(struct audio_info_struct *ai, unsigned char *buf, int len)
{

	/* Sleep for half the length of the FIFO */
	while (sfifo_space( &fifo ) < len ) {
		usleep( (FIFO_DURATION/2) * 1000000 );
	}
	
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

int audio_close(struct audio_info_struct *ai)
{
	SDL_CloseAudio();
	
	sfifo_close( &fifo );
	
	return 0;
}

void audio_queueflush(struct audio_info_struct *ai)
{
	SDL_PauseAudio(1);
	
	sfifo_flush( &fifo );	
}

