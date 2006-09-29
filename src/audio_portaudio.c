/*
	audio_portaudio: audio output via PortAudio cross-platform audio API

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Nicholas J. Humfrey
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <portaudio.h>

#include "config.h"
#include "audio.h"
#include "sfifo.h"
#include "mpg123.h"
#include "debug.h"


#define SAMPLE_SIZE			(2)
#define FRAMES_PER_BUFFER	(256)
#define FIFO_DURATION		(0.5f)


static int pa_initialised=0;
static PaStream *pa_stream=NULL;
static sfifo_t fifo;




static int paCallback( void *inputBuffer, void *outputBuffer,
			 unsigned long framesPerBuffer,
			 PaTimestamp outTime, void *userData )
{
	struct audio_info_struct *ai = userData;
	unsigned long bytes = framesPerBuffer * SAMPLE_SIZE * ai->channels;
	
	if (sfifo_used(&fifo)<bytes) {
		error("ringbuffer for PortAudio is empty");
		return 1;
	} else {
		sfifo_read( &fifo, outputBuffer, bytes );
		return 0;
	}
}


int audio_open(struct audio_info_struct *ai)
{
	PaError err;
	
	/* Initialise PortAudio */
	if (!pa_initialised)  {
		err = Pa_Initialize();
		if( err != paNoError ) {
			error1("Failed to initialise PortAudio: %s", Pa_GetErrorText( err ));
			return -1;
		} else {
			pa_initialised=1;
		}
	}
	
	

	/* Open an audio I/O stream. */
	if (ai->rate > 0 && ai->channels >0 ) {
	
		err = Pa_OpenDefaultStream(
					&pa_stream,
					0,          	/* no input channels */
					ai->channels,	/* number of output channels */
					paInt16,		/* signed 16-bit samples */
					ai->rate,		/* sample rate */
					FRAMES_PER_BUFFER,	/* frames per buffer */
					0,				/* number of buffers, if zero then use default minimum */
					paCallback,		/* no callback - use blocking IO */
					ai );
			
		if( err != paNoError ) {
			error1("Failed to open PortAudio default stream: %s", Pa_GetErrorText( err ));
			return -1;
		}
		
		/* Initialise FIFO */
		sfifo_init( &fifo, ai->rate * FIFO_DURATION * SAMPLE_SIZE *ai->channels );
									   
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
	PaError err;
	int written;
	
	/* Sleep for half the length of the FIFO */
	while (sfifo_space( &fifo ) < len ) {
		usleep( (FIFO_DURATION/2) * 1000000 );
	}
	
	/* Write the audio to the ring buffer */
	written = sfifo_write( &fifo, buf, len );

	/* Start stream if not ative */
	err = Pa_StreamActive( pa_stream );
	if (err == 0) {
		err = Pa_StartStream( pa_stream );
		if( err != paNoError ) {
			error1("Failed to start PortAudio stream: %s", Pa_GetErrorText( err ));
			return -1; /* triggering exit here is not good, better handle that somehow... */
		}
	} else if (err < 0) {
		error1("Failed to check state of PortAudio stream: %s", Pa_GetErrorText( err ));
		return -1;
	}
	
	return written;
}

int audio_close(struct audio_info_struct *ai)
{
	PaError err;
	
	if (pa_stream) {
		/* stop the stream if it is active */
		if (Pa_StreamActive( pa_stream ) == 1) {
			err = Pa_StopStream( pa_stream );
			if( err != paNoError ) {
				fprintf(stderr, "Failed to stop PortAudio stream: %s", Pa_GetErrorText( err ));
				return -1;
			}
		}
	
		/* and then close the stream */
		err = Pa_CloseStream( pa_stream );
		if( err != paNoError ) {
			fprintf(stderr, "Failed to close PortAudio stream: %s", Pa_GetErrorText( err ));
			return -1;
		}
		
		pa_stream = NULL;
	}
	
	/* and free memory used by fifo */
	sfifo_close( &fifo );
    
	return 0;
}

void audio_queueflush(struct audio_info_struct *ai)
{
	PaError err;
	
	/* throw away contents of FIFO */
	sfifo_flush( &fifo );
	
	/* and empty out PortAudio buffers */
	err = Pa_AbortStream( pa_stream );
	
}

