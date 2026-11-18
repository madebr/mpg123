/*
    os2_kai: audio output for OS/2 KAI

    copyright 1998-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
    see COPYING and AUTHORS files in distribution or http://mpg123.org
    initially written by KO Myung-Hun
*/

#include "../out123_int.h"

#define INCL_DOS
#include <os2.h>

#include <kai.h>
#include <stdlib.h>

/* Including the sfifo code locally, to avoid module linkage issues. */
#define SFIFO_STATIC
#include "../sfifo.c"

#include "../../common/debug.h"

#define FRAME_SIZE 2048

struct kai_instance
{
    HKAI hkai;
    sfifo_t fifo;
    HEV hev;
};

static ULONG APIENTRY kai_callback(PVOID cbdata, PVOID buffer, ULONG size )
{
    struct kai_instance *kai = cbdata;
    int len = sfifo_used(&kai->fifo);

    if (len > size)
        len = size;

    sfifo_read(&kai->fifo, buffer, len);
    if (len < size)
        memset((char *)buffer + len, 0, size - len);

    DosPostEventSem(kai->hev);

    return size;
}

int open_kai(out123_handle *ao)
{
    KAISPEC w, o;
    struct kai_instance *kai = ao->userptr;

    if(!ao) return -1;
    if(ao->rate == -1) return 0;
    if(ao->channels != 1 && ao->channels != 2) return -1;

    switch(ao->format)
    {
        case MPG123_ENC_UNSIGNED_8:
            w.ulBitsPerSample = 8;
            break;

        case MPG123_ENC_SIGNED_16:
            w.ulBitsPerSample = 16;
            break;

        default:
            return -1;
    }

    if (DosCreateEventSem(NULL, &kai->hev, 0, FALSE))
        return -1;

    w.usDeviceIndex    = 0;
    w.ulType           = KAIT_PLAY;
    w.ulSamplingRate   = ao->rate;
    w.ulDataFormat     = 0;
    w.ulChannels       = ao->channels;
    w.ulNumBuffers     = 2;
    w.ulBufferSize     = (w.ulBitsPerSample >> 3) * FRAME_SIZE * w.ulChannels;
    w.fShareable       = TRUE;
    w.pfnCallBack      = kai_callback;
    w.pCallBackData    = ao->userptr;

    if(kaiOpen(&w, &o, &kai->hkai))
    {
        mdebug("kaiOpen() failed!\n");
        return -1;
    }

    sfifo_init(&kai->fifo, o.ulBufferSize * o.ulNumBuffers * 2);

    ao->userptr = kai;

    kaiPlay(kai->hkai);

    return 0;
}

static int get_formats_kai(out123_handle *ao)
{
    /* Support S16 only */
    return MPG123_ENC_SIGNED_16;
}

static int write_kai(out123_handle *ao, unsigned char *buf, int len)
{
    struct kai_instance *kai;
    int l, w;
    ULONG post;

    if (!ao) return -1;

    kai = ao->userptr;

    w = 0;
    for (;;)
    {
        l = sfifo_space(&kai->fifo);
        if (l > len)
            l = len;

        sfifo_write(&kai->fifo, buf + w, l);
        w += l;
        len -= l;

        if (len == 0)
            break;

        DosResetEventSem(kai->hev, &post);
        DosWaitEventSem(kai->hev, SEM_INDEFINITE_WAIT);
    }

    return w;
}

static void flush_kai(out123_handle *ao)
{
    if(ao && ao->userptr)
    {
        struct kai_instance *kai = ao->userptr;

        sfifo_flush(&kai->fifo);
    }
}

static void drain_kai(out123_handle *ao)
{
    struct kai_instance *kai;

    if(!ao)
        return;

    kai = ao->userptr;
    while(sfifo_used(&kai->fifo) > 0)
        _sleep2(1);
}


static int close_kai(out123_handle *ao)
{
    if(ao && ao->userptr)
    {
        struct kai_instance *kai = ao->userptr;

        if (kai->hkai)
        {
            kaiClose(kai->hkai);
            kai->hkai = NULLHANDLE;

            sfifo_close(&kai->fifo);
            DosCloseEventSem(kai->hev);
        }
    }

    return 0;
}

static void deinit_kai(out123_handle *ao)
{
    if(ao == NULL) return;

    free(ao->userptr);
    ao->userptr = NULL;

    kaiDone();
}

static int init_kai(out123_handle *ao)
{
    if (ao==NULL) return -1;

    if (kaiInit(KAIM_AUTO)) return -1;

    ao->userptr = malloc(sizeof(struct kai_instance));
    if (ao->userptr == NULL) return -1;

    /* Set callbacks */
    ao->open = open_kai;
    ao->get_formats = get_formats_kai;
    ao->write = write_kai;
    ao->flush = flush_kai;
    ao->drain = drain_kai;
    ao->close = close_kai;
    ao->deinit = deinit_kai;

    /* Success */
    return 0;
}



/*
    Module information data structure
*/
mpg123_module_t mpg123_output_module_info = {
    /* api_version */   MPG123_MODULE_API_VERSION,
    /* name */          "os2_kai",
    /* description */   "Audio output for OS2 KAI.",
    /* revision */      "$Rev:$",
    /* handle */        NULL,

    /* init_output */   init_kai,
};
