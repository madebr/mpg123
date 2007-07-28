/*
	mpg123: main code of the program (not of the decoder...)

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#define ME "main"
#include "mpg123.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif

/* be paranoid about setpriority support */
#ifndef PRIO_PROCESS
#undef HAVE_SETPRIORITY
#endif

#include "common.h"
#include "getlopt.h"
#include "buffer.h"
#include "term.h"
#ifdef GAPLESS
#include "layer3.h"
#endif
#include "playlist.h"
#include "id3.h"
#include "icy.h"

#ifdef OPT_MPLAYER
/* disappear! */
func_dct64 mpl_dct64;
#endif

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
  TRUE ,  /* resync after stream error */
  0 ,     /* verbose level */
#ifdef HAVE_TERMIOS
  FALSE , /* term control */
#endif
  0 ,     /* force mono */
  0 ,     /* force stereo */
  0 ,     /* force 8bit */
  0 ,     /* force rate */
  0 , 	  /* down sample */
  FALSE , /* checkrange */
  0 ,	  /* doublespeed */
  0 ,	  /* halfspeed */
  0 ,	  /* force_reopen, always (re)opens audio device for next song */
  #ifdef OPT_3DNOW
  0 ,     /* autodetect from CPUFLAGS */
  #endif
  /* test_cpu flag is valid for multi and 3dnow.. even if 3dnow is built alone; ensure it appears only once */
  #ifdef OPT_MULTI
  FALSE , /* normal operation */
  #else
  #ifdef OPT_3DNOW
  FALSE , /* normal operation */
  #endif
  #endif
  FALSE,  /* try to run process in 'realtime mode' */   
  { 0,},  /* wav,cdr,au Filename */
#ifdef GAPLESS
	0, /* gapless off per default - yet */
#endif
	0, /* default is to play all titles in playlist */
	0, /* do not use rva per default */
	NULL, /* no playlist per default */
	0 /* condensed id3 per default */
	#ifdef OPT_MULTI
	,NULL /* choose optimization */
	,0
	#endif
#ifdef FIFO
	,NULL
#endif
#ifndef WIN32
	,0
#endif
	,1
};

char *prgName = NULL;
char *equalfile = NULL;
/* ThOr: pointers are not TRUE or FALSE */
int have_eq_settings = FALSE;
scale_t outscale  = MAXOUTBURST;
long numframes = -1;
long startFrame= 0;
int buffer_fd[2];
int buffer_pid;

static int intflag = FALSE;

int OutputDescriptor;

/* A safe realloc also for very old systems where realloc(NULL, size) returns NULL. */
void *safe_realloc(void *ptr, size_t size)
{
	if(ptr == NULL) return malloc(size);
	else return realloc(ptr, size);
}

#ifndef HAVE_STRERROR
const char *strerror(int errnum)
{
  extern int sys_nerr;
  extern char *sys_errlist[];

  return (errnum < sys_nerr) ?  sys_errlist[errnum]  :  "";
}
#endif

#if !defined(WIN32) && !defined(GENERIC)
#ifndef NOXFERMEM
static void catch_child(void)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);
}
#endif

static void catch_interrupt(void)
{
  intflag = TRUE;
}
#endif

/* oh, what a mess... */
void next_track(void)
{
	intflag = TRUE;
}

void safe_exit(int code)
{
#ifdef HAVE_TERMIOS
	if(param.term_ctrl)
		term_restore();
#endif
	exit(code);
}

static struct frame fr;
struct audio_info_struct ai,pre_ai;
txfermem *buffermem = NULL;
#define FRAMEBUFUNIT (18 * 64 * 4)

void set_synth_functions(struct frame *fr);

void init_output(void)
{
  static int init_done = FALSE;

  if (init_done)
    return;
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
  
  if (param.usebuffer) {
    unsigned int bufferbytes;
    sigset_t newsigset, oldsigset;
    if (param.usebuffer < 32)
      param.usebuffer = 32; /* minimum is 32 Kbytes! */
    bufferbytes = (param.usebuffer * 1024);
    bufferbytes -= bufferbytes % FRAMEBUFUNIT;
	/* +1024 for NtoM rounding problems */
    xfermem_init (&buffermem, bufferbytes ,0,1024);
    pcm_sample = (unsigned char *) buffermem->data;
    pcm_point = 0;
    sigemptyset (&newsigset);
    sigaddset (&newsigset, SIGUSR1);
    sigprocmask (SIG_BLOCK, &newsigset, &oldsigset);
    #if !defined(WIN32) && !defined(GENERIC)
    catchsignal (SIGCHLD, catch_child);
	 #endif
    switch ((buffer_pid = fork())) {
      case -1: /* error */
        perror("fork()");
        safe_exit(1);
      case 0: /* child */
        if(rd)
          rd->close(rd); /* child doesn't need the input stream */
        xfermem_init_reader (buffermem);
        buffer_loop (&ai, &oldsigset);
        xfermem_done_reader (buffermem);
        xfermem_done (buffermem);
        exit(0);
      default: /* parent */
        xfermem_init_writer (buffermem);
        param.outmode = DECODE_BUFFER;
    }
  }
  else {
#endif
	/* + 1024 for NtoM rate converter */
    if (!(pcm_sample = (unsigned char *) malloc(audiobufsize * 2 + 1024))) {
      perror ("malloc()");
      safe_exit (1);
#ifndef NOXFERMEM
    }
#endif
  }

  switch(param.outmode) {
    case DECODE_AUDIO:
      if(audio_open(&ai) < 0) {
        perror("audio");
        safe_exit(1);
      }
      break;
    case DECODE_WAV:
      wav_open(&ai,param.filename);
      break;
    case DECODE_AU:
      au_open(&ai,param.filename);
      break;
    case DECODE_CDR:
      cdr_open(&ai,param.filename);
      break;
  }
}

static void set_output_h(char *a)
{
  if(ai.output <= 0)
    ai.output = AUDIO_OUT_HEADPHONES;
  else
    ai.output |= AUDIO_OUT_HEADPHONES;
}
static void set_output_s(char *a)
{
  if(ai.output <= 0)
    ai.output = AUDIO_OUT_INTERNAL_SPEAKER;
  else
    ai.output |= AUDIO_OUT_INTERNAL_SPEAKER;
}
static void set_output_l(char *a)
{
  if(ai.output <= 0)
    ai.output = AUDIO_OUT_LINE_OUT;
  else
    ai.output |= AUDIO_OUT_LINE_OUT;
}

static void set_output (char *arg)
{
    switch (*arg) {
        case 'h': set_output_h(arg); break;
        case 's': set_output_s(arg); break;
        case 'l': set_output_l(arg); break;
        default:
            error3("%s: Unknown argument \"%s\" to option \"%s\".\n",
                prgName, arg, loptarg);
            safe_exit(1);
    }
}

void set_verbose (char *arg)
{
    param.verbose++;
}
void set_wav(char *arg)
{
  param.outmode = DECODE_WAV;
  strncpy(param.filename,arg,255);
  param.filename[255] = 0;
}
void set_cdr(char *arg)
{
  param.outmode = DECODE_CDR;
  strncpy(param.filename,arg,255);
  param.filename[255] = 0;
}
void set_au(char *arg)
{
  param.outmode = DECODE_AU;
  strncpy(param.filename,arg,255);
  param.filename[255] = 0;
}
static void SetOutFile(char *Arg)
{
	param.outmode=DECODE_FILE;
	#ifdef WIN32
	OutputDescriptor=_open(Arg,_O_CREAT|_O_WRONLY|_O_BINARY|_O_TRUNC,0666);
	#else
	OutputDescriptor=open(Arg,O_CREAT|O_WRONLY|O_TRUNC,0666);
	#endif
	if(OutputDescriptor==-1)
	{
		error2("Can't open %s for writing (%s).\n",Arg,strerror(errno));
		safe_exit(1);
	}
}
static void SetOutStdout(char *Arg)
{
	param.outmode=DECODE_FILE;
	param.remote_err=TRUE;
	OutputDescriptor=STDOUT_FILENO;
	#ifdef WIN32
	_setmode(STDOUT_FILENO, _O_BINARY);
	#endif
}
static void SetOutStdout1(char *Arg)
{
  param.outmode=DECODE_AUDIOFILE;
	param.remote_err=TRUE;
  OutputDescriptor=STDOUT_FILENO;
	#ifdef WIN32
	_setmode(STDOUT_FILENO, _O_BINARY);
	#endif
}

void realtime_not_compiled(char *arg)
{
  fprintf(stderr,"Option '-T / --realtime' not compiled into this binary.\n");
}

/* Please note: GLO_NUM expects point to LONG! */
/* ThOr:
 *  Yeah, and despite that numerous addresses to int variables were 
passed.
 *  That's not good on my Alpha machine with int=32bit and long=64bit!
 *  Introduced GLO_INT and GLO_LONG as different bits to make that clear.
 *  GLO_NUM no longer exists.
 */
topt opts[] = {
	{'k', "skip",        GLO_ARG | GLO_LONG, 0, &startFrame, 0},
	{'a', "audiodevice", GLO_ARG | GLO_CHAR, 0, &ai.device,  0},
	{'2', "2to1",        GLO_INT,  0, &param.down_sample, 1},
	{'4', "4to1",        GLO_INT,  0, &param.down_sample, 2},
	{'t', "test",        GLO_INT,  0, &param.outmode, DECODE_TEST},
	{'s', "stdout",      GLO_INT,  SetOutStdout, &param.outmode, DECODE_FILE},
	{'S', "STDOUT",      GLO_INT,  SetOutStdout1, &param.outmode,DECODE_AUDIOFILE},
	{'O', "outfile",     GLO_ARG | GLO_CHAR, SetOutFile, NULL, 0},
	{'c', "check",       GLO_INT,  0, &param.checkrange, TRUE},
	{'v', "verbose",     0,        set_verbose, 0,           0},
	{'q', "quiet",       GLO_INT,  0, &param.quiet, TRUE},
	{'y', "resync",      GLO_INT,  0, &param.tryresync, FALSE},
	{'0', "single0",     GLO_INT,  0, &param.force_mono, MONO_LEFT},
	{0,   "left",        GLO_INT,  0, &param.force_mono, MONO_LEFT},
	{'1', "single1",     GLO_INT,  0, &param.force_mono, MONO_RIGHT},
	{0,   "right",       GLO_INT,  0, &param.force_mono, MONO_RIGHT},
	{'m', "singlemix",   GLO_INT,  0, &param.force_mono, MONO_MIX},
	{0,   "mix",         GLO_INT,  0, &param.force_mono, MONO_MIX},
	{0,   "mono",        GLO_INT,  0, &param.force_mono, MONO_MIX},
	{0,   "stereo",      GLO_INT,  0, &param.force_stereo, 1},
	{0,   "reopen",      GLO_INT,  0, &param.force_reopen, 1},
	{'g', "gain",        GLO_ARG | GLO_LONG, 0, &ai.gain,    0},
	{'r', "rate",        GLO_ARG | GLO_LONG, 0, &param.force_rate,  0},
	{0,   "8bit",        GLO_INT,  0, &param.force_8bit, 1},
	{0,   "headphones",  0,                  set_output_h, 0,0},
	{0,   "speaker",     0,                  set_output_s, 0,0},
	{0,   "lineout",     0,                  set_output_l, 0,0},
	{'o', "output",      GLO_ARG | GLO_CHAR, set_output, 0,  0},
#ifdef FLOATOUT
	{'f', "scale",       GLO_ARG | GLO_DOUBLE, 0, &outscale,   0},
#else
	{'f', "scale",       GLO_ARG | GLO_LONG, 0, &outscale,   0},
#endif
	{'n', "frames",      GLO_ARG | GLO_LONG, 0, &numframes,  0},
	#ifdef HAVE_TERMIOS
	{'C', "control",     GLO_INT,  0, &param.term_ctrl, TRUE},
	#endif
	{'b', "buffer",      GLO_ARG | GLO_LONG, 0, &param.usebuffer,  0},
	{'R', "remote",      GLO_INT,  0, &param.remote, TRUE},
	{0,   "remote-err",  GLO_INT,  0, &param.remote_err, TRUE},
	{'d', "doublespeed", GLO_ARG | GLO_LONG, 0, &param.doublespeed,0},
	{'h', "halfspeed",   GLO_ARG | GLO_LONG, 0, &param.halfspeed,  0},
	{'p', "proxy",       GLO_ARG | GLO_CHAR, 0, &proxyurl,   0},
	{'@', "list",        GLO_ARG | GLO_CHAR, 0, &param.listname,   0},
	/* 'z' comes from the the german word 'zufall' (eng: random) */
	{'z', "shuffle",     GLO_INT,  0, &param.shuffle, 1},
	{'Z', "random",      GLO_INT,  0, &param.shuffle, 2},
	{'E', "equalizer",	 GLO_ARG | GLO_CHAR, 0, &equalfile,1},
	#ifdef HAVE_SETPRIORITY
	{0,   "aggressive",	 GLO_INT,  0, &param.aggressive, 2},
	#endif
	#ifdef OPT_3DNOW
	{0,   "force-3dnow", GLO_INT,  0, &param.stat_3dnow, 1},
	{0,   "no-3dnow",    GLO_INT,  0, &param.stat_3dnow, 2},
	{0,   "test-3dnow",  GLO_INT,  0, &param.test_cpu, TRUE},
	#endif
	#ifdef OPT_MULTI
	{0, "cpu", GLO_ARG | GLO_CHAR, 0, &param.cpu,  0},
	{0, "test-cpu",  GLO_INT,  0, &param.test_cpu, TRUE},
	{0, "list-cpu", GLO_INT,  0, &param.list_cpu , 1},
	#endif
	#if !defined(WIN32) && !defined(GENERIC)
	{'u', "auth",        GLO_ARG | GLO_CHAR, 0, &httpauth,   0},
	#endif
	#ifdef HAVE_SCHED_SETSCHEDULER
	/* check why this should be a long variable instead of int! */
	{'T', "realtime",    GLO_LONG,  0, &param.realtime, TRUE },
	#else
	{'T', "realtime",    0,  realtime_not_compiled, 0,           0 },    
	#endif
	{0, "title",         GLO_INT,  0, &param.xterm_title, TRUE },
	{'w', "wav",         GLO_ARG | GLO_CHAR, set_wav, 0 , 0 },
	{0, "cdr",           GLO_ARG | GLO_CHAR, set_cdr, 0 , 0 },
	{0, "au",            GLO_ARG | GLO_CHAR, set_au, 0 , 0 },
	#ifdef GAPLESS
	{0,   "gapless",	 GLO_INT,  0, &param.gapless, 1},
	#endif
	{'?', "help",            0,  want_usage, 0,           0 },
	{0 , "longhelp" ,        0,  want_long_usage, 0,      0 },
	{0 , "version" ,         0,  give_version, 0,         0 },
	{'l', "listentry",       GLO_ARG | GLO_LONG, 0, &param.listentry, 0 },
	{0, "rva-mix",         GLO_INT,  0, &param.rva, 1 },
	{0, "rva-radio",         GLO_INT,  0, &param.rva, 1 },
	{0, "rva-album",         GLO_INT,  0, &param.rva, 2 },
	{0, "rva-audiophile",         GLO_INT,  0, &param.rva, 2 },
	{0, "long-tag",         GLO_INT,  0, &param.long_id3, 1 },
#ifdef FIFO
	{0, "fifo", GLO_ARG | GLO_CHAR, 0, &param.fifo,  0},
#endif
#ifndef WIN32
	{0, "timeout", GLO_ARG | GLO_LONG, 0, &param.timeout, 0},
#endif
	{0, "loop", GLO_ARG | GLO_LONG, 0, &param.loop, 0},
	{0, 0, 0, 0, 0, 0}
};

/*
 *   Change the playback sample rate.
 *   Consider that changing it after starting playback is not covered by gapless code!
 */
static void reset_audio(void)
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
		buffermem->buf[0] = ai.rate; 
		buffermem->buf[1] = ai.channels; 
		buffermem->buf[2] = ai.format;
		buffer_reset();
	}
	else 
#endif
	if (param.outmode == DECODE_AUDIO) {
		/* audio_reset_parameters(&ai); */
		/*   close and re-open in order to flush
		 *   the device's internal buffer before
		 *   changing the sample rate.   [OF]
		 */
		audio_close (&ai);
		if (audio_open(&ai) < 0) {
			perror("audio");
			safe_exit(1);
		}
	}
}


/*
	precog the audio rate that will be set before output begins
	this is needed to give gapless code a chance to keep track for firstframe != 0
*/
void prepare_audioinfo(struct frame *fr, struct audio_info_struct *nai)
{
	long newrate = freqs[fr->sampling_frequency]>>(param.down_sample);
	fr->down_sample = param.down_sample;
	if(!audio_fit_capabilities(nai,fr->stereo,newrate)) safe_exit(1);
}

/*
 * play a frame read by read_frame();
 * (re)initialize audio if necessary.
 *
 * needs a major rewrite .. it's incredible ugly!
 */
int play_frame(int init,struct frame *fr)
{
	int clip;
	long newrate;
	long old_rate,old_format,old_channels;

	if(fr->header_change || init) {

		if (!param.quiet && init) {
			if (param.verbose)
				print_header(fr);
			else
				print_header_compact(fr);
		}

		if(fr->header_change > 1 || init) {
			old_rate = ai.rate;
			old_format = ai.format;
			old_channels = ai.channels;

			newrate = freqs[fr->sampling_frequency]>>(param.down_sample);
			prepare_audioinfo(fr, &ai);
			if(param.verbose > 1) fprintf(stderr, "Note: audio output rate = %li\n", ai.rate);
			#ifdef GAPLESS
			if(param.gapless && (fr->lay == 3)) layer3_gapless_bytify(fr, &ai);
			#endif
			
			/* check, whether the fitter set our proposed rate */
			if(ai.rate != newrate) {
				if(ai.rate == (newrate>>1) )
					fr->down_sample++;
				else if(ai.rate == (newrate>>2) )
					fr->down_sample+=2;
				else {
					fr->down_sample = 3;
					fprintf(stderr,"Warning, flexible rate not heavily tested!\n");
				}
				if(fr->down_sample > 3)
					fr->down_sample = 3;
			}

			switch(fr->down_sample) {
				case 0:
				case 1:
				case 2:
					fr->down_sample_sblimit = SBLIMIT>>(fr->down_sample);
					break;
				case 3:
					{
						long n = freqs[fr->sampling_frequency];
                                                long m = ai.rate;

						if(!synth_ntom_set_step(n,m)) return 0;

						if(n>m) {
							fr->down_sample_sblimit = SBLIMIT * m;
							fr->down_sample_sblimit /= n;
						}
						else {
							fr->down_sample_sblimit = SBLIMIT;
						}
					}
					break;
			}

			init_output();
			if(ai.rate != old_rate || ai.channels != old_channels ||
			   ai.format != old_format || param.force_reopen) {
				if(!param.force_mono) {
					if(ai.channels == 1)
						fr->single = 3;
					else
						fr->single = -1;
				}
				else
					fr->single = param.force_mono-1;

				param.force_stereo &= ~0x2;
				if(fr->single >= 0 && ai.channels == 2) {
					param.force_stereo |= 0x2;
				}

				set_synth_functions(fr);
				init_layer3(fr->down_sample_sblimit);
				reset_audio();
				if(param.verbose) {
					if(fr->down_sample == 3) {
						long n = freqs[fr->sampling_frequency];
						long m = ai.rate;
						if(n > m) {
							fprintf(stderr,"Audio: %2.4f:1 conversion,",(float)n/(float)m);
						}
						else {
							fprintf(stderr,"Audio: 1:%2.4f conversion,",(float)m/(float)n);
						}
					}
					else {
						fprintf(stderr,"Audio: %ld:1 conversion,",(long)pow(2.0,fr->down_sample));
					}
 					fprintf(stderr," rate: %ld, encoding: %s, channels: %d\n",ai.rate,audio_encoding_name(ai.format),ai.channels);
				}
			}
			if (intflag)
				return 1;
		}
	}

	if (fr->error_protection) {
		getbits(16); /* skip crc */
	}

	/* do the decoding */
	clip = (fr->do_layer)(fr,param.outmode,&ai);

#ifndef NOXFERMEM
	if (param.usebuffer) {
		if (!intflag) {
			buffermem->freeindex =
				(buffermem->freeindex + pcm_point) % buffermem->size;
			if (buffermem->wakeme[XF_READER])
				xfermem_putcmd(buffermem->fd[XF_WRITER], XF_CMD_WAKEUP_INFO);
		}
		pcm_sample = (unsigned char *) (buffermem->data + buffermem->freeindex);
		pcm_point = 0;
		while (xfermem_get_freespace(buffermem) < (FRAMEBUFUNIT << 1))
			if (xfermem_block(XF_WRITER, buffermem) == XF_CMD_TERMINATE) {
				intflag = TRUE;
				break;
			}
		if (intflag)
			return 1;
	}
#endif

	if(clip > 0 && param.checkrange)
		fprintf(stderr,"%d samples clipped\n", clip);
	return 1;
}

/* set synth functions for current frame, optimizations handled by opt_* macros */
void set_synth_functions(struct frame *fr)
{
	int ds = fr->down_sample;
	int p8=0;
	static func_synth funcs[2][4] = { 
		{ NULL,
		  synth_2to1,
		  synth_4to1,
		  synth_ntom } ,
		{ NULL,
		  synth_2to1_8bit,
		  synth_4to1_8bit,
		  synth_ntom_8bit } 
	};
	static func_synth_mono funcs_mono[2][2][4] = {    
		{ { NULL ,
		    synth_2to1_mono2stereo ,
		    synth_4to1_mono2stereo ,
		    synth_ntom_mono2stereo } ,
		  { NULL ,
		    synth_2to1_8bit_mono2stereo ,
		    synth_4to1_8bit_mono2stereo ,
		    synth_ntom_8bit_mono2stereo } } ,
		{ { NULL ,
		    synth_2to1_mono ,
		    synth_4to1_mono ,
		    synth_ntom_mono } ,
		  { NULL ,
		    synth_2to1_8bit_mono ,
		    synth_4to1_8bit_mono ,
		    synth_ntom_8bit_mono } }
	};

	/* possibly non-constand entries filled here */
	funcs[0][0] = opt_synth_1to1;
	funcs[1][0] = opt_synth_1to1_8bit;
	funcs_mono[0][0][0] = opt_synth_1to1_mono2stereo;
	funcs_mono[0][1][0] = opt_synth_1to1_8bit_mono2stereo;
	funcs_mono[1][0][0] = opt_synth_1to1_mono;
	funcs_mono[1][1][0] = opt_synth_1to1_8bit_mono;

	if((ai.format & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_8)
		p8 = 1;
	fr->synth = funcs[p8][ds];
	fr->synth_mono = funcs_mono[ai.channels==2 ? 0 : 1][p8][ds];

	if(p8) {
		if(make_conv16to8_table(ai.format) != 0)
		{
			/* it's a bit more work to get proper error propagation up */
			safe_exit(1);
		}
	}
}

int main(int argc, char *argv[])
{
	int result;
	char *fname;
#if !defined(WIN32) && !defined(GENERIC)
	struct timeval start_time, now;
	unsigned long secdiff;
#endif	
	int init;
	#ifdef GAPLESS
	int pre_init;
	#endif
	int j;

#ifdef OS2
        _wildcard(&argc,&argv);
#endif

	if(sizeof(short) != 2) {
		fprintf(stderr,"Ouch SHORT has size of %d bytes (required: '2')\n",(int)sizeof(short));
		safe_exit(1);
	}
	if(sizeof(long) < 4) {
		fprintf(stderr,"Ouch LONG has size of %d bytes (required: at least 4)\n",(int)sizeof(long));
	}

	(prgName = strrchr(argv[0], '/')) ? prgName++ : (prgName = argv[0]);

	audio_info_struct_init(&ai);

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

	#ifdef OPT_MULTI
	if(param.list_cpu)
	{
		list_cpu_opt();
		safe_exit(0);
	}
	if (param.test_cpu)
	{
		test_cpu_flags();
		safe_exit(0);
	}
	if(!set_cpu_opt()) safe_exit(1);
	#else
	#ifdef OPT_3DNOW
	if (param.test_cpu) {
		struct cpuflags cf;
		getcpuflags(&cf);
		fprintf(stderr,"CPUFLAGS = %08x\n",cf.ext);
		if ((cf.ext & 0x00800000) == 0x00800000) {
			fprintf(stderr,"MMX instructions are supported.\n");
		}
		if ((cf.ext & 0x80000000) == 0x80000000) {
			fprintf(stderr,"3DNow! instructions are supported.\n");
		}
		safe_exit(0);
	}
	#endif
	#endif
	#ifdef OPT_MPLAYER
	mpl_dct64 = opt_mpl_dct64;
	#endif

	if (loptind >= argc && !param.listname && !param.remote)
		usage(1);

#if !defined(WIN32) && !defined(GENERIC)
	if (param.remote) {
		param.verbose = 0;        
		param.quiet = 1;
	}
#endif

	if (!(param.listentry < 0) && !param.quiet)
		print_title(stderr); /* do not pollute stdout! */

	if(param.force_mono) {
		fr.single = param.force_mono-1;
	}

	if(param.force_rate && param.down_sample) {
		fprintf(stderr,"Down sampling and fixed rate options not allowed together!\n");
		safe_exit(1);
	}

	audio_capabilities(&ai);
	/* equalizer initialization regardless of equalfile */
	for(j=0; j<32; j++) {
		equalizer[0][j] = equalizer[1][j] = 1.0;
		equalizer_sum[0][j] = equalizer_sum[1][j] = 0.0;
	}
	if(equalfile != NULL) { /* tst; ThOr: not TRUE or FALSE: allocated or not... */
		FILE *fe;
		int i;

		equalizer_cnt = 0;

		fe = fopen(equalfile,"r");
		if(fe) {
			char line[256];
			for(i=0;i<32;i++) {
				float e1,e0; /* %f -> float! */
				line[0]=0;
				fgets(line,255,fe);
				if(line[0]=='#')
					continue;
				sscanf(line,"%f %f",&e0,&e1);
				equalizer[0][i] = e0;
				equalizer[1][i] = e1;	
			}
			fclose(fe);
			have_eq_settings = TRUE;			
		}
		else
			fprintf(stderr,"Can't open equalizer file '%s'\n",equalfile);
	}

#ifdef HAVE_SETPRIORITY
	if(param.aggressive) { /* tst */
		int mypid = getpid();
		setpriority(PRIO_PROCESS,mypid,-20);
	}
#endif

#ifdef HAVE_SCHED_SETSCHEDULER
	if (param.realtime) {  /* Get real-time priority */
	  struct sched_param sp;
	  fprintf(stderr,"Getting real-time priority\n");
	  memset(&sp, 0, sizeof(struct sched_param));
	  sp.sched_priority = sched_get_priority_min(SCHED_FIFO);
	  if (sched_setscheduler(0, SCHED_RR, &sp) == -1)
	    fprintf(stderr,"Can't get real-time priority\n");
	}
#endif

	set_synth_functions(&fr);

	if(!param.remote) prepare_playlist(argc, argv);

	opt_make_decode_tables(outscale);
	init_layer2(); /* inits also shared tables with layer1 */
	init_layer3(fr.down_sample);

#if !defined(WIN32) && !defined(GENERIC)
	/* This ctrl+c for title skip only when not in some control mode */
	if
	(
		!param.remote 
		#ifdef HAVE_TERMIOS
		&& !param.term_ctrl
		#endif
	)
	catchsignal (SIGINT, catch_interrupt);

	if(param.remote) {
		int ret;
		init_id3();
		init_icy();
		ret = control_generic(&fr);
		clear_icy();
		exit_id3();
		safe_exit(ret);
	}
#endif

	init_icy();
	init_id3(); /* prepare id3 memory */
	while ((fname = get_next_file())) {
		char *dirname, *filename;
		long leftFrames,newFrame;

		if(!*fname || !strcmp(fname, "-"))
			fname = NULL;
               if (open_stream(fname,-1) < 0)
                       continue;
      
		if (!param.quiet) {
			if (split_dir_file(fname ? fname : "standard input",
				&dirname, &filename))
				fprintf(stderr, "\nDirectory: %s", dirname);
			fprintf(stderr, "\nPlaying MPEG stream %lu of %lu: %s ...\n", (unsigned long)pl.pos, (unsigned long)pl.fill, filename);

#if !defined(GENERIC)
{
	const char *term_type;
	term_type = getenv("TERM");
	if (term_type && param.xterm_title &&
	    (!strncmp(term_type,"xterm",5) || !strncmp(term_type,"rxvt",4)))
	{
		fprintf(stderr, "\033]0;%s\007", filename);
	}
}
#endif

		}

#if !defined(WIN32) && !defined(GENERIC)
#ifdef HAVE_TERMIOS
		if(!param.term_ctrl)
#endif
			gettimeofday (&start_time, NULL);
#endif
		read_frame_init(&fr);

		init = 1;
		#ifdef GAPLESS
		pre_init = 1;
		#endif
		newFrame = startFrame;
		
#ifdef HAVE_TERMIOS
		debug1("param.term_ctrl: %i", param.term_ctrl);
		if(param.term_ctrl)
			term_init();
#endif
		leftFrames = numframes;
		/* read_frame is counting the frames! */
		for(;read_frame(&fr) && leftFrames && !intflag;) {
#ifdef HAVE_TERMIOS			
tc_hack:
#endif
			if(fr.num < startFrame || (param.doublespeed && (fr.num % param.doublespeed))) {
				if(fr.lay == 3)
				{
					set_pointer(512);
					#ifdef GAPLESS
					if(param.gapless)
					{
						if(pre_init)
						{
							prepare_audioinfo(&fr, &pre_ai);
							pre_init = 0;
						}
						/* keep track... */
						layer3_gapless_set_position(fr.num, &fr, &pre_ai);
					}
					#endif
				}
				continue;
			}
			if(leftFrames > 0)
			  leftFrames--;
			if(!play_frame(init,&fr))
			{
				error("frame playback failed, skipping rest of track");
				break;
			}
			init = 0;

			if(param.verbose) {
#ifndef NOXFERMEM
				if (param.verbose > 1 || !(fr.num & 0x7))
					print_stat(&fr,fr.num,xfermem_get_usedspace(buffermem),&ai); 
				if(param.verbose > 2 && param.usebuffer)
					fprintf(stderr,"[%08x %08x]",buffermem->readindex,buffermem->freeindex);
#else
				if (param.verbose > 1 || !(fr.num & 0x7))
					print_stat(&fr,fr.num,0,&ai);
#endif
			}
#ifdef HAVE_TERMIOS
			if(!param.term_ctrl) {
				continue;
			} else {
				long offset;
				if((offset=term_control(&fr,&ai))) {
					if(!rd->back_frame(rd, &fr, -offset)) {
						debug1("seeked to %lu", fr.num);
						#ifdef GAPLESS
						if(param.gapless && (fr.lay == 3))
						layer3_gapless_set_position(fr.num, &fr, &ai);
						#endif
					} else { error("seek failed!"); }
				}
			}
#endif

		}
		#ifdef GAPLESS
		/* make sure that the correct padding is skipped after track ended */
		if(param.gapless) audio_flush(param.outmode, &ai);
		#endif

#ifndef NOXFERMEM
	if(param.usebuffer) {
		int s;
		while ((s = xfermem_get_usedspace(buffermem))) {
			struct timeval wait170 = {0, 170000};

			buffer_ignore_lowmem();
			
			if(param.verbose)
				print_stat(&fr,fr.num,s,&ai);
#ifdef HAVE_TERMIOS
			if(param.term_ctrl) {
				long offset;
				if((offset=term_control(&fr,&ai))) {
					if((!rd->back_frame(rd, &fr, -offset)) 
						&& read_frame(&fr))
					{
						debug1("seeked to %lu", fr.num);
						#ifdef GAPLESS
						if(param.gapless && (fr.lay == 3))
						layer3_gapless_set_position(fr.num, &fr, &ai);
						#endif
						goto tc_hack;	/* Doh! Gag me with a spoon! */
					} else { error("seek failed!"); }
				}
			}
#endif
			select(0, NULL, NULL, NULL, &wait170);
		}
	}
#endif
	if(param.verbose)
		print_stat(&fr,fr.num,xfermem_get_usedspace(buffermem),&ai); 
#ifdef HAVE_TERMIOS
	if(param.term_ctrl)
		term_restore();
#endif

	if (!param.quiet) {
		/* 
		 * This formula seems to work at least for
		 * MPEG 1.0/2.0 layer 3 streams.
		 */
		int secs = get_songlen(&fr,fr.num);
		fprintf(stderr,"\n[%d:%02d] Decoding of %s finished.\n", secs / 60,
			secs % 60, filename);
	}

	rd->close(rd);
#if 0
	if(param.remote)
		fprintf(stderr,"@R MPG123\n");        
	if (remflag) {
		intflag = FALSE;
		remflag = FALSE;
	}
#endif
	
      if (intflag) {

/* 
 * When HAVE_TERMIOS is defined, there is 'q' to terminate a list of songs, so
 * no pressing need to keep up this first second SIGINT hack that was too
 * often mistaken as a bug. [dk]
 * ThOr: Yep, I deactivated the Ctrl+C hack for active control modes.
 */
#if !defined(WIN32) && !defined(GENERIC)
#ifdef HAVE_TERMIOS
	if(!param.term_ctrl)
#endif
        {
		gettimeofday (&now, NULL);
        	secdiff = (now.tv_sec - start_time.tv_sec) * 1000;
        	if (now.tv_usec >= start_time.tv_usec)
          		secdiff += (now.tv_usec - start_time.tv_usec) / 1000;
        	else
          		secdiff -= (start_time.tv_usec - now.tv_usec) / 1000;
        	if (secdiff < 1000)
          		break;
	}
#endif
        intflag = FALSE;

#ifndef NOXFERMEM
        if(param.usebuffer) buffer_resync();
#endif
      }
    } /* end of loop over input files */
    clear_icy();
    exit_id3(); /* free id3 memory */
#ifndef NOXFERMEM
    if (param.usebuffer) {
      buffer_end();
      xfermem_done_writer (buffermem);
      waitpid (buffer_pid, NULL, 0);
      xfermem_done (buffermem);
    }
    else {
#endif
      audio_flush(param.outmode, &ai);
      free (pcm_sample);
#ifndef NOXFERMEM
    }
#endif

    switch(param.outmode) {
      case DECODE_AUDIO:
        audio_close(&ai);
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
   	if(!param.remote) free_playlist();
    return 0;
}

static void print_title(FILE *o)
{
	fprintf(o, "High Performance MPEG 1.0/2.0/2.5 Audio Player for Layers 1, 2 and 3\n");
	fprintf(o, "\tversion %s; written and copyright by Michael Hipp and others\n", PACKAGE_VERSION);
	fprintf(o, "\tfree software (LGPL/GPL) without any warranty but with best wishes\n");
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
	fprintf(o,"   -b n  output buffer: n Kbytes [0]    -f n  change scalefactor [%g]\n", (double)outscale);
	fprintf(o,"   -r n  set/force samplerate [auto]    -g n  set audio hardware output gain\n");
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
	#ifdef GAPLESS
	fprintf(o,"   -C    enable control keys            --gapless  skip junk/padding in some mp3s\n");
	#else
	fprintf(o,"   -C    enable control keys\n");
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
	fprintf(o," -n     --frames <n>       play only <n> frames of every stream\n");
	fprintf(o," -y     --resync           DISABLES resync on error\n");
	fprintf(o," -p <f> --proxy <f>        set WWW proxy\n");
	fprintf(o," -u     --auth             set auth values for HTTP access\n");
	fprintf(o," -@ <f> --list <f>         play songs in playlist <f> (plain list, m3u, pls (shoutcast))\n");
	fprintf(o," -l <n> --listentry <n>    play nth title in playlist; show whole playlist for n < 0\n");
	fprintf(o,"        --loop <n>         loop track(s) <n> times, < 0 means infinite loop (not with --random!)\n");
#ifndef WIN32
	fprintf(o,"        --timeout <n>      Timeout in seconds before declaring a stream dead (if <= 0, wait forever)\n");
#endif
	fprintf(o," -z     --shuffle          shuffle song-list before playing\n");
	fprintf(o," -Z     --random           full random play\n");

	fprintf(o,"\noutput/processing options\n\n");
	fprintf(o," -a <d> --audiodevice <d>  select audio device\n");
	fprintf(o," -s     --stdout           write raw audio to stdout (host native format)\n");
	fprintf(o," -S     --STDOUT           play AND output stream (not implemented yet)\n");
	fprintf(o," -w <f> --wav <f>          write samples as WAV file in <f> (- is stdout)\n");
	fprintf(o,"        --au <f>           write samples as Sun AU file in <f> (- is stdout)\n");
	fprintf(o,"        --cdr <f>          write samples as CDR file in <f> (- is stdout)\n");
	fprintf(o,"        --reopen           force close/open on audiodevice\n");
	#ifdef OPT_MULTI
	fprintf(o,"        --cpu <string>     set cpu optimization\n");
	fprintf(o,"        --test-cpu         list optmizations possible with cpu and exit\n");
	fprintf(o,"        --list-cpu         list builtin optimizations and exit\n");
	#endif
	#ifdef OPT_3DNOW
	fprintf(o,"        --test-3dnow       display result of 3DNow! autodetect and exit (obsoleted by --cpu)\n");
	fprintf(o,"        --force-3dnow      force use of 3DNow! optimized routine (obsoleted by --test-cpu)\n");
	fprintf(o,"        --no-3dnow         force use of floating-pointer routine (obsoleted by --cpu)\n");
	#endif
	fprintf(o," -g     --gain             set audio hardware output gain\n");
	fprintf(o," -f <n> --scale <n>        scale output samples (soft gain, default=%g)\n", (double)outscale);
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
	fprintf(o,"        --8bit             force 8 bit output\n");
	fprintf(o," -d     --doublespeed      play only every second frame\n");
	fprintf(o," -h     --halfspeed        play every frame twice\n");
	fprintf(o,"        --equalizer        exp.: scales freq. bands acrd. to 'equalizer.dat'\n");
	#ifdef GAPLESS
	fprintf(o,"        --gapless          remove padding/junk added by encoder/decoder\n");
	#endif
	fprintf(o,"                           (experimental, needs Lame tag, layer 3 only)\n");
	fprintf(o," -o h   --headphones       (aix/hp/sun) output on headphones\n");
	fprintf(o," -o s   --speaker          (aix/hp/sun) output on speaker\n");
	fprintf(o," -o l   --lineout          (aix/hp/sun) output to lineout\n");
	fprintf(o," -b <n> --buffer <n>       set play buffer (\"output cache\")\n");

	fprintf(o,"\nmisc options\n\n");
	fprintf(o," -t     --test             only decode, no output (benchmark)\n");
	fprintf(o," -c     --check            count and display clipped samples\n");
	fprintf(o," -v[*]  --verbose          increase verboselevel\n");
	fprintf(o," -q     --quiet            quiet mode\n");
	#ifdef HAVE_TERMIOS
	fprintf(o," -C     --control          enable terminal control keys\n");
	#endif
	#ifndef GENERIG
	fprintf(o,"        --title            set xterm/rxvt title to filename\n");
	#endif
	fprintf(o,"        --long-tag         spacy id3 display with every item on a separate line\n");
	fprintf(o," -R     --remote           generic remote interface\n");
	fprintf(o,"        --remote-err       force use of stderr for generic remote interface\n");
#ifdef FIFO
	fprintf(o,"        --fifo <path>      open a FIFO at <path> for commands instead of stdin\n");
#endif
	#ifdef HAVE_SETPRIORITY
	fprintf(o,"        --aggressive       tries to get higher priority (nice)\n");
	#endif
	#ifdef HAVE_SCHED_SETSCHEDULER
	fprintf(o," -T     --realtime         tries to get realtime priority\n");
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
