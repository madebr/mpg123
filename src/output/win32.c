/*
	win32: audio output for Windows 32bit

	copyright ?-2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written (as it seems) by Tony Million
*/

#include <fcntl.h>
#include <windows.h>

#include "mpg123.h"


/* FIXME: these should be in a structure, not globals */
static CRITICAL_SECTION cs;
static HWAVEOUT dev   = NULL;
static int nBlocks    = 0;
static int MAX_BLOCKS = 6;


static void wait(void)
{
	while(nBlocks) Sleep(77);
}


static void CALLBACK wave_callback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	WAVEHDR *wh;
	HGLOBAL hg;

	if(uMsg == WOM_DONE)
	{
		debug("one block done");
		EnterCriticalSection(&cs);
		debug("in critical section");

		wh = (WAVEHDR *)dwParam1;

		waveOutUnprepareHeader(dev, wh, sizeof (WAVEHDR));

		/* Deallocate the buffer memory */
		hg = GlobalHandle(wh->lpData);
		GlobalUnlock(hg);
		GlobalFree(hg);

		/* Deallocate the header memory */
		hg = GlobalHandle(wh);
		GlobalUnlock(hg);
		GlobalFree(hg);

		/* decrease the number of USED blocks */
		nBlocks--;

		LeaveCriticalSection(&cs);
		debug1("down to %i blocks", nBlocks);
	}
	else debug1("got message %u != WOM_DONE", uMsg);
}

static int open_win32(struct audio_info_struct *ai)
{
	MMRESULT res;
	WAVEFORMATEX outFormatex;

	if(ai->rate == -1)
	return 0;

	if(!waveOutGetNumDevs())
	{
		MessageBox(NULL, "No audio devices present!", "Error...", MB_OK);
		return -1;
	}

	outFormatex.wFormatTag      = WAVE_FORMAT_PCM;
	outFormatex.wBitsPerSample  = 16;
	outFormatex.nChannels       = 2;
	outFormatex.nSamplesPerSec  = ai->rate;
	outFormatex.nAvgBytesPerSec = outFormatex.nSamplesPerSec * outFormatex.nChannels * outFormatex.wBitsPerSample/8;
	outFormatex.nBlockAlign     = outFormatex.nChannels * outFormatex.wBitsPerSample/8;

	res = waveOutOpen(&dev, (UINT)ai->device, &outFormatex, (DWORD)wave_callback, 0, CALLBACK_FUNCTION);
	if(res != MMSYSERR_NOERROR)
	{
		switch(res)
		{
			case MMSYSERR_ALLOCATED:
				MessageBox(NULL, "Device Is Already Open", "Error...", MB_OK);
			break;
			case MMSYSERR_BADDEVICEID:
				MessageBox(NULL, "The Specified Device Is out of range", "Error...", MB_OK);
			break;
			case MMSYSERR_NODRIVER:
				MessageBox(NULL, "There is no audio driver in this system.", "Error...", MB_OK);
			break;
			case MMSYSERR_NOMEM:
				MessageBox(NULL, "Unable to allocate sound memory.", "Error...", MB_OK);
			break;
			case WAVERR_BADFORMAT:
				MessageBox(NULL, "This audio format is not supported.", "Error...", MB_OK);
			break;
			case WAVERR_SYNC:
				MessageBox(NULL, "The device is synchronous.", "Error...", MB_OK);
			break;
			default:
				MessageBox(NULL, "Unknown Media Error", "Error...", MB_OK);
			break;
		}
		return -1;
	}

	waveOutReset(dev);
	InitializeCriticalSection(&cs);

	return 0;
}

static int get_formats_win32(struct audio_info_struct *ai)
{
	return AUDIO_FORMAT_SIGNED_16;
}

static int write_win32(struct audio_info_struct *ai,unsigned char *buf,int len)
{
	HGLOBAL hg, hg2;
	LPWAVEHDR wh;
	MMRESULT res;
	void *b;

	/*  Wait for a few FREE blocks... */
	while(nBlocks > MAX_BLOCKS)
	{
		debug2("waiting for free blocks... %i > %i", nBlocks, MAX_BLOCKS);
		Sleep(77);
	}
	/* FIRST allocate some memory for a copy of the buffer! */
	/* Isn't that blatantly inefficient to allocate and free memory all the time??
	   We should make a fixed ringbuffer set here. */
	hg2 = GlobalAlloc(GMEM_MOVEABLE, len);
	if(!hg2)
	{
		MessageBox(NULL, "GlobalAlloc failed!", "Error...",  MB_OK);
		return(-1);
	}
	b = GlobalLock(hg2);

	/* Here we can call any modification output functions we want.... */
	CopyMemory(b, buf, len);

	/* now make a header and WRITE IT! */
	hg = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(WAVEHDR));
	if(!hg) return -1;

	wh = GlobalLock(hg);
	wh->dwBufferLength = len;
	wh->lpData = b;

	debug("going to push prepared block");

	res = waveOutPrepareHeader(dev, wh, sizeof(WAVEHDR));
	if(res)
	{
		error("could not prepare header");
		GlobalUnlock(hg);
		GlobalFree(hg);
		/* LeaveCriticalSection(&cs); */
		return -1;
	}
	debug("prepared header");
	/* Hm, this causes deadlock when in critical section... I guess it handles locking in itself */
	res = waveOutWrite(dev, wh, sizeof(WAVEHDR));
	if(res)
	{
		error("could not write");
		GlobalUnlock(hg);
		GlobalFree(hg);
		/* LeaveCriticalSection(&cs); */
		return -1;
	}
	debug("wrote block");
	EnterCriticalSection(&cs);
	debug("in critical section");
	nBlocks++; /* we need lock around _that_ */

	LeaveCriticalSection(&cs);
	debug("done");

	return len;
}

static int close_win32(struct audio_info_struct *ai)
{
	if(dev)
	{
		wait();
		waveOutReset(dev);
		waveOutClose(dev);
		dev=NULL;
	}
	DeleteCriticalSection(&cs);
	nBlocks = 0;
	return 0;
}

static void flush_win32(struct audio_info_struct *ai)
{
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
	/* description */	"Audio output for Windows.",
	/* revision */		"$Rev:$",						
	/* handle */		NULL,
	
	/* init_output */	init_win32,						
};


