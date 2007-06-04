/*
	audio_nas: audio output via NAS

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Martin Denn
*/

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <audio/audiolib.h>
#include <audio/soundlib.h>

#include "mpg123.h"

typedef struct
{
    AuServer            *aud;
    AuFlowID            flow;
    AuDeviceAttributes  *da;
    int                 numDevices;
    char                *buf;
    AuUint32            buf_size;
    AuUint32            buf_cnt;
    AuBool              data_sent;
    AuBool              finished;
} InfoRec, *InfoPtr;

#define NAS_SOUND_PORT_DURATION 5 /* seconds */
#define NAS_SOUND_LOW_WATER_MARK 25 /* percent */
#define NAS_MAX_FORMAT 10 /* currently, there are 7 supported formats */

static InfoRec info;

/* NAS specific routines */

static void
nas_sendData(AuServer *aud, InfoPtr i, AuUint32 numBytes)
{
    if (numBytes < i->buf_cnt) {
        AuWriteElement(aud, i->flow, 0, numBytes, i->buf, AuFalse, NULL);
        memmove(i->buf, i->buf + numBytes, i->buf_cnt - numBytes);
        i->buf_cnt = i->buf_cnt - numBytes;
    }
    else {
         AuWriteElement(aud, i->flow, 0, i->buf_cnt, i->buf,
                        (numBytes > i->buf_cnt), NULL);
         i->buf_cnt = 0;
    }
    i->data_sent = AuTrue;
}

static AuBool
nas_eventHandler(AuServer *aud, AuEvent *ev, AuEventHandlerRec *handler)
{
    InfoPtr         i = (InfoPtr) handler->data;

    switch (ev->type)
    {
        case AuEventTypeMonitorNotify:
            i->finished = AuTrue;
            break;
       case AuEventTypeElementNotify:
           {
               AuElementNotifyEvent *event = (AuElementNotifyEvent *) ev;

               switch (event->kind)
               {
                   case AuElementNotifyKindLowWater:
                       nas_sendData(aud, i, event->num_bytes);
                       break;
                   case AuElementNotifyKindState:
                       switch (event->cur_state)
                       {
                           case AuStatePause:
                               if (event->reason != AuReasonUser)
                                   nas_sendData(aud, i, event->num_bytes);
                               break;
                            case AuStateStop:
                                i->finished = AuTrue;
                                break;
                       }
               }
           }
    }
    return AuTrue;
}

/* 0 on error */
int nas_createFlow(struct audio_info_struct *ai)
{
    AuDeviceID      device = AuNone;
    AuElement       elements[2];
    unsigned char   format;
    AuUint32        buf_samples;
    int             i;
 

    switch(ai->format) {
    case AUDIO_FORMAT_SIGNED_16:
    default:
		if (((char) *(short *)"x")=='x') /* ugly, but painless */
			format = AuFormatLinearSigned16LSB; /* little endian */
		else
		format = AuFormatLinearSigned16MSB; /* big endian */
        break;
    case AUDIO_FORMAT_UNSIGNED_8:
        format = AuFormatLinearUnsigned8;
        break;
    case AUDIO_FORMAT_SIGNED_8:
        format = AuFormatLinearSigned8;
        break;
    case AUDIO_FORMAT_ULAW_8:
        format = AuFormatULAW8;
        break;
    }
    /* look for an output device */
    for (i = 0; i < AuServerNumDevices(info.aud); i++)
       if (((AuDeviceKind(AuServerDevice(info.aud, i)) ==
              AuComponentKindPhysicalOutput) &&
             AuDeviceNumTracks(AuServerDevice(info.aud, i))
             ==  ai->channels )) {
            device = AuDeviceIdentifier(AuServerDevice(info.aud, i));
            break;
       }
    if (device == AuNone) {
       error1("Couldn't find an output device providing %d channels.", ai->channels);
       return 0;
    }

    /* set gain */
    if(ai->gain >= 0) {
        info.da = AuGetDeviceAttributes(info.aud, device, NULL);
        if ((info.da)!=NULL) {
            AuDeviceGain(info.da) = AuFixedPointFromSum(ai->gain, 0);
            AuSetDeviceAttributes(info.aud, AuDeviceIdentifier(info.da),
                                  AuCompDeviceGainMask, info.da, NULL);
        }
        else
            error("audio/gain: setable Volume/PCM-Level not supported");
    }
    
    if (!(info.flow = AuCreateFlow(info.aud, NULL))) {
        error("Couldn't create flow");
        return 0;
    }

    buf_samples = ai->rate * NAS_SOUND_PORT_DURATION;

    AuMakeElementImportClient(&elements[0],        /* element */
                              (unsigned short) ai->rate,
                                                   /* rate */
                              format,              /* format */
                              ai->channels,        /* channels */
                              AuTrue,              /* ??? */
                              buf_samples,         /* max samples */
                              (AuUint32) (buf_samples / 100
                                  * NAS_SOUND_LOW_WATER_MARK),
                                                   /* low water mark */
                              0,                   /* num actions */
                              NULL);               /* actions */
    AuMakeElementExportDevice(&elements[1],        /* element */
                              0,                   /* input */
                              device,              /* device */
                              (unsigned short) ai->rate,
                                                   /* rate */
                              AuUnlimitedSamples,  /* num samples */
                              0,                   /* num actions */
                              NULL);               /* actions */
    AuSetElements(info.aud,                        /* Au server */
                  info.flow,                       /* flow ID */
                  AuTrue,                          /* clocked */
                  2,                               /* num elements */
                  elements,                        /* elements */
                  NULL);                           /* return status */

    AuRegisterEventHandler(info.aud,               /* Au server */
                           AuEventHandlerIDMask,   /* value mask */
                           0,                      /* type */
                           info.flow,              /* id */
                           nas_eventHandler,       /* callback */
                           (AuPointer) &info);     /* data */

    info.buf_size = buf_samples * ai->channels * AuSizeofFormat(format);
    info.buf = (char *) malloc(info.buf_size);
    if (info.buf == NULL) {
        error1("Unable to allocate input/output buffer of size %ld",
             info.buf_size);
        return 0;
    }
    info.buf_cnt = 0;
    info.data_sent = AuFalse;
    info.finished = AuFalse;
    
    AuStartFlow(info.aud,                          /* Au server */
                info.flow,                         /* id */
                NULL);                             /* status */
    return 1; /* success */
}


void nas_flush()
{
    AuEvent         ev;
    
    while ((!info.data_sent) && (!info.finished)) {
        AuNextEvent(info.aud, AuTrue, &ev);
        AuDispatchEvent(info.aud, &ev);
    }
    info.data_sent = AuFalse;
}

/* required functions */

/* returning -1 on error, 0 on success... */
int audio_open(struct audio_info_struct *ai)
{
    if(!ai)
        return -1;

    if (!(info.aud = AuOpenServer(ai->device, 0, NULL, 0, NULL, NULL))) {
        if (ai->device==NULL)
            error("could not open default NAS server");
        else
            error1("could not open NAS server %s\n", ai->device);
        return -1;
    }
    info.buf_size = 0;
        
    return 0;
}


int audio_get_formats(struct audio_info_struct *ai)
{
    int i, j, k, ret;

    ret=0;
    j = AuServerNumFormats(info.aud);
    for (i=0; i<j; i++) {
        k=AuServerFormat(info.aud,i);
        switch (k)
        {
        case AuFormatULAW8:
            ret |= AUDIO_FORMAT_ULAW_8;
            break;
        case AuFormatLinearUnsigned8:
            ret |= AUDIO_FORMAT_UNSIGNED_8;
            break;
        case AuFormatLinearSigned8:
            ret |= AUDIO_FORMAT_SIGNED_8;
            break;
        case AuFormatLinearSigned16LSB:
            ret |= AUDIO_FORMAT_SIGNED_16;
            break;
        }
    }
    return ret;
}

int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
    int buf_cnt = 0;

    if (info.buf_size == 0)
    if(!nas_createFlow(ai)) return -1;
    
    while ((info.buf_cnt + (len - buf_cnt)) >  info.buf_size) {
        memcpy(info.buf + info.buf_cnt,
               buf + buf_cnt,
               (info.buf_size - info.buf_cnt));
        buf_cnt += (info.buf_size - info.buf_cnt);
        info.buf_cnt += (info.buf_size - info.buf_cnt);
        nas_flush();
    }
    memcpy(info.buf + info.buf_cnt,
           buf + buf_cnt,
           (len - buf_cnt));
    info.buf_cnt += (len - buf_cnt);
    
    return len;
}

int audio_close(struct audio_info_struct *ai)
{
    if (info.aud == NULL) {
        return 0;
    }
    
    if (info.buf_size == 0) {
        /* Au server opened, but not yet initialized */
        AuCloseServer(info.aud);
        return 0;
    }
        
    while (!info.finished) {
        nas_flush();
    }
    AuCloseServer(info.aud);
    free(info.buf);
    
    return 0;
}

void audio_queueflush(struct audio_info_struct *ai)
{
}
