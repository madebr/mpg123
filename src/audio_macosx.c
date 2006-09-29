/*
	audio_macosx: audio output on MacOS X

	copyright ?-2006 by the mpg123 project - free software under the terms of the GPL 2
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Guillaume Outters
	modified by Nicholas J Humfrey to use SFIFO code
*/


#include "config.h"
#include "debug.h"
#include "sfifo.h"
#include "mpg123.h"

#include <CoreAudio/AudioHardware.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define FIFO_DURATION		(0.5f)


struct anEnv
{
	AudioDeviceID device;
	char play;
	
	/* Convertion buffer */
	float * buffer;
	size_t buffer_size;
	
	/* Ring buffer */
	sfifo_t fifo;
};

static struct anEnv *env=NULL;



static OSStatus playProc(AudioDeviceID inDevice, const AudioTimeStamp * inNow,
						 const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime,
                         AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime, void * inClientData)
{
	
	long n;
	
	for(n = 0; n < outOutputData->mNumberBuffers; n++)
	{
		unsigned int wanted = outOutputData->mBuffers[n].mDataByteSize;
		unsigned char *dest = outOutputData->mBuffers[n].mData;
		unsigned int read;
		
		/* Only play if we have data left */
		if ( sfifo_used( &env->fifo ) < wanted ) {
			warning("Didn't have any audio data in callback (buffer underflow)");
			return -1;
		}
		
		/* Read audio from FIFO to SDL's buffer */
		read = sfifo_read( &env->fifo, dest, wanted );
		
		if (wanted!=read)
			warning2("Error reading from the ring buffer (wanted=%u, read=%u).\n", wanted, read);
		
	}
	
	return (0); 
}



int audio_open(struct audio_info_struct *ai)
{
	AudioStreamBasicDescription format;
	Float64 devicerate;
	UInt32 size;
	
	/* Allocate memory for data structure */
	if (!env) {
		env = (struct anEnv*)malloc( sizeof( struct anEnv ) );
		if (!env) {
			error("failed to malloc memory for 'struct anEnv'");
			return -1;
		}
	}

	/* Initialize our environment */
	env->device = 0;
	env->play = 0;
	env->buffer = NULL;
	env->buffer_size = 0;
	

	
	/* Get the default audio output device */
	size = sizeof(env->device);
	if(AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &env->device)) {
		error("AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice) failed");
		return(-1);
	}
	
	/* Ensure that the device supports PCM */
	size = sizeof(format);
	if(AudioDeviceGetProperty(env->device, 0, 0, kAudioDevicePropertyStreamFormat, &size, &format)) {
		error("AudioDeviceGetProperty(kAudioDevicePropertyStreamFormat) failed");
		return(-1);
	}
	if(format.mFormatID != kAudioFormatLinearPCM) {
		error("format.mFormatID != kAudioFormatLinearPCM");
		return(-1);
	}
	
	/* Get the nominal sample rate of the device */
	size = sizeof(devicerate);
	if(AudioDeviceGetProperty(env->device, 0, 0, kAudioDevicePropertyNominalSampleRate, &size, &devicerate)) {
		error("AudioDeviceGetProperty(kAudioDevicePropertyNominalSampleRate) failed");
		return(-1);
	}
		
	/* Add our callback - but don't start it yet */
	if(AudioDeviceAddIOProc(env->device, playProc, env)) {
		error("AudioDeviceAddIOProc failed");
		return(-1);
	}
	
	
	/* Open an audio I/O stream. */
	if (ai->rate > 0 && ai->channels >0 ) {
		int ringbuffer_len;
		
		/* Check sample rate */
		if (devicerate != ai->rate) {
			error2("Error: sample rate of device doesn't match playback rate (%d != %d)", (int)devicerate, (int)ai->rate);
			return(-1);
		}
		
		/* Initialise FIFO */
		ringbuffer_len = ai->rate * FIFO_DURATION * sizeof(float) *ai->channels;
		debug2( "Allocating %d byte ring-buffer (%f seconds)", ringbuffer_len, (float)FIFO_DURATION);
		sfifo_init( &env->fifo, ringbuffer_len );
									   
	}
	
	return(0);
}


int audio_get_formats(struct audio_info_struct *ai)
{
	/* Only support Signed 16-bit output */
	return AUDIO_FORMAT_SIGNED_16;
}


int audio_play_samples(struct audio_info_struct *ai, unsigned char *buf, int len)
{
	short *src = (short *)buf;
	int samples = len/sizeof(short);
	int flen = samples*sizeof(float);
	int written, n;

	/* If there is no room, then sleep for half the length of the FIFO */
	while (sfifo_space( &env->fifo ) < flen ) {
		usleep( (FIFO_DURATION/2) * 1000000 );
	}

	/* Ensure conversion buffer is big enough */
	if (env->buffer_size < flen) {
		debug1("Allocating %d byte sample conversion buffer", flen);
		env->buffer = realloc( env->buffer, flen);
		env->buffer_size = flen;
	}
	
	/* Convert audio samples to 32-bit float */
	for( n=0; n<samples; n++) {
		env->buffer[n] = src[n] / 32768.0f;
	}
	
	/* Store converted audio in ring buffer */
	written = sfifo_write( &env->fifo, (char*)env->buffer, flen);
	if (written != flen) {
		warning( "Failed to write audio to ring buffer" );
		return -1;
	}
	
	/* Start playback now that we have something to play */
	if(!env->play)
	{
		if(AudioDeviceStart(env->device, playProc)) {
			error("AudioDeviceStart failed");
			return(-1);
		}
		env->play = 1;
	}
	
	return len;
}

int audio_close(struct audio_info_struct *ai)
{

	if (env) {
		/* No matter the error code, we want to close it (by brute force if necessary) */
		AudioDeviceStop(env->device, playProc);
		AudioDeviceRemoveIOProc(env->device, playProc);
	
	    /* Free the ring buffer */
		sfifo_close( &env->fifo );
		
		/* Free the conversion buffer */
		if (env->buffer) free( env->buffer );
		
		/* Free environment data structure */
		free(env);
		env=NULL;
	}
	
	return 0;
}

void audio_queueflush(struct audio_info_struct *ai)
{

	/* Stop playback */
	if(AudioDeviceStop(env->device, playProc)) {
		error("AudioDeviceStop failed");
	}
	env->play=0;
	
	/* Empty out the ring buffer */
	sfifo_flush( &env->fifo );	
}
