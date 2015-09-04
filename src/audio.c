/*
	audio: audio output interface

	copyright ?-2015 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include <errno.h>
#include "mpg123app.h"
#include "out123.h"
#include "common.h"
#include "sysutil.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "debug.h"

struct enc_desc
{
	int code; /* MPG123_ENC_SOMETHING */
	const char *longname; /* signed bla bla */
	const char *name; /* sXX, short name */
	const unsigned char nlen; /* significant characters in short name */
};

static const struct enc_desc encdesc[] =
{
	{ MPG123_ENC_SIGNED_16, "signed 16 bit", "s16 ", 3 },
	{ MPG123_ENC_UNSIGNED_16, "unsigned 16 bit", "u16 ", 3 },
	{ MPG123_ENC_UNSIGNED_8, "unsigned 8 bit", "u8  ", 2 },
	{ MPG123_ENC_SIGNED_8, "signed 8 bit", "s8  ", 2 },
	{ MPG123_ENC_ULAW_8, "mu-law (8 bit)", "ulaw ", 4 },
	{ MPG123_ENC_ALAW_8, "a-law (8 bit)", "alaw ", 4 },
	{ MPG123_ENC_FLOAT_32, "float (32 bit)", "f32 ", 3 },
	{ MPG123_ENC_SIGNED_32, "signed 32 bit", "s32 ", 3 },
	{ MPG123_ENC_UNSIGNED_32, "unsigned 32 bit", "u32 ", 3 },
	{ MPG123_ENC_SIGNED_24, "signed 24 bit", "s24 ", 3 },
	{ MPG123_ENC_UNSIGNED_24, "unsigned 24 bit", "u24 ", 3 }
};
#define KNOWN_ENCS (sizeof(encdesc)/sizeof(struct enc_desc))

int audio_enc_name2code(const char* name)
{
	int code = 0;
	int i;
	for(i=0;i<KNOWN_ENCS;++i)
	if(!strncasecmp(encdesc[i].name, name, encdesc[i].nlen))
	{
		code = encdesc[i].code;
		break;
	}
	return code;
}

void audio_enclist(char** list)
{
	size_t length = 0;
	int i;
	*list = NULL;
	for(i=0;i<KNOWN_ENCS;++i) length += encdesc[i].nlen;

	length += KNOWN_ENCS-1; /* spaces between the encodings */
	*list = malloc(length+1); /* plus zero */
	if(*list != NULL)
	{
		size_t off = 0;
		(*list)[length] = 0;
		for(i=0;i<KNOWN_ENCS;++i)
		{
			if(i>0) (*list)[off++] = ' ';
			memcpy(*list+off, encdesc[i].name, encdesc[i].nlen);
			off += encdesc[i].nlen;
		}
	}
}

/* Safer as function... */
const char* audio_encoding_name(const int encoding, const int longer)
{
	const char *name = longer ? "unknown" : "???";
	int i;
	for(i=0;i<KNOWN_ENCS;++i)
	if(encdesc[i].code == encoding)
	name = longer ? encdesc[i].longname : encdesc[i].name;

	return name;
}


static void capline(mpg123_handle *mh, long rate)
{
	int enci;
	const int  *encs;
	size_t      num_encs;
	mpg123_encodings(&encs, &num_encs);
	fprintf(stderr," %5ld |", pitch_rate(rate));
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
	char *name;
	char *dev;
	out123_driver_info(ao, &name, &dev);
	mpg123_rates(&rates, &num_rates);
	mpg123_encodings(&encs, &num_encs);
	fprintf(stderr,"\nAudio driver: %s\nAudio device: %s\nAudio capabilities:\n(matrix of [S]tereo or [M]ono support for sample format and rate in Hz)\n       |", name, dev);
	for(e=0;e<num_encs;e++) fprintf(stderr," %5s |",audio_encoding_name(encs[e], 0));

	fprintf(stderr,"\n ------|");
	for(e=0;e<num_encs;e++) fprintf(stderr,"-------|");

	fprintf(stderr, "\n");
	for(r=0; r<num_rates; ++r) capline(mh, rates[r]);

	if(param.force_rate) capline(mh, param.force_rate);

	fprintf(stderr,"\n");
}

/* This uses the currently opened audio device, queries its caps.
   In case of buffered playback, this works _once_ by querying the buffer for the caps before entering the main loop. */
void audio_capabilities(audio_output_t *ao, mpg123_handle *mh)
{
	int force_fmt = 0;
	int fmts;
	size_t ri;
	/* Pitching introduces a difference between decoder rate and playback rate. */
	long rate, decode_rate;
	int channels;
	const long *rates;
	size_t      num_rates, rlimit;
	debug("audio_capabilities");
	mpg123_rates(&rates, &num_rates);
	mpg123_format_none(mh); /* Start with nothing. */
	if(param.force_encoding != NULL)
	{
		if(!param.quiet) fprintf(stderr, "Note: forcing output encoding %s\n", param.force_encoding);

		force_fmt = audio_enc_name2code(param.force_encoding);
		if(!force_fmt)
		{
			error1("Failed to find an encoding to match requested \"%s\"!\n", param.force_encoding);
			return; /* No capabilities at all... */
		}
		else if(param.verbose > 2) fprintf(stderr, "Note: forcing encoding code 0x%x\n", force_fmt);
	}
	rlimit = param.force_rate > 0 ? num_rates+1 : num_rates;
	for(channels=1; channels<=2; channels++)
	for(ri = 0;ri<rlimit;ri++)
	{
		decode_rate = ri < num_rates ? rates[ri] : param.force_rate;
		rate = pitch_rate(decode_rate);
		if(param.verbose > 2) fprintf(stderr, "Note: checking support for %liHz/%ich.\n", rate, channels);

		fmts = out123_encodings(ao, channels, rate);

		if(param.verbose > 2) fprintf(stderr, "Note: result 0x%x\n", fmts);
		if(force_fmt)
		{ /* Filter for forced encoding. */
			if((fmts & force_fmt) == force_fmt) fmts = force_fmt;
			else fmts = 0; /* Nothing else! */

			if(param.verbose > 2) fprintf(stderr, "Note: after forcing 0x%x\n", fmts);
		}

		if(fmts < 0) continue;
		else mpg123_format(mh, decode_rate, channels, fmts);
	}

	if(param.verbose > 1) print_capabilities(ao, mh);
}

int set_pitch(mpg123_handle *fr, audio_output_t *ao, double new_pitch)
{
	double old_pitch = param.pitch;
	long rate;
	int channels, format;
	int smode = 0;

	/* Be safe, check support. */
	if(mpg123_getformat(fr, &rate, &channels, &format) != MPG123_OK)
	{
		/* We might just not have a track handy. */
		error("There is no current audio format, cannot apply pitch. This might get fixed in future.");
		return 0;
	}

	param.pitch = new_pitch;
	if(param.pitch < -0.99) param.pitch = -0.99;

	if(channels == 1) smode = MPG123_MONO;
	if(channels == 2) smode = MPG123_STEREO;

	out123_stop(ao);
	/* Remember: This takes param.pitch into account. */
	audio_capabilities(ao, fr);
	if(!(mpg123_format_support(fr, rate, format) & smode))
	{
		/* Note: When using --pitch command line parameter, you can go higher
		   because a lower decoder sample rate is automagically chosen.
		   Here, we'd need to switch decoder rate during track... good? */
		error("Reached a hardware limit there with pitch!");
		param.pitch = old_pitch;
		audio_capabilities(ao, fr);
	}
	return out123_start(ao, pitch_rate(rate), channels, format);
}
