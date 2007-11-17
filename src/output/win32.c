/*
	win32: audio output for Windows 32bit

	copyright ?-2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	initially written (as it seems) by Tony Million
	rewrite of basic functionality for callback-less and properly ringbuffered operation by "ravenexp" 
*/

#include "mpg123app.h"
#include <windows.h>

/* Number of buffers in the playback ring */
#define NUM_BUFFERS 64 /* about 1 sec of 44100 sampled stream */

/* Buffer ring queue state */
struct queue_state
{
	WAVEHDR buffer_headers[NUM_BUFFERS];
	/* The next buffer to be filled and put in playback */
	int next_buffer;
	/* Buffer playback completion event */
	HANDLE play_done_event;
};

int open_win32(struct audio_output_struct *ao)
{
	struct queue_state* state;
	MMRESULT res;
	WAVEFORMATEX out_fmt;
	UINT dev_id;

	if(!ao) return -1;

	if(ao->rate == -1) return 0;

	/* Allocate queue state struct for this device */
	state = calloc(1, sizeof(struct queue_state));
	if(!state) return -1;

	ao->userptr = state;
	state->play_done_event = CreateEvent(0,FALSE,FALSE,0);
	if(state->play_done_event == INVALID_HANDLE_VALUE) return -1;

	/* FIXME: real device enumeration by capabilities? */
	dev_id = WAVE_MAPPER;	/* probably does the same thing */
	ao->device = "WaveMapper";
	/* FIXME: support for smth besides MPG123_ENC_SIGNED_16? */
	out_fmt.wFormatTag = WAVE_FORMAT_PCM;
	out_fmt.wBitsPerSample = 16;
	out_fmt.nChannels = 2;
	out_fmt.nSamplesPerSec = ao->rate;
	out_fmt.nBlockAlign = out_fmt.nChannels*out_fmt.wBitsPerSample/8;
	out_fmt.nAvgBytesPerSec = out_fmt.nBlockAlign*out_fmt.nSamplesPerSec;
	out_fmt.cbSize = 0;

	res = waveOutOpen((HWAVEOUT*)&ao->fn, dev_id, &out_fmt,
	                  (DWORD)state->play_done_event, 0, CALLBACK_EVENT);

	switch(res)
	{
		case MMSYSERR_NOERROR:
			break;
		case MMSYSERR_ALLOCATED:
			error("Audio output device is already allocated.");
			return -1;
		case MMSYSERR_NODRIVER:
			error("No device driver is present.");
			return -1;
		case MMSYSERR_NOMEM:
			error("Unable to allocate or lock memory.");
			return -1;
		case WAVERR_BADFORMAT:
			error("Unsupported waveform-audio format.");
			return -1;
		default:
			error("Unable to open wave output device.");
			return -1;
	}
	/* Reset event from the "device open" message */
	ResetEvent(state->play_done_event);
	waveOutReset((HWAVEOUT)ao->fn);
	/* Playback starts when the full queue is prebuffered */
	waveOutPause((HWAVEOUT)ao->fn);
	return 0;
}

int get_formats_win32(struct audio_output_struct *ao)
{
	/* FIXME: support for smth besides MPG123_ENC_SIGNED_16? */
	return MPG123_ENC_SIGNED_16;
}

int write_win32(struct audio_output_struct *ao, unsigned char *buffer, int len)
{
	struct queue_state* state;
	MMRESULT res;
	WAVEHDR* hdr;

	if(!ao || !ao->userptr) return -1;
	if(!buffer || len <= 0) return 0;

	state = (struct queue_state*)ao->userptr;
	/* The last recently used buffer in the play ring */
	hdr = &state->buffer_headers[state->next_buffer];
	/* Check buffer header and wait if it's being played */
	while(hdr->dwFlags & WHDR_INQUEUE)
	{
		/* Looks like queue is full now, can start playing */
		waveOutRestart((HWAVEOUT)ao->fn);
		WaitForSingleObject(state->play_done_event, INFINITE);
		/* BUG: Sometimes event is signaled for some other reason. */
		if(!(hdr->dwFlags & WHDR_DONE))	debug("Audio output device signals something...");
	}
	/* Cleanup */
	if(hdr->dwFlags & WHDR_DONE) waveOutUnprepareHeader((HWAVEOUT)ao->fn, hdr, sizeof(WAVEHDR));

	/* Now have the wave buffer prepared and shove the data in. */
	hdr->dwFlags = 0;	
	/* (Re)allocate buffer only if required.
	   hdr->dwUser = allocated length */
	if(!hdr->lpData || hdr->dwUser < len)
	{
		hdr->lpData = realloc(hdr->lpData, len);
		if(!hdr->lpData){	error("Out of memory for playback buffers.");	return -1; }

		hdr->dwUser = len;
	}
	hdr->dwBufferLength = len;
	memcpy(hdr->lpData, buffer, len);
	res = waveOutPrepareHeader((HWAVEOUT)ao->fn, hdr, sizeof(WAVEHDR));
	if(res != MMSYSERR_NOERROR){ error("Can't write to audio output device (prepare)."); return -1; }

	res = waveOutWrite((HWAVEOUT)ao->fn, hdr, sizeof(WAVEHDR));
	if(res != MMSYSERR_NOERROR){ error("Can't write to audio output device."); return -1; }

	/* Cycle to the next buffer in the ring queue. */
	state->next_buffer = (state->next_buffer + 1) % NUM_BUFFERS;
	return len;
}

void flush_win32(struct audio_output_struct *ao)
{
	int i;
	struct queue_state* state;

	if(!ao || !ao->userptr) return;

	state = (struct queue_state*)ao->userptr;
	waveOutReset((HWAVEOUT)ao->fn);
	ResetEvent(state->play_done_event);
	for(i = 0; i < NUM_BUFFERS; i++)
	{
		if(state->buffer_headers[i].dwFlags & WHDR_DONE)
		waveOutUnprepareHeader((HWAVEOUT)ao->fn, &state->buffer_headers[i], sizeof(WAVEHDR));

		state->buffer_headers[i].dwFlags = 0;	
	}
	waveOutPause((HWAVEOUT)ao->fn);
}

int close_win32(struct audio_output_struct *ao)
{
	int i;
	struct queue_state* state;

	if(!ao || !ao->userptr) return -1;

	state = (struct queue_state*)ao->userptr;
	flush_win32(ao);
	waveOutClose((HWAVEOUT)ao->fn);
	CloseHandle(state->play_done_event);
	for(i = 0; i < NUM_BUFFERS; i++) free(state->buffer_headers[i].lpData);

	free(state);
	ao->userptr = 0;
	return 0;
}

static int init_win32(audio_output_t* ao)
{
	if (ao==NULL) return -1;

	/* Set callbacks */
	ao->open = open_win32;
	ao->flush = flush_win32;
	ao->write = write_win32;
	ao->get_formats = get_formats_win32;
	ao->close = close_win32;

	/* Success */
	return 0;
}





/* 
	Module information data structure
*/
mpg123_module_t mpg123_output_module_info = {
	/* api_version */	MPG123_MODULE_API_VERSION,
	/* name */			"win32",						
	/* description */	"Audio output for Windows (winmm).",
	/* revision */		"$Rev:$",						
	/* handle */		NULL,
	
	/* init_output */	init_win32,						
};


