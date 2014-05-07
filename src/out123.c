/*
	out123: simple program to stream data to an audio output device

	copyright 1995-2014 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis (extracted from mpg123.c)

	This rips out anything dealing with MPEG decoding and playlist handling,
	reducing the functionality to what shall become part of libout123: Simple
	means to get mono/stereo audio to various output devices on various
	platforms. Since this is rather redundant to other approaches (portaudio,
	etc.), the justification stems from two points:

	1. It's code home-grown for mpg123, defining a feature set without resorting
	another build dependency. Throwing the code away would reduce what the mpg123
	code base offers by itself.

	2. It's feature set is a commonly-needed subset of what a full audio
	framework provides. It's what simple playback of mono/stereo data needs.
	Therefore, it has a chance to stay (be?) simple to use, explicitly hiding
	complexity.

	TODO: Decide if the buffer process should be part of it. Might be a good
	idea. It's specific to output and is something tricky to get going yourself.
*/

#define ME "main"
#include "mpg123app.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include <errno.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "getlopt.h"
#include "buffer.h"

#include "debug.h"

static void usage(int err);
static void want_usage(char* arg);
static void long_usage(int err);
static void want_long_usage(char* arg);
static void print_title(FILE* o);
static void give_version(char* arg);

struct parameter param = { 
  FALSE , /* aggressiv */
  FALSE , /* shuffle */
  FALSE , /* remote */
  FALSE , /* remote to stderr */
  DECODE_AUDIO , /* write samples to audio device */
  FALSE , /* silent operation */
  FALSE , /* xterm title on/off */
  0 ,     /* second level buffer size */
  0 ,     /* verbose level */
  DEFAULT_OUTPUT_MODULE,	/* output module */
  NULL,   /* output device */
  0,      /* destination (headphones, ...) */
#ifdef HAVE_TERMIOS
  FALSE , /* term control */
  NULL,
  NULL,
#endif
  FALSE , /* checkrange */
  0 ,	  /* force_reopen, always (re)opens audio device for next song */
  /* test_cpu flag is valid for multi and 3dnow.. even if 3dnow is built alone; ensure it appears only once */
  FALSE , /* normal operation */
  FALSE,  /* try to run process in 'realtime mode' */
#ifdef HAVE_WINDOWS_H 
  0, /* win32 process priority */
#endif
  NULL,  /* wav,cdr,au Filename */
	0, /* default is to play all titles in playlist */
	NULL, /* no playlist per default */
	0 /* condensed id3 per default */
	,0 /* list_cpu */
	,NULL /* cpu */ 
#ifdef FIFO
	,NULL
#endif
	,0 /* timeout */
	,1 /* loop */
	,0 /* delay */
	,0 /* index */
	/* Parameters for mpg123 handle, defaults are queried from library! */
	,0 /* down_sample */
	,0 /* rva */
	,0 /* halfspeed */
	,0 /* doublespeed */
	,0 /* start_frame */
	,-1 /* frame_number */
	,0 /* outscale */
	,0 /* flags */
	,0 /* force_rate */
	,1 /* ICY */
	,1024 /* resync_limit */
	,0 /* smooth */
	,0.0 /* pitch */
	,0 /* appflags */
	,NULL /* proxyurl */
	,0 /* keep_open */
	,0 /* force_utf8 */
	,INDEX_SIZE
	,NULL /* force_encoding */
	,1. /* preload */
	,-1 /* preframes */
	,-1 /* gain */
	,NULL /* stream dump file */
	,0 /* ICY interval */
};

audio_output_t *ao = NULL;
txfermem *buffermem = NULL;
char *prgName = NULL;
/* ThOr: pointers are not TRUE or FALSE */
char *equalfile = NULL;
int fresh = TRUE;
int have_output = FALSE; /* If we are past the output init step. */

int buffer_fd[2];
int buffer_pid;
size_t bufferblock = 4096;

static int intflag = FALSE;
static int skip_tracks = 0;
int OutputDescriptor;

static int filept = -1;

static int network_sockets_used = 0; /* Win32 socket open/close Support */

char *fullprogname = NULL; /* Copy of argv[0]. */
char *binpath; /* Path to myself. */

/* File-global storage of command line arguments.
   They may be needed for cleanup after charset conversion. */
static char **argv = NULL;
static int    argc = 0;

void safe_exit(int code)
{
	char *dummy, *dammy;

	if(have_output) exit_output(ao, intflag);

#ifdef WANT_WIN32_UNICODE
	win32_cmdline_free(argc, argv); /* This handles the premature argv == NULL, too. */
#endif
#if defined (WANT_WIN32_SOCKETS)
	win32_net_deinit();
#endif
	/* It's ugly... but let's just fix this still-reachable memory chunk of static char*. */
	split_dir_file("", &dummy, &dammy);
	if(fullprogname) free(fullprogname);
	exit(code);
}

/* returns 1 if reset_audio needed instead */
static void set_output_module( char *arg )
{
	unsigned int i;
		
	/* Search for a colon and set the device if found */
	for(i=0; i< strlen( arg ); i++) {
		if (arg[i] == ':') {
			arg[i] = 0;
			param.output_device = &arg[i+1];
			debug1("Setting output device: %s", param.output_device);
			break;
		}	
	}
	/* Set the output module */
	param.output_module = arg;
	debug1("Setting output module: %s", param.output_module );
}

static void set_output_flag(int flag)
{
  if(param.output_flags <= 0) param.output_flags = flag;
  else param.output_flags |= flag;
}

static void set_output_h(char *a)
{
	set_output_flag(AUDIO_OUT_HEADPHONES);
}

static void set_output_s(char *a)
{
	set_output_flag(AUDIO_OUT_INTERNAL_SPEAKER);
}

static void set_output_l(char *a)
{
	set_output_flag(AUDIO_OUT_LINE_OUT);
}

static void set_output(char *arg)
{
	/* If single letter, it's the legacy output switch for AIX/HP/Sun.
	   If longer, it's module[:device] . If zero length, it's rubbish. */
	if(strlen(arg) <= 1) switch(arg[0])
	{
		case 'h': set_output_h(arg); break;
		case 's': set_output_s(arg); break;
		case 'l': set_output_l(arg); break;
		default:
			error1("\"%s\" is no valid output", arg);
			safe_exit(1);
	}
	else set_output_module(arg);
}

static void set_verbose (char *arg)
{
    param.verbose++;
}

static void set_quiet (char *arg)
{
	param.verbose=0;
	param.quiet=TRUE;
}

static void set_out_wav(char *arg)
{
	param.outmode = DECODE_WAV;
	param.filename = arg;
}

void set_out_cdr(char *arg)
{
	param.outmode = DECODE_CDR;
	param.filename = arg;
}

void set_out_au(char *arg)
{
  param.outmode = DECODE_AU;
  param.filename = arg;
}

static void set_out_file(char *arg)
{
	param.outmode=DECODE_FILE;
	#ifdef WIN32
	#ifdef WANT_WIN32_UNICODE
	wchar_t *argw = NULL;
	OutputDescriptor = win32_utf8_wide(arg, &argw, NULL);
	if(argw != NULL)
	{
		OutputDescriptor=_wopen(argw,_O_CREAT|_O_WRONLY|_O_BINARY|_O_TRUNC,0666);
		free(argw);
	}
	#else
	OutputDescriptor=_open(arg,_O_CREAT|_O_WRONLY|_O_BINARY|_O_TRUNC,0666);
	#endif /*WANT_WIN32_UNICODE*/
	#else /*WIN32*/
	OutputDescriptor=open(arg,O_CREAT|O_WRONLY|O_TRUNC,0666);
	#endif /*WIN32*/
	if(OutputDescriptor==-1)
	{
		error2("Can't open %s for writing (%s).\n",arg,strerror(errno));
		safe_exit(1);
	}
}

static void set_out_stdout(char *arg)
{
	param.outmode=DECODE_FILE;
	param.remote_err=TRUE;
	OutputDescriptor=STDOUT_FILENO;
	#ifdef WIN32
	_setmode(STDOUT_FILENO, _O_BINARY);
	#endif
}

static void set_out_stdout1(char *arg)
{
	param.outmode=DECODE_AUDIOFILE;
	param.remote_err=TRUE;
	OutputDescriptor=STDOUT_FILENO;
	#ifdef WIN32
	_setmode(STDOUT_FILENO, _O_BINARY);
	#endif
}

static int frameflag; /* ugly, but that's the way without hacking getlopt */
static void set_frameflag(char *arg)
{
	/* Only one mono flag at a time! */
	if(frameflag & MPG123_FORCE_MONO) param.flags &= ~MPG123_FORCE_MONO;
	param.flags |= frameflag;
}
static void unset_frameflag(char *arg)
{
	param.flags &= ~frameflag;
}

static int appflag; /* still ugly, but works */
static void set_appflag(char *arg)
{
	param.appflags |= appflag;
}
/* static void unset_appflag(char *arg)
{
	param.appflags &= ~appflag;
} */

/* Please note: GLO_NUM expects point to LONG! */
/* ThOr:
 *  Yeah, and despite that numerous addresses to int variables were 
passed.
 *  That's not good on my Alpha machine with int=32bit and long=64bit!
 *  Introduced GLO_INT and GLO_LONG as different bits to make that clear.
 *  GLO_NUM no longer exists.
 */
topt opts[] = {
	{'k', "skip",        GLO_ARG | GLO_LONG, 0, &param.start_frame, 0},
	{'2', "2to1",        GLO_INT,  0, &param.down_sample, 1},
	{'4', "4to1",        GLO_INT,  0, &param.down_sample, 2},
	{'t', "test",        GLO_INT,  0, &param.outmode, DECODE_TEST},
	{'s', "stdout",      GLO_INT,  set_out_stdout, &param.outmode, DECODE_FILE},
	{'S', "STDOUT",      GLO_INT,  set_out_stdout1, &param.outmode,DECODE_AUDIOFILE},
	{'O', "outfile",     GLO_ARG | GLO_CHAR, set_out_file, NULL, 0},
	{'c', "check",       GLO_INT,  0, &param.checkrange, TRUE},
	{'v', "verbose",     0,        set_verbose, 0,           0},
	{'q', "quiet",       0,        set_quiet,   0,           0},
	{'y', "no-resync",      GLO_INT,  set_frameflag, &frameflag, MPG123_NO_RESYNC},
	/* compatibility, no-resync is to be used nowadays */
	{0, "resync",      GLO_INT,  set_frameflag, &frameflag, MPG123_NO_RESYNC},
	{'0', "single0",     GLO_INT,  set_frameflag, &frameflag, MPG123_MONO_LEFT},
	{0,   "left",        GLO_INT,  set_frameflag, &frameflag, MPG123_MONO_LEFT},
	{'1', "single1",     GLO_INT,  set_frameflag, &frameflag, MPG123_MONO_RIGHT},
	{0,   "right",       GLO_INT,  set_frameflag, &frameflag, MPG123_MONO_RIGHT},
	{'m', "singlemix",   GLO_INT,  set_frameflag, &frameflag, MPG123_MONO_MIX},
	{0,   "mix",         GLO_INT,  set_frameflag, &frameflag, MPG123_MONO_MIX},
	{0,   "mono",        GLO_INT,  set_frameflag, &frameflag, MPG123_MONO_MIX},
	{0,   "stereo",      GLO_INT,  set_frameflag, &frameflag, MPG123_FORCE_STEREO},
	{0,   "reopen",      GLO_INT,  0, &param.force_reopen, 1},
	{'g', "gain",        GLO_ARG | GLO_LONG, 0, &param.gain,    0},
	{'r', "rate",        GLO_ARG | GLO_LONG, 0, &param.force_rate,  0},
	{0,   "8bit",        GLO_INT,  set_frameflag, &frameflag, MPG123_FORCE_8BIT},
	{0,   "float",       GLO_INT,  set_frameflag, &frameflag, MPG123_FORCE_FLOAT},
	{0,   "headphones",  0,                  set_output_h, 0,0},
	{0,   "speaker",     0,                  set_output_s, 0,0},
	{0,   "lineout",     0,                  set_output_l, 0,0},
	{'o', "output",      GLO_ARG | GLO_CHAR, set_output, 0,  0},
	{0,   "list-modules",0,        list_modules, NULL,  0}, 
	{'a', "audiodevice", GLO_ARG | GLO_CHAR, 0, &param.output_device,  0},
	{'f', "scale",       GLO_ARG | GLO_LONG, 0, &param.outscale,   0},
	{'n', "frames",      GLO_ARG | GLO_LONG, 0, &param.frame_number,  0},
#ifndef NOXFERMEM
	{'b', "buffer",      GLO_ARG | GLO_LONG, 0, &param.usebuffer,  0},
	{0,  "smooth",      GLO_INT,  0, &param.smooth, 1},
#endif
	{'R', "remote",      GLO_INT,  0, &param.remote, TRUE},
	{0,   "remote-err",  GLO_INT,  0, &param.remote_err, TRUE},
	{'d', "doublespeed", GLO_ARG | GLO_LONG, 0, &param.doublespeed, 0},
	{'h', "halfspeed",   GLO_ARG | GLO_LONG, 0, &param.halfspeed, 0},
	{'p', "proxy",       GLO_ARG | GLO_CHAR, 0, &param.proxyurl,   0},
	{'@', "list",        GLO_ARG | GLO_CHAR, 0, &param.listname,   0},
	/* 'z' comes from the the german word 'zufall' (eng: random) */
	{'z', "shuffle",     GLO_INT,  0, &param.shuffle, 1},
	{'Z', "random",      GLO_INT,  0, &param.shuffle, 2},
	{'E', "equalizer",	 GLO_ARG | GLO_CHAR, 0, &equalfile,1},
	{0, "title",         GLO_INT,  0, &param.xterm_title, TRUE },
	{'w', "wav",         GLO_ARG | GLO_CHAR, set_out_wav, 0, 0 },
	{0, "cdr",           GLO_ARG | GLO_CHAR, set_out_cdr, 0, 0 },
	{0, "au",            GLO_ARG | GLO_CHAR, set_out_au, 0, 0 },
	{0,   "gapless",	 GLO_INT,  set_frameflag, &frameflag, MPG123_GAPLESS},
	{0,   "no-gapless", GLO_INT, unset_frameflag, &frameflag, MPG123_GAPLESS},
	{'?', "help",            0,  want_usage, 0,           0 },
	{0 , "longhelp" ,        0,  want_long_usage, 0,      0 },
	{0 , "version" ,         0,  give_version, 0,         0 },
	{'l', "listentry",       GLO_ARG | GLO_LONG, 0, &param.listentry, 0 },
	{0, "continue", GLO_INT, set_appflag, &appflag, MPG123APP_CONTINUE },
	{0, "rva-mix",         GLO_INT,  0, &param.rva, 1 },
	{0, "rva-radio",         GLO_INT,  0, &param.rva, 1 },
	{0, "rva-album",         GLO_INT,  0, &param.rva, 2 },
	{0, "rva-audiophile",         GLO_INT,  0, &param.rva, 2 },
	{0, "no-icy-meta",      GLO_INT,  0, &param.talk_icy, 0 },
	{0, "long-tag",         GLO_INT,  0, &param.long_id3, 1 },
	{0, "timeout", GLO_ARG | GLO_LONG, 0, &param.timeout, 0},
	{0, "loop", GLO_ARG | GLO_LONG, 0, &param.loop, 0},
	{'i', "index", GLO_INT, 0, &param.index, 1},
	{'D', "delay", GLO_ARG | GLO_INT, 0, &param.delay, 0},
	{0, "resync-limit", GLO_ARG | GLO_LONG, 0, &param.resync_limit, 0},
	{0, "pitch", GLO_ARG|GLO_DOUBLE, 0, &param.pitch, 0},
	{0, "ignore-mime", GLO_INT, set_appflag, &appflag, MPG123APP_IGNORE_MIME },
	{0, "lyrics", GLO_INT, set_appflag, &appflag, MPG123APP_LYRICS},
	{0, "keep-open", GLO_INT, 0, &param.keep_open, 1},
	{0, "utf8", GLO_INT, 0, &param.force_utf8, 1},
	{0, "fuzzy", GLO_INT,  set_frameflag, &frameflag, MPG123_FUZZY},
	{0, "index-size", GLO_ARG|GLO_LONG, 0, &param.index_size, 0},
	{0, "no-seekbuffer", GLO_INT, unset_frameflag, &frameflag, MPG123_SEEKBUFFER},
	{'e', "encoding", GLO_ARG|GLO_CHAR, 0, &param.force_encoding, 0},
	{0, "preload", GLO_ARG|GLO_DOUBLE, 0, &param.preload, 0},
	{0, "preframes", GLO_ARG|GLO_LONG, 0, &param.preframes, 0},
	{0, "skip-id3v2", GLO_INT, set_frameflag, &frameflag, MPG123_SKIP_ID3V2},
	{0, "streamdump", GLO_ARG|GLO_CHAR, 0, &param.streamdump, 0},
	{0, "icy-interval", GLO_ARG|GLO_LONG, 0, &param.icy_interval, 0},
	{0, "ignore-streamlength", GLO_INT, set_frameflag, &frameflag, MPG123_IGNORE_STREAMLENGTH},
	{0, 0, 0, 0, 0, 0}
};

/*
 *   Change the playback sample rate.
 *   Consider that changing it after starting playback is not covered by gapless code!
 */
static void reset_audio(long rate, int channels, int format)
{
#ifndef NOXFERMEM
	if (param.usebuffer) {
		/* wait until the buffer is empty,
		 * then tell the buffer process to
		 * change the sample rate.   [OF]
		 */
		while (xfermem_get_usedspace(buffermem)	> 0)
			if (xfermem_block(XF_WRITER, buffermem) == XF_CMD_TERMINATE) {
				intflag = TRUE;
				break;
			}
		buffermem->freeindex = -1;
		buffermem->readindex = 0; /* I know what I'm doing! ;-) */
		buffermem->freeindex = 0;
		if (intflag)
			return;
		buffermem->rate     = pitch_rate(rate); 
		buffermem->channels = channels; 
		buffermem->format   = format;
		buffer_reset();
	}
	else 
	{
#endif
		if(ao == NULL)
		{
			error("Audio handle should not be NULL here!");
			safe_exit(98);
		}
		ao->rate     = pitch_rate(rate); 
		ao->channels = channels; 
		ao->format   = format;
		if(reset_output(ao) < 0)
		{
			error1("failed to reset audio device: %s", strerror(errno));
			safe_exit(1);
		}
#ifndef NOXFERMEM
	}
#endif
}

static FILE* input = NULL;
size_t pcmblock = 1152; /* samples (pcm frames) we treat en bloc */
size_t pcmframe = 4; /* fixed to stereo 16 bit for now */
unsigned char audio[4*1152];

/* return 1 on success, 0 on failure */
int play_frame(void)
{
	size_t got_samples;
	got_samples = fread(audio, pcmframe, pcmblock, input);
	/* Play what is there to play (starting with second decode_frame call!) */
	if(got_samples)
	{
		size_t got_bytes = pcmframe * got_samples;
		if(flush_output(ao, audio, got_bytes) < (int)got_bytes)
		{
			error("Deep trouble! Cannot flush to my output anymore!");
			safe_exit(133);
		}
		return 1;
	}
	else return 0;
}

void buffer_drain(void)
{
#ifndef NOXFERMEM
	int s;
	while ((s = xfermem_get_usedspace(buffermem)))
	{
		struct timeval wait170 = {0, 170000};
		if(intflag) break;
		buffer_ignore_lowmem();
/*		if(param.verbose) print_stat(mh,0,s); */
		select(0, NULL, NULL, NULL, &wait170);
	}
#endif
}

int main(int sys_argc, char ** sys_argv)
{
	int result;
	char end_of_files = FALSE;
	long parr;
	char *fname;
	int libpar = 0;
	mpg123_pars *mp;
#if defined (WANT_WIN32_UNICODE)
	if(win32_cmdline_utf8(&argc, &argv) != 0)
	{
		error("Cannot convert command line to UTF8!");
		safe_exit(76);
	}
#else
	argv = sys_argv;
	argc = sys_argc;
#endif

	if(!(fullprogname = strdup(argv[0])))
	{
		error("OOM"); /* Out Of Memory. Don't waste bytes on that error. */
		safe_exit(1);
	}
	/* Extract binary and path, take stuff before/after last / or \ . */
	if(  (prgName = strrchr(fullprogname, '/')) 
	  || (prgName = strrchr(fullprogname, '\\')))
	{
		/* There is some explicit path. */
		prgName[0] = 0; /* End byte for path. */
		prgName++;
		binpath = fullprogname;
	}
	else
	{
		prgName = fullprogname; /* No path separators there. */
		binpath = NULL; /* No path at all. */
	}

#ifdef OS2
        _wildcard(&argc,&argv);
#endif

	while ((result = getlopt(argc, argv, opts)))
	switch (result) {
		case GLO_UNKNOWN:
			fprintf (stderr, "%s: Unknown option \"%s\".\n", 
				prgName, loptarg);
			usage(1);
		case GLO_NOARG:
			fprintf (stderr, "%s: Missing argument for option \"%s\".\n",
				prgName, loptarg);
			usage(1);
	}

	bufferblock = pcmblock*pcmframe;
	if(init_output(&ao) < 0)
	{
		error("Failed to initialize output, goodbye.");
		return 99; /* It's safe here... nothing nasty happened yet. */
	}
	have_output = TRUE;



	fprintf(stderr, "TODO: Check audio caps, add option to display 'em.\n");
	/* audio_capabilities(ao, mh); */

	reset_audio(44100, 2, MPG123_ENC_SIGNED_16);
	input = stdin;
	while(play_frame())
	{
		// be happy
	}
	/* Ensure we played everything. */
	if(param.usebuffer)
	{
		buffer_drain();
		buffer_resync();
	}

	safe_exit(0); /* That closes output and restores terminal, too. */
	return 0;
}

static void print_title(FILE *o)
{
	fprintf(o, "High Performance MPEG 1.0/2.0/2.5 Audio Player for Layers 1, 2 and 3\n");
	fprintf(o, "\tversion %s; written and copyright by Michael Hipp and others\n", PACKAGE_VERSION);
	fprintf(o, "\tfree software (LGPL) without any warranty but with best wishes\n");
}

static void usage(int err)  /* print syntax & exit */
{
	FILE* o = stdout;
	if(err)
	{
		o = stderr; 
		fprintf(o, "You made some mistake in program usage... let me briefly remind you:\n\n");
	}
	print_title(o);
	fprintf(o,"\nusage: %s [option(s)] [file(s) | URL(s) | -]\n", prgName);
	fprintf(o,"supported options [defaults in brackets]:\n");
	fprintf(o,"   -v    increase verbosity level       -q    quiet (don't print title)\n");
	fprintf(o,"   -t    testmode (no output)           -s    write to stdout\n");
	fprintf(o,"   -w <filename> write Output as WAV file\n");
	fprintf(o,"   -k n  skip first n frames [0]        -n n  decode only n frames [all]\n");
	fprintf(o,"   -c    check range violations         -y    DISABLE resync on errors\n");
	fprintf(o,"   -b n  output buffer: n Kbytes [0]    -f n  change scalefactor [%li]\n", param.outscale);
	fprintf(o,"   -r n  set/force samplerate [auto]\n");
	fprintf(o,"   -os,-ol,-oh  output to built-in speaker,line-out connector,headphones\n");
	#ifdef NAS
	fprintf(o,"                                        -a d  set NAS server\n");
	#elif defined(SGI)
	fprintf(o,"                                        -a [1..4] set RAD device\n");
	#else
	fprintf(o,"                                        -a d  set audio device\n");
	#endif
	fprintf(o,"   -2    downsample 1:2 (22 kHz)        -4    downsample 1:4 (11 kHz)\n");
	fprintf(o,"   -d n  play every n'th frame only     -h n  play every frame n times\n");
	fprintf(o,"   -0    decode channel 0 (left) only   -1    decode channel 1 (right) only\n");
	fprintf(o,"   -m    mix both channels (mono)       -p p  use HTTP proxy p [$HTTP_PROXY]\n");
	#ifdef HAVE_SCHED_SETSCHEDULER
	fprintf(o,"   -@ f  read filenames/URLs from f     -T get realtime priority\n");
	#else
	fprintf(o,"   -@ f  read filenames/URLs from f\n");
	#endif
	fprintf(o,"   -z    shuffle play (with wildcards)  -Z    random play\n");
	fprintf(o,"   -u a  HTTP authentication string     -E f  Equalizer, data from file\n");
#ifdef HAVE_TERMIOS
	fprintf(o,"   -C    enable control keys            --no-gapless  not skip junk/padding in mp3s\n");
#else
	fprintf(o,"                                        --no-gapless  not skip junk/padding in mp3s\n");
#endif
	fprintf(o,"   -?    this help                      --version  print name + version\n");
	fprintf(o,"See the manpage %s(1) or call %s with --longhelp for more parameters and information.\n", prgName,prgName);
	safe_exit(err);
}

static void want_usage(char* arg)
{
	usage(0);
}

static void long_usage(int err)
{
	char *enclist;
	FILE* o = stdout;
	if(err)
	{
  	o = stderr; 
  	fprintf(o, "You made some mistake in program usage... let me remind you:\n\n");
	}
	print_title(o);
	fprintf(o,"\nusage: %s [option(s)] [file(s) | URL(s) | -]\n", prgName);

	fprintf(o,"\ninput options\n\n");
	fprintf(o," -k <n> --skip <n>         skip n frames at beginning\n");
	fprintf(o,"        --skip-id3v2       skip ID3v2 tags without parsing\n");
	fprintf(o," -n     --frames <n>       play only <n> frames of every stream\n");
	fprintf(o,"        --fuzzy            Enable fuzzy seeks (guessing byte offsets or using approximate seek points from Xing TOC)\n");
	fprintf(o," -y     --no-resync        DISABLES resync on error (--resync is deprecated)\n");
	fprintf(o," -p <f> --proxy <f>        set WWW proxy\n");
	fprintf(o," -u     --auth             set auth values for HTTP access\n");
	fprintf(o,"        --ignore-mime      ignore HTTP MIME types (content-type)\n");
	fprintf(o,"        --no-seekbuffer    disable seek buffer\n");
	fprintf(o," -@ <f> --list <f>         play songs in playlist <f> (plain list, m3u, pls (shoutcast))\n");
	fprintf(o," -l <n> --listentry <n>    play nth title in playlist; show whole playlist for n < 0\n");
	fprintf(o,"        --continue         playlist continuation mode (see man page)\n");
	fprintf(o,"        --loop <n>         loop track(s) <n> times, < 0 means infinite loop (not with --random!)\n");
	fprintf(o,"        --keep-open        (--remote mode only) keep loaded file open after reaching end\n");
	fprintf(o,"        --timeout <n>      Timeout in seconds before declaring a stream dead (if <= 0, wait forever)\n");
	fprintf(o," -z     --shuffle          shuffle song-list before playing\n");
	fprintf(o," -Z     --random           full random play\n");
	fprintf(o,"        --no-icy-meta      Do not accept ICY meta data\n");
	fprintf(o," -i     --index            index / scan through the track before playback\n");
	fprintf(o,"        --index-size <n>   change size of frame index\n");
	fprintf(o,"        --preframes  <n>   number of frames to decode in advance after seeking (to keep layer 3 bit reservoir happy)\n");
	fprintf(o,"        --resync-limit <n> Set number of bytes to search for valid MPEG data; <0 means search whole stream.\n");
	fprintf(o,"        --streamdump <f>   Dump a copy of input data (as read by libmpg123) to given file.\n");
	fprintf(o,"        --icy-interval <n> Enforce ICY interval in bytes (for playing a stream dump.\n");
	fprintf(o,"        --ignore-streamlength Ignore header info about length of MPEG streams.");
	fprintf(o,"\noutput/processing options\n\n");
	fprintf(o," -o <o> --output <o>       select audio output module\n");
	fprintf(o,"        --list-modules     list the available modules\n");
	fprintf(o," -a <d> --audiodevice <d>  select audio device\n");
	fprintf(o," -s     --stdout           write raw audio to stdout (host native format)\n");
	fprintf(o," -S     --STDOUT           play AND output stream (not implemented yet)\n");
	fprintf(o," -w <f> --wav <f>          write samples as WAV file in <f> (- is stdout)\n");
	fprintf(o,"        --au <f>           write samples as Sun AU file in <f> (- is stdout)\n");
	fprintf(o,"        --cdr <f>          write samples as raw CD audio file in <f> (- is stdout)\n");
	fprintf(o,"        --reopen           force close/open on audiodevice\n");
	#ifdef OPT_MULTI
	fprintf(o,"        --cpu <string>     set cpu optimization\n");
	fprintf(o,"        --test-cpu         list optimizations possible with cpu and exit\n");
	fprintf(o,"        --list-cpu         list builtin optimizations and exit\n");
	#endif
	#ifdef OPT_3DNOW
	fprintf(o,"        --test-3dnow       display result of 3DNow! autodetect and exit (obsoleted by --cpu)\n");
	fprintf(o,"        --force-3dnow      force use of 3DNow! optimized routine (obsoleted by --test-cpu)\n");
	fprintf(o,"        --no-3dnow         force use of floating-pointer routine (obsoleted by --cpu)\n");
	#endif
	fprintf(o," -g     --gain             [DEPRECATED] set audio hardware output gain\n");
	fprintf(o," -f <n> --scale <n>        scale output samples (soft gain - based on 32768), default=%li)\n", param.outscale);
	fprintf(o,"        --rva-mix,\n");
	fprintf(o,"        --rva-radio        use RVA2/ReplayGain values for mix/radio mode\n");
	fprintf(o,"        --rva-album,\n");
	fprintf(o,"        --rva-audiophile   use RVA2/ReplayGain values for album/audiophile mode\n");
	fprintf(o," -0     --left --single0   play only left channel\n");
	fprintf(o," -1     --right --single1  play only right channel\n");
	fprintf(o," -m     --mono --mix       mix stereo to mono\n");
	fprintf(o,"        --stereo           duplicate mono channel\n");
	fprintf(o," -r     --rate             force a specific audio output rate\n");
	fprintf(o," -2     --2to1             2:1 downsampling\n");
	fprintf(o," -4     --4to1             4:1 downsampling\n");
  fprintf(o,"        --pitch <value>    set hardware pitch (speedup/down, 0 is neutral; 0.05 is 5%%)\n");
	fprintf(o,"        --8bit             force 8 bit output\n");
	fprintf(o,"        --float            force floating point output (internal precision)\n");
	audio_enclist(&enclist);
	fprintf(o," -e <c> --encoding <c>     force a specific encoding (%s)\n", enclist != NULL ? enclist : "OOM!");
	fprintf(o," -d n   --doublespeed n    play only every nth frame\n");
	fprintf(o," -h n   --halfspeed   n    play every frame n times\n");
	fprintf(o,"        --equalizer        exp.: scales freq. bands acrd. to 'equalizer.dat'\n");
	fprintf(o,"        --gapless          remove padding/junk on mp3s (best with Lame tag)\n");
	fprintf(o,"                           This is on by default when libmpg123 supports it.\n");
	fprintf(o,"        --no-gapless       disable gapless mode, not remove padding/junk\n");
	fprintf(o," -D n   --delay n          insert a delay of n seconds before each track\n");
	fprintf(o," -o h   --headphones       (aix/hp/sun) output on headphones\n");
	fprintf(o," -o s   --speaker          (aix/hp/sun) output on speaker\n");
	fprintf(o," -o l   --lineout          (aix/hp/sun) output to lineout\n");
#ifndef NOXFERMEM
	fprintf(o," -b <n> --buffer <n>       set play buffer (\"output cache\")\n");
	fprintf(o,"        --preload <value>  fraction of buffer to fill before playback\n");
	fprintf(o,"        --smooth           keep buffer over track boundaries\n");
#endif

	fprintf(o,"\nmisc options\n\n");
	fprintf(o," -t     --test             only decode, no output (benchmark)\n");
	fprintf(o," -c     --check            count and display clipped samples\n");
	fprintf(o," -v[*]  --verbose          increase verboselevel\n");
	fprintf(o," -q     --quiet            quiet mode\n");
	#ifdef HAVE_TERMIOS
	fprintf(o," -C     --control          enable terminal control keys\n");
	fprintf(o,"        --ctrlusr1 <c>     control key (characer) to map to SIGUSR1\n");
	fprintf(o,"                           (default is for stop/start)\n");
	fprintf(o,"        --ctrlusr2 <c>     control key (characer) to map to SIGUSR2\n");
	fprintf(o,"                           (default is for next track)\n");
	#endif
	#ifndef GENERIC
	fprintf(o,"        --title            set terminal title to filename\n");
	#endif
	fprintf(o,"        --long-tag         spacy id3 display with every item on a separate line\n");
	fprintf(o,"        --lyrics           show lyrics (from ID3v2 USLT frame)\n");
	fprintf(o,"        --utf8             Regardless of environment, print metadata in UTF-8.\n");
	fprintf(o," -R     --remote           generic remote interface\n");
	fprintf(o,"        --remote-err       force use of stderr for generic remote interface\n");
#ifdef FIFO
	fprintf(o,"        --fifo <path>      open a FIFO at <path> for commands instead of stdin\n");
#endif
	#ifdef HAVE_SETPRIORITY
	fprintf(o,"        --aggressive       tries to get higher priority (nice)\n");
	#endif
	#if defined (HAVE_SCHED_SETSCHEDULER) || defined (HAVE_WINDOWS_H)
	fprintf(o," -T     --realtime         tries to get realtime priority\n");
	#endif
	#ifdef HAVE_WINDOWS_H
	fprintf(o,"        --priority <n>     use specified process priority\n");
	fprintf(o,"                           accepts -2 to 3 as integer arguments\n");
	fprintf(o,"                           -2 as idle, 0 as normal and 3 as realtime.\n");
	#endif
	fprintf(o," -?     --help             give compact help\n");
	fprintf(o,"        --longhelp         give this long help listing\n");
	fprintf(o,"        --version          give name / version string\n");

	fprintf(o,"\nSee the manpage %s(1) for more information.\n", prgName);
	safe_exit(err);
}

static void want_long_usage(char* arg)
{
	long_usage(0);
}

static void give_version(char* arg)
{
	fprintf(stdout, PACKAGE_NAME" "PACKAGE_VERSION"\n");
	safe_exit(0);
}
