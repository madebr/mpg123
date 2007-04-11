/*
	audio_macosx: audio output on MacOS X

	copyright ?-2006 by the mpg123 project - free software under the terms of the GPL 2
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Guillaume Outters
	modified by Nicholas J Humfrey to use SFIFO code
	modified by Taihei Monma to use AudioUnit and AudioConverter APIs
*/


#include "config.h"
#include "debug.h"
#include "sfifo.h"
#include "mpg123.h"

#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define FIFO_DURATION		(0.5f)


struct anEnv
{
	AudioConverterRef converter;
	AudioUnit outputUnit;
	char play;
	int channels;
	int last_buffer;
	int play_done;
	int decode_done;
	
	/* Convertion buffer */
	unsigned char * buffer;
	size_t buffer_size;
	
	/* Ring buffer */
	sfifo_t fifo;
};

static struct anEnv *env=NULL;



static OSStatus playProc(AudioConverterRef inAudioConverter,
						 UInt32 *ioNumberDataPackets,
                         AudioBufferList *outOutputData,
                         AudioStreamPacketDescription **outDataPacketDescription,
                         void* inClientData)
{
	long n;
	
	if(env->last_buffer) {
		env->play_done = 1;
		return noErr;
	}
	
	for(n = 0; n < outOutputData->mNumberBuffers; n++)
	{
		unsigned int wanted = *ioNumberDataPackets * env->channels * 2;
		unsigned char *dest;
		unsigned int read;
		if(env->buffer_size < wanted) {
			debug1("Allocating %d byte sample conversion buffer", wanted);
			env->buffer = realloc( env->buffer, wanted);
			env->buffer_size = wanted;
		}
		dest = env->buffer;
		
		/* Only play if we have data left */
		if ( sfifo_used( &env->fifo ) < wanted ) {
			if(!env->decode_done) {
				warning("Didn't have any audio data in callback (buffer underflow)");
				return -1;
			}
			wanted = sfifo_used( &env->fifo );
			env->last_buffer = 1;
		}
		
		/* Read audio from FIFO to SDL's buffer */
		read = sfifo_read( &env->fifo, dest, wanted );
		
		if (wanted!=read)
			warning2("Error reading from the ring buffer (wanted=%u, read=%u).\n", wanted, read);
		
		outOutputData->mBuffers[n].mDataByteSize = read;
		outOutputData->mBuffers[n].mData = dest;
	}
	
	return noErr; 
}

static OSStatus convertProc(void *inRefCon, AudioUnitRenderActionFlags *inActionFlags,
                            const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                            UInt32 inNumFrames, AudioBufferList *ioData)
{
	OSStatus err= noErr;
	void *inInputDataProcUserData=NULL;
	AudioStreamPacketDescription* outPacketDescription =NULL;
	
	err = AudioConverterFillComplexBuffer(env->converter, playProc, inInputDataProcUserData, &inNumFrames, ioData, outPacketDescription);
	
	return err;
}


int audio_open(struct audio_info_struct *ai)
{
	UInt32 size;
	ComponentDescription desc;
	Component comp;
	AudioStreamBasicDescription inFormat;
	AudioStreamBasicDescription outFormat;
	AURenderCallbackStruct  renderCallback;
	Boolean outWritable;
	
	/* Allocate memory for data structure */
	if (!env) {
		env = (struct anEnv*)malloc( sizeof( struct anEnv ) );
		if (!env) {
			error("failed to malloc memory for 'struct anEnv'");
			return -1;
		}
	}

	/* Initialize our environment */
	env->play = 0;
	env->buffer = NULL;
	env->buffer_size = 0;
	env->last_buffer = 0;
	env->play_done = 0;
	env->decode_done = 0;

	
	/* Get the default audio output unit */
	desc.componentType = kAudioUnitType_Output; 
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	comp = FindNextComponent(NULL, &desc);
	if(comp == NULL) {
		error("FindNextComponent failed");
		return(-1);
	}
	
	if(OpenAComponent(comp, &(env->outputUnit)))  {
		error("OpenAComponent failed");
		return (-1);
	}
	
	if(AudioUnitInitialize(env->outputUnit)) {
		error("AudioUnitInitialize failed");
		return (-1);
	}
	
	/* Specify the output PCM format */
	AudioUnitGetPropertyInfo(env->outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &size, &outWritable);
	if(AudioUnitGetProperty(env->outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &outFormat, &size)) {
		error("AudioUnitGetProperty(kAudioUnitProperty_StreamFormat) failed");
		return (-1);
	}
	
	if(AudioUnitSetProperty(env->outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &outFormat, size)) {
		error("AudioUnitSetProperty(kAudioUnitProperty_StreamFormat) failed");
		return (-1);
	}
	
	/* Specify the input PCM format */
	env->channels = ai->channels;
	inFormat.mSampleRate = ai->rate;
	inFormat.mChannelsPerFrame = ai->channels;
	inFormat.mBitsPerChannel = 16;
	inFormat.mBytesPerPacket = 2*inFormat.mChannelsPerFrame;
	inFormat.mFramesPerPacket = 1;
	inFormat.mBytesPerFrame = 2*inFormat.mChannelsPerFrame;
	inFormat.mFormatID = kAudioFormatLinearPCM;
#ifdef _BIG_ENDIAN
	inFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked | kLinearPCMFormatFlagIsBigEndian;
#else
	inFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
#endif
	
		
	/* Add our callback - but don't start it yet */
	memset(&renderCallback, 0, sizeof(AURenderCallbackStruct));
	renderCallback.inputProc = convertProc;
	renderCallback.inputProcRefCon = 0;
	if(AudioUnitSetProperty(env->outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &renderCallback, sizeof(AURenderCallbackStruct))) {
		error("AudioUnitSetProperty(kAudioUnitProperty_SetRenderCallback) failed");
		return(-1);
	}
	
	
	/* Open an audio I/O stream and create converter */
	if (ai->rate > 0 && ai->channels >0 ) {
		int ringbuffer_len;

		if(AudioConverterNew(&inFormat, &outFormat, &(env->converter))) {
			error("AudioConverterNew failed");
			return(-1);
		}
		if(ai->channels == 1) {
			SInt32 channelMap[2] = { 0, 0 };
			if(AudioConverterSetProperty(env->converter, kAudioConverterChannelMap, sizeof(channelMap), channelMap)) {
				error("AudioConverterSetProperty(kAudioConverterChannelMap) failed");
				return(-1);
			}
		}
		
		/* Initialise FIFO */
		ringbuffer_len = ai->rate * FIFO_DURATION * sizeof(short) *ai->channels;
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
	int written;

	/* If there is no room, then sleep for half the length of the FIFO */
	while (sfifo_space( &env->fifo ) < len ) {
		usleep( (FIFO_DURATION/2) * 1000000 );
	}
	
	/* Store converted audio in ring buffer */
	written = sfifo_write( &env->fifo, (char*)buf, len);
	if (written != len) {
		warning( "Failed to write audio to ring buffer" );
		return -1;
	}
	
	/* Start playback now that we have something to play */
	if(!env->play)
	{
		if(AudioOutputUnitStart(env->outputUnit)) {
			error("AudioOutputUnitStart failed");
			return(-1);
		}
		env->play = 1;
	}
	
	return len;
}

int audio_close(struct audio_info_struct *ai)
{
	if (env) {
		env->decode_done = 1;
		while(!env->play_done && env->play) usleep(10000);
		
		/* No matter the error code, we want to close it (by brute force if necessary) */
		AudioConverterDispose(env->converter);
		AudioOutputUnitStop(env->outputUnit);
		AudioUnitUninitialize(env->outputUnit);
		CloseComponent(env->outputUnit);
	
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
	if(AudioOutputUnitStop(env->outputUnit)) {
		error("AudioOutputUnitStop failed");
	}
	env->play=0;
	
	/* Empty out the ring buffer */
	sfifo_flush( &env->fifo );	
}
