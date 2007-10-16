/*
	audio: audio output interface

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123app.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif


/* Open an audio output module */
audio_output_t* open_output_module( const char* name )
{
	mpg123_module_t *module = NULL;
	audio_output_t *ao = NULL;
	int result = 0;

	/* Open the module */
	module = open_module( "output", name );
	if (module == NULL) return NULL;

	/* Check module supports output */
	if (module->init_output == NULL) {
		error1("Module '%s' does not support audio output.", name);
		close_module( module );
		return NULL;
	}
	
	/* Allocation memory for audio output type */
	ao = alloc_audio_output();
	if (ao==NULL) {
		error( "Failed to allocate audio output structure." );
		return NULL;
	}
	
	/* Call the init function */
	ao->device = param.output_device;
	ao->flags  = param.output_flags;
	result = module->init_output(ao);
	if (result) {
		error1( "Module's init function failed: %d", result );
		close_module( module );
		return NULL;
	}
	
	/* Store the pointer to the module (so we can close it later) */
	ao->module = module;

	return ao;
}



/* Close the audio output and close the module */
void close_output_module( audio_output_t* ao ) 
{
	if (!ao) return;
	
	debug("closing output module");

	/* Close the audio output */
	if (ao->close) ao->close( ao );

	/* Deinitialise the audio output */
	if (ao->deinit) ao->deinit( ao );
	
	/* Unload the module */
	if (ao->module) close_module( ao->module );

	/* Free up memory */
	free( ao );
}



/* allocate and initialise memory */
audio_output_t* alloc_audio_output()
{
	audio_output_t* ao = malloc( sizeof( audio_output_t ) );
	if (ao==NULL) error( "Failed to allocate memory for audio_output_t." );

	/* Initialise variables */
	ao->fn = -1;
	ao->rate = -1;
	ao->gain = -1;
	ao->userptr = NULL;
	ao->device = NULL;
	ao->channels = -1;
	ao->format = -1;
	ao->flags = 0;
	
	/*ao->module = NULL;*/

	/* Set the callbacks to NULL */
	ao->open = NULL;
	ao->get_formats = NULL;
	ao->write = NULL;
	ao->flush = NULL;
	ao->close = NULL;
	ao->deinit = NULL;
	
	return ao;
}

/*
static void audio_output_dump(audio_output_t *ao)
{
	fprintf(stderr, "ao->fn=%d\n", ao->fn);
	fprintf(stderr, "ao->userptr=%p\n", ao->userptr);
	fprintf(stderr, "ao->rate=%ld\n", ao->rate);
	fprintf(stderr, "ao->gain=%ld\n", ao->gain);
	fprintf(stderr, "ao->device='%s'\n", ao->device);
	fprintf(stderr, "ao->channels=%d\n", ao->channels);
	fprintf(stderr, "ao->format=%d\n", ao->format);
}
*/


#define NUM_CHANNELS 2
#define NUM_ENCODINGS 6
#define NUM_RATES 10

/* Safer as function... */
const char* audio_encoding_name(const int encoding, const int longer)
{
	const char *name = longer ? "unknown" : "???";
	switch(encoding)
	{
		case MPG123_ENC_SIGNED_16:   name = longer ? "signed 16 bit"   : "s16 ";  break;
		case MPG123_ENC_UNSIGNED_16: name = longer ? "unsigned 16 bit" : "u16 ";  break;
		case MPG123_ENC_UNSIGNED_8:  name = longer ? "unsigned 8 bit"  : "u8  ";   break;
		case MPG123_ENC_SIGNED_8:    name = longer ? "signed 8 bit"    : "s8  ";   break;
		case MPG123_ENC_ULAW_8:      name = longer ? "mu-law (8 bit)"  : "ulaw "; break;
		case MPG123_ENC_ALAW_8:      name = longer ? "a-law (8 bit)"   : "alaw "; break;
	}
	return name;
}

static int channels[NUM_CHANNELS] = { 1 , 2 };
static int rates[NUM_RATES] = { 
	8000, 11025, 12000, 
	16000, 22050, 24000,
	32000, 44100, 48000,
	8000	/* 8000 = dummy for user forced */
};

static int encodings[NUM_ENCODINGS] = {
	AUDIO_FORMAT_SIGNED_16, 
	AUDIO_FORMAT_UNSIGNED_16,
	AUDIO_FORMAT_UNSIGNED_8,
	AUDIO_FORMAT_SIGNED_8,
	AUDIO_FORMAT_ULAW_8,
	AUDIO_FORMAT_ALAW_8
};

static void capline(mpg123_handle *mh, int ratei)
{
	int enci;
	fprintf(stderr," %5ld  |", ratei >= 0 ? mpg123_rates[ratei] : param.force_rate);
	for(enci=0; enci<MPG123_ENCODINGS; ++enci)
	{
		switch(mpg123_format_support(mh, ratei, enci))
		{
			case MPG123_MONO:               fprintf(stderr, "   M   |"); break;
			case MPG123_STEREO:             fprintf(stderr, "   S   |"); break;
			case MPG123_MONO|MPG123_STEREO: fprintf(stderr, "  M/S  |"); break;
			default:                        fprintf(stderr, "       |");
		}
	}
	fprintf(stderr, "\n");
}

void print_capabilities(audio_output_t *ao, mpg123_handle *mh)
{
	int r,e;
	fprintf(stderr,"\nAudio driver: %s\nAudio device: %s\nAudio capabilities:\n(matrix of [S]tereo or [M]ono support for sample format and rate in Hz)\n        |",
	        ao->module->name, ao->device != NULL ? ao->device : "<none>");
	for(e=0;e<MPG123_ENCODINGS;e++) fprintf(stderr," %5s |",audio_encoding_name(mpg123_encodings[e], 0));
	fprintf(stderr,"\n --------------------------------------------------------\n");
	for(r=0; r<MPG123_RATES; ++r) capline(mh, r);

	if(param.force_rate) capline(mh, -1);

	fprintf(stderr,"\n");
}

LIB: void audio_capabilities(struct audio_info_struct *ai, mpg123_handle *mh)
{
	int fmts;
	int ri;
	audio_output_t ao1 = *ao; /* a copy */

	if(mpg123_param(mh, MPG123_FORCE_RATE, param.force_rate, 0) != MPG123_OK)
	{
		error1("Cannot set forced rate (%s)!", mpg123_strerror(mh));
		mpg123_format_none(mh);
		return;
	}
	if(param.outmode != DECODE_AUDIO)
	{ /* File/stdout writers can take anything. */
		mpg123_format_all(mh);
		return;
	}

	mpg123_format_none(mh); /* Start with nothing. */

	/* If audio_open fails, the device is just not capable of anything... */
	if(ao1.open(&ao1) < 0) error("failed to open audio device");
	else
	{
		for(ao1.channels=1; ao1.channels<=2; ao1.channels++)
		for(ri=-1;ri<MPG123_RATES;ri++)
		{
			ao1.rate = ri >= 0 ? mpg123_rates[ri] : param.force_rate;
			fmts = ao1.get_formats(&ao1);
			if(fmts < 0) continue;
			else mpg123_format(mh, ri, ao1.channels, fmts);
		}
		ao1.close(&ao1);
	}

	if(param.verbose > 1) print_capabilities(ao, mh);
}

#if !defined(WIN32) && !defined(GENERIC)
#ifndef NOXFERMEM
static void catch_child(void)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);
}
#endif
#endif


/* FIXME: Old output initialization code that needs updating */

int init_output(audio_output_t *ao)
{
	static int init_done = FALSE;
	
	if (init_done) return 1;
	init_done = TRUE;
  
#ifndef NOXFERMEM
	/*
	* Only DECODE_AUDIO and DECODE_FILE are sanely handled by the
	* buffer process. For now, we just ignore the request
	* to buffer the output. [dk]
	*/
	if (param.usebuffer && (param.outmode != DECODE_AUDIO) &&
	(param.outmode != DECODE_FILE)) {
	fprintf(stderr, "Sorry, won't buffer output unless writing plain audio.\n");
	param.usebuffer = 0;
	} 

	if (param.usebuffer)
	{
		unsigned int bufferbytes;
		sigset_t newsigset, oldsigset;
		bufferbytes = (param.usebuffer * 1024);
		if (bufferbytes < bufferblock)
		{
			bufferbytes = 2*bufferblock;
			if(!param.quiet) fprintf(stderr, "Note: raising buffer to minimal size %liKiB\n", (unsigned long) bufferbytes>>10);
		}
		bufferbytes -= bufferbytes % bufferblock;
		/* No +1024 for NtoM rounding problems anymore! */
		xfermem_init (&buffermem, bufferbytes ,0,0);
		mpg123_replace_buffer(mh, (unsigned char *) buffermem->data, bufferblock);
		sigemptyset (&newsigset);
		sigaddset (&newsigset, SIGUSR1);
		sigprocmask (SIG_BLOCK, &newsigset, &oldsigset);
#if !defined(WIN32) && !defined(GENERIC)
		catchsignal (SIGCHLD, catch_child);
#endif
		switch ((buffer_pid = fork()))
		{
			case -1: /* error */
			perror("fork()");
			safe_exit(1);
			case 0: /* child */
			/* oh, is that trouble here? well, buffer should actually be opened before loading tracks IMHO */
			mpg123_close(mh); /* child doesn't need the input stream */
			xfermem_init_reader (buffermem);
			buffer_loop (ao, &oldsigset);
			xfermem_done_reader (buffermem);
			xfermem_done (buffermem);
			exit(0);
			default: /* parent */
			xfermem_init_writer (buffermem);
			param.outmode = DECODE_BUFFER;
		}
	}
#endif
	/* Open audio if not decoding to buffer */
	switch(param.outmode) {
		case DECODE_AUDIO:
			if(ao->open(ao) < 0) {
				error("failed to open audio device");
				return 1;
			}
		break;
		case DECODE_WAV:
			wav_open(ao,param.filename);
		break;
		case DECODE_AU:
			au_open(ao,param.filename);
		break;
		case DECODE_CDR:
			cdr_open(ao,param.filename);
		break;
	}

	return 0;
}

void flush_output(int outmode, audio_output_t *ao, unsigned char *bytes, size_t count)
{
	if(count)
	{
		switch(outmode)
		{
			case DECODE_FILE:
				write (OutputDescriptor, bytes, count);
			break;
			case DECODE_AUDIO:
				ao->write(ao, bytes, count);
			break;
			case DECODE_BUFFER:
				error("The buffer doesn't work like that... I shouldn't ever be getting here.");
				write (buffer_fd[1], bytes, count);
			break;
			case DECODE_WAV:
			case DECODE_CDR:
			case DECODE_AU:
				wav_write(bytes, count);
			break;
		}
		count = 0;
	}
}

void close_output(int outmode, audio_output_t *ao)
{

    switch(outmode) {
      case DECODE_AUDIO:
        ao->close(ao);
        break;
      case DECODE_WAV:
        wav_close();
        break;
      case DECODE_AU:
        au_close();
        break;
      case DECODE_CDR:
        cdr_close();
        break;
    }

}
