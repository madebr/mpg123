/*
	audio: audio output interface

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123app.h"
#include "common.h"

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

static void capline(mpg123_handle *mh, long rate)
{
	int enci;
	const int  *encs;
	size_t      num_encs;
	mpg123_encodings(&encs, &num_encs);
	fprintf(stderr," %5ld  |", rate);
	for(enci=0; enci<num_encs; ++enci)
	{
		switch(mpg123_format_support(mh, rate, encs[enci]))
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
	const long *rates;
	size_t      num_rates;
	const int  *encs;
	size_t      num_encs;
	mpg123_rates(&rates, &num_rates);
	mpg123_encodings(&encs, &num_encs);
	fprintf(stderr,"\nAudio driver: %s\nAudio device: %s\nAudio capabilities:\n(matrix of [S]tereo or [M]ono support for sample format and rate in Hz)\n        |",
	        ao->module->name, ao->device != NULL ? ao->device : "<none>");
	for(e=0;e<num_encs;e++) fprintf(stderr," %5s |",audio_encoding_name(encs[e], 0));
	fprintf(stderr,"\n --------------------------------------------------------\n");
	for(r=0; r<num_rates; ++r) capline(mh, rates[r]);

	if(param.force_rate) capline(mh, param.force_rate);

	fprintf(stderr,"\n");
}

void audio_capabilities(audio_output_t *ao, mpg123_handle *mh)
{
	int fmts;
	int ri;
	audio_output_t ao1 = *ao; /* a copy */
	const long *rates;
	size_t      num_rates;
	mpg123_rates(&rates, &num_rates);

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
		for(ri = param.force_rate>0 ? -1 : 0;ri<num_rates;ri++)
		{
			ao1.rate = ri >= 0 ? rates[ri] : param.force_rate;
			fmts = ao1.get_formats(&ao1);
			if(fmts < 0) continue;
			else mpg123_format(mh, ao1.rate, ao1.channels, fmts);
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

int init_output(audio_output_t *ao, mpg123_handle *mh)
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
			error("cannot fork!");
			return -1;
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

/* is this used? */
void close_output(int outmode, audio_output_t *ao)
{

    switch(outmode) {
      case DECODE_AUDIO:
        ao->close(ao);
        /* Module frees and closes its resources, but may not reset them. */
        ao->userptr = NULL;
        ao->fn = -1;
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

/* Also for WAV decoding? */
int reset_output(audio_output_t *ao)
{
	if(param.outmode == DECODE_AUDIO)
	{
		close_output(param.outmode, ao);
		return ao->open(ao);
	}
	else return 0;
}
