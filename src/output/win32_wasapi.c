/*
	win32_wasapi: audio output for Windows wasapi exclusive mode audio

	copyright ?-2013 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	based on win32.c
*/
#define COBJMACROS 1
#define _WIN32_WINNT 0x601
#include <initguid.h>
#include "mpg123app.h"
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#include "debug.h"

/* Push mode does not work right yet, noisy audio, probably something to do with timing and buffers */
#define WASAPI_EVENT_MODE 1
#ifdef WASAPI_EVENT_MODE
#define Init_Flag AUDCLNT_STREAMFLAGS_EVENTCALLBACK
#define MOD_STRING "Experimental Audio output for Windows (wasapi event mode)."
#define BUFFER_TIME 20000000.0
#else
#define Init_Flag 0
#define MOD_STRING "Experimental Audio output for Windows (wasapi push mode)."
#define BUFFER_TIME 640000000.0
#endif

static int init_win32(audio_output_t* ao);
static void flush_win32(struct audio_output_struct *ao);
/* 
	Module information data structure
*/
mpg123_module_t mpg123_output_module_info = {
	/* api_version */	MPG123_MODULE_API_VERSION,
	/* name */			"win32_wasapi",						
	/* description */	MOD_STRING,
	/* revision */		"$Rev:$",						
	/* handle */		NULL,
	
	/* init_output */	init_win32,
};

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

/* todo: move into handle struct */
static IMMDeviceEnumerator *pEnumerator = NULL;
static IMMDevice *pDevice = NULL;
static IAudioClient *pAudioClient = NULL;
static IAudioRenderClient *pRenderClient = NULL;
static UINT32 bufferFrameCount;
static REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
static HANDLE hEvent = NULL;
static HANDLE hTask = NULL;
static BYTE *pData = NULL;
static size_t pData_off = 0;
static int is_playing = 0;
static DWORD taskIndex = 0;

/* setup endpoints */
static int open_win32(struct audio_output_struct *ao){
  HRESULT hr = 0;

  debug1("%s",__FUNCTION__);
  CoInitialize(NULL);
  hr = CoCreateInstance(&CLSID_MMDeviceEnumerator,NULL,CLSCTX_ALL, &IID_IMMDeviceEnumerator,(void**)&pEnumerator);
  debug("CoCreateInstance");
  EXIT_ON_ERROR(hr)

  hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(pEnumerator,eRender, eConsole, &pDevice);
  debug("IMMDeviceEnumerator_GetDefaultAudioEndpoint");
  EXIT_ON_ERROR(hr)

  hr = IMMDeviceActivator_Activate(pDevice,
                  &IID_IAudioClient, CLSCTX_ALL,
                  NULL, (void**)&pAudioClient);
  debug("IMMDeviceActivator_Activate");
  EXIT_ON_ERROR(hr)

  return 0;
  Exit:
  debug2("%s failed with %lx", __FUNCTION__, hr);
  return 1;
}

/* check supported formats */
static int get_formats_win32(struct audio_output_struct *ao){
  /* PLEASE check with write_init and write_win32 buffer size calculation in case it is able to support something other than 16bit */
  HRESULT hr;
  int ret = 0;
  debug1("%s",__FUNCTION__);
  debug2("channels %d\nrate %ld",ao->channels, ao->rate);
  WAVEFORMATEX s16 = {
    WAVE_FORMAT_PCM,
	ao->channels,
	ao->rate,
	ao->channels * 2 * ao->rate,
	ao->channels * 2,
	16,
	0
  };

  if((hr = IAudioClient_IsFormatSupported(pAudioClient,AUDCLNT_SHAREMODE_EXCLUSIVE, &s16, NULL)) == S_OK)
    ret |= MPG123_ENC_SIGNED_16;
  /*s16.nBlockAlign = ao->channels * 4;
  s16.wBitsPerSample = 32;
  if((hr = IAudioClient_IsFormatSupported(pAudioClient,AUDCLNT_SHAREMODE_EXCLUSIVE, &s16, NULL)) == S_OK)
    ret |= MPG123_ENC_SIGNED_32;
  s16.nBlockAlign = ao->channels * 1;
  s16.wBitsPerSample = 8;
  if((hr = IAudioClient_IsFormatSupported(pAudioClient,AUDCLNT_SHAREMODE_EXCLUSIVE, &s16, NULL)) == S_OK)
    ret |= MPG123_ENC_SIGNED_8;
  */

  return ret; /* afaik only 16bit has been known to work */
}

/* setup with agreed on format, for now only MPG123_ENC_SIGNED_16 */
static int write_init(struct audio_output_struct *ao){
  HRESULT hr;
  double offset = 0.5;
  WAVEFORMATEX s16 = {
    WAVE_FORMAT_PCM,
	ao->channels,
	ao->rate,
	ao->channels * 2 * ao->rate,
	ao->channels * 2, /* 16bit/8 */
	16,
	0
  };
  debug1("%s",__FUNCTION__);
  /* cargo cult code */
  hr = IAudioClient_GetDevicePeriod(pAudioClient,NULL, &hnsRequestedDuration);
  debug("IAudioClient_GetDevicePeriod OK");
  reinit:
  hr = IAudioClient_Initialize(pAudioClient,
                       AUDCLNT_SHAREMODE_EXCLUSIVE,
                       Init_Flag,
                       hnsRequestedDuration,
                       hnsRequestedDuration,
                       &s16,
                       NULL);
  debug("IAudioClient_Initialize OK");
  /* something about buffer sizes on Win7, fixme might loop forever */
  if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED){
    if (offset > 10.0) goto Exit; /* is 10 enough to break out of the loop?*/
    debug("AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED");
    IAudioClient_GetBufferSize(pAudioClient,&bufferFrameCount);
    /* double buffered */
	hnsRequestedDuration = (REFERENCE_TIME)((BUFFER_TIME / s16.nSamplesPerSec * bufferFrameCount) + offset);
    offset += 0.5;
	IAudioClient_Release(pAudioClient);
	pAudioClient = NULL;
	hr = IMMDeviceActivator_Activate(pDevice,
                  &IID_IAudioClient, CLSCTX_ALL,
                  NULL, (void**)&pAudioClient);
    debug("IMMDeviceActivator_Activate");
    goto reinit;
  }
  EXIT_ON_ERROR(hr)
  EXIT_ON_ERROR(hr)
  hr = IAudioClient_GetService(pAudioClient,
                        &IID_IAudioRenderClient,
                        (void**)&pRenderClient);
  debug("IAudioClient_GetService OK");
  EXIT_ON_ERROR(hr)
  hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  debug("CreateEvent OK");
  if(!hEvent) goto Exit;
  hr = IAudioClient_SetEventHandle(pAudioClient,hEvent);
  EXIT_ON_ERROR(hr);
  hr = IAudioClient_GetBufferSize(pAudioClient,&bufferFrameCount);
  debug("IAudioClient_GetBufferSize OK");
  EXIT_ON_ERROR(hr)
  return 0;
Exit:
  debug2("%s failed with %lx", __FUNCTION__, hr);
  return 1;
}

/* Set play mode if unset, also raise thread priority */
static HRESULT play_init(){
  HRESULT hr = S_OK;
  if(!is_playing){
    debug1("%s",__FUNCTION__);
    hTask = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);
    hr = IAudioClient_Start(pAudioClient);
    is_playing = 1;
    debug("IAudioClient_Start");
    EXIT_ON_ERROR(hr)
    }
  return hr;
Exit:
  debug2("%s failed with %lx", __FUNCTION__, hr);
  return hr;
}

/* copy audio into IAudioRenderClient provided buffer */
static int write_win32(struct audio_output_struct *ao, unsigned char *buf, int len){
  HRESULT hr;
  size_t to_copy = 0;
  size_t framesize = ao->channels * 2; /* 16bit/8 */
  size_t frames_in = len/framesize; /* Frames in buf, is framesize even correct? */
  debug1("%s",__FUNCTION__);
  if(!len) return 0;
  if(!pRenderClient) write_init(ao);
#ifdef WASAPI_EVENT_MODE
  /* Event mode WASAPI */
  DWORD retval = -1;
  int flag = 0; /* Silence flag */
  feed_again:
  if(!pData){
    /* Acquire buffer */
    hr = IAudioRenderClient_GetBuffer(pRenderClient,bufferFrameCount, &pData);
    debug("IAudioRenderClient_GetBuffer");
    EXIT_ON_ERROR(hr)
  }
  if(frames_in){ /* Did we get half a frame?? non-zero len smaller than framesize? */
    /* We must put in exactly the amount of frames specified by IAudioRenderClient_GetBuffer */
    while(pData_off < bufferFrameCount){
      to_copy = bufferFrameCount - pData_off;
      debug3("pData_off %I64d, bufferFrameCount %d, to_copy %I64d", pData_off, bufferFrameCount, to_copy);
      if(to_copy > frames_in){
        /* buf can fit in provided buffer space */
        debug1("all buffers copied, %I64d", frames_in);
        memcpy(pData+pData_off*framesize,buf,framesize*(frames_in));
        pData_off += frames_in;
        frames_in = 0;
        break;
      } else {
        /* buf too big, needs spliting */
        debug1("partial buffers %I64d", to_copy);
        memcpy(pData+pData_off*framesize,buf,framesize*(to_copy));
        pData_off += to_copy;
        buf+=(to_copy*framesize);
        frames_in -= to_copy;
      }
    }
  } else {
    /* In case we ever get half a frame, is it possible? */
    flag = AUDCLNT_BUFFERFLAGS_SILENT;
  }
  debug2("Copied %I64d, left %I64d", pData_off, frames_in);
  if(pData_off == bufferFrameCount) {
    /* Tell IAudioRenderClient that buffer is filled and released */
    hr = IAudioRenderClient_ReleaseBuffer(pRenderClient,pData_off, flag);
    pData_off = 0;
    pData = NULL;
    debug("IAudioRenderClient_ReleaseBuffer");
    EXIT_ON_ERROR(hr)
    hr = play_init();
    EXIT_ON_ERROR(hr)
    /* wait for next pull event */
    retval = WaitForSingleObject(hEvent, 2000);
    if (retval != WAIT_OBJECT_0){
      /* Event handle timed out after a 2-second wait, something went very wrong */
      IAudioClient_Stop(pAudioClient);
      hr = ERROR_TIMEOUT;
      goto Exit;
    }
  }
  if(frames_in > 0)
    goto feed_again;
#else /* PUSH mode code */
    UINT32 numFramesAvailable, numFramesPadding, bufferFrameCount;
feed_again:
    /* How much buffer do we get to use? */
    hr = IAudioClient_GetBufferSize(pAudioClient,&bufferFrameCount);
    debug("IAudioRenderClient_GetBuffer");
    EXIT_ON_ERROR(hr)
    hr = IAudioClient_GetCurrentPadding(pAudioClient,&numFramesPadding);
    debug("IAudioClient_GetCurrentPadding");
    EXIT_ON_ERROR(hr)
    /* How much buffer is writable at the moment? */
    numFramesAvailable = bufferFrameCount - numFramesPadding;
    debug3("numFramesAvailable %d, bufferFrameCount %d, numFramesPadding %d", numFramesAvailable, bufferFrameCount, numFramesPadding);
    if(numFramesAvailable > frames_in){
      /* can fit all frames now */
      pData_off = 0;
      to_copy = frames_in;
    } else {
      /* copy whatever that fits in the buffer */
      pData_off = frames_in - numFramesAvailable;
      to_copy = numFramesAvailable;
    }
    /* Acquire buffer */
    hr = IAudioRenderClient_GetBuffer(pRenderClient,to_copy,&pData);
    debug("IAudioRenderClient_GetBuffer");
    EXIT_ON_ERROR(hr)
    memcpy(pData,buf+pData_off*framesize,to_copy*framesize);
    /* Release buffer */
    hr = IAudioRenderClient_ReleaseBuffer(pRenderClient,to_copy, 0);
    debug("IAudioRenderClient_ReleaseBuffer");
    EXIT_ON_ERROR(hr)
    hr = play_init();
    EXIT_ON_ERROR(hr)
    frames_in -= to_copy;
    /* Wait sometime for buffer to empty? */
    Sleep((DWORD)(hnsRequestedDuration/REFTIMES_PER_MILLISEC));
    if (frames_in)
      goto feed_again;
#endif
  return len;
  Exit:
  debug2("%s failed with %lx", __FUNCTION__, hr);
  return -1;
}

static void flush_win32(struct audio_output_struct *ao){
  /* Wait for the last buffer to play before stopping. */
  HRESULT hr;
  debug1("%s",__FUNCTION__);
  if(!pAudioClient) return;
  Sleep((DWORD)(hnsRequestedDuration/REFTIMES_PER_MILLISEC));
  hr = IAudioClient_Stop(pAudioClient);
  EXIT_ON_ERROR(hr)
  return;
  Exit:
  debug2("%s IAudioClient_Stop with %lx", __FUNCTION__, hr);
}

static int close_win32(struct audio_output_struct *ao)
{
  if(pAudioClient) IAudioClient_Stop(pAudioClient);
  if(pRenderClient) IAudioRenderClient_Release(pRenderClient);  
  if(pAudioClient) IAudioClient_Release(pAudioClient);
  if(hTask) AvRevertMmThreadCharacteristics(hTask);
  if(pEnumerator) IMMDeviceEnumerator_Release(pEnumerator);
  if(pDevice) IMMDevice_Release(pDevice);
  CoUninitialize();
  pEnumerator = NULL;
  pDevice = NULL;
  pAudioClient = NULL;
  pRenderClient = NULL;
  bufferFrameCount = 0;
  hnsRequestedDuration = REFTIMES_PER_SEC;
  hEvent = NULL;
  hTask = NULL;
  pData = NULL;
  pData_off = 0;
  is_playing = 0;
  taskIndex = 0;
	return 0;
}

static int init_win32(audio_output_t* ao){
    debug1("%s",__FUNCTION__);
	if(!ao) return -1;

	/* Set callbacks */
	ao->open = open_win32;
	ao->flush = flush_win32;
	ao->write = write_win32;
	ao->get_formats = get_formats_win32;
	ao->close = close_win32;

	/* Success */
	return 0;
}
