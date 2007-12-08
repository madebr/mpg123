/*
	term: terminal control

	copyright ?-2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123app.h"

#ifdef HAVE_TERMIOS

#include <termios.h>
#include <ctype.h>
#include <sys/time.h>

#include "buffer.h"
#include "term.h"
#include "common.h"

extern int buffer_pid;
extern audio_output_t *ao;

static int term_enable = 0;
static struct termios old_tio;
int seeking = FALSE;

void term_sigcont(int sig);

/* This must call only functions safe inside a signal handler. */
int term_setup(struct termios *pattern)
{
  struct termios tio = *pattern;

  signal(SIGCONT, term_sigcont);

  tio.c_lflag &= ~(ICANON|ECHO); 
  tio.c_cc[VMIN] = 1;
  tio.c_cc[VTIME] = 0;
  return tcsetattr(0,TCSANOW,&tio);
}

void term_sigcont(int sig)
{
  term_enable = 0;

  if (term_setup(&old_tio) < 0) {
    fprintf(stderr,"Can't set terminal attributes\n");
    return;
  }

  term_enable = 1;
}

/* initialze terminal */
void term_init(void)
{
  debug("term_init");

  term_enable = 0;

  if(tcgetattr(0,&old_tio) < 0) {
    fprintf(stderr,"Can't get terminal attributes\n");
    return;
  }
  if(term_setup(&old_tio) < 0) {
    fprintf(stderr,"Can't set terminal attributes\n");
    return;
  }

  term_enable = 1;
}

static void term_handle_input(mpg123_handle *,int);

static int stopped = 0;
static int paused = 0;
static int pause_cycle;

static int print_index(mpg123_handle *mh)
{
	int err;
	size_t c, fill;
	off_t *index;
	off_t  step;
	err = mpg123_index(mh, &index, &step, &fill);
	if(err == MPG123_ERR)
	{
		fprintf(stderr, "Error accessing frame index: %s\n", mpg123_strerror(mh));
		return err;
	}
	for(c=0; c < fill;++c) 
		fprintf(stderr, "[%lu] %lu: %li (+%li)\n",
		(unsigned long) c,
		(unsigned long) (c*step), 
		(long) index[c], 
		(long) (c ? index[c]-index[c-1] : 0));
	return MPG123_OK;
}

static off_t offset = 0;

off_t term_control(mpg123_handle *fr)
{
	offset = 0;

	if(!term_enable) return 0;

	if(paused)
	{
		if(!--pause_cycle)
		{
			pause_cycle=(int)(LOOP_CYCLES/mpg123_tpf(fr));
			offset-=pause_cycle;
			if(param.usebuffer)
			{
				while(paused && xfermem_get_usedspace(buffermem))
				{
					buffer_ignore_lowmem();
					term_handle_input(fr, TRUE);
				}
				if(!paused)	offset += pause_cycle;
			}
		}
	}

	do
	{
		term_handle_input(fr, stopped|seeking);
		if((offset < 0) && (-offset > framenum)) offset = - framenum;
		if(param.verbose && offset != 0)
		print_stat(fr,offset,0);
	} while (stopped);

	/* Make the seeking experience with buffer less annoying.
	   No sound during seek, but at least it is possible to go backwards. */
	if(offset)
	{
		if((offset = mpg123_seek_frame(fr, offset, SEEK_CUR)) >= 0)
		debug1("seeked to %li", (long)offset);
		else error1("seek failed: %s!", mpg123_strerror(fr));
		/* Buffer resync already happened on un-stop? */
		/* if(param.usebuffer) buffer_resync();*/
	}
	return 0;
}

/* Stop playback while seeking if buffer is involved. */
static void seekmode(void)
{
	if(param.usebuffer && !stopped)
	{
		stopped = TRUE;
		buffer_stop();
		fprintf(stderr, "%s", STOPPED_STRING);
	}
}

static void term_handle_input(mpg123_handle *fr, int do_delay)
{
  int n = 1;
  /* long offset = 0; */
  
  while(n > 0) {
    fd_set r;
    struct timeval t;
    char val;

    t.tv_sec=0;
    t.tv_usec=(do_delay) ? 10*1000 : 0;
    
    FD_ZERO(&r);
    FD_SET(0,&r);
    n = select(1,&r,NULL,NULL,&t);
    if(n > 0 && FD_ISSET(0,&r)) {
      if(read(0,&val,1) <= 0)
        break;

      switch(tolower(val)) {
	case BACK_KEY:
        if(!param.usebuffer) ao->flush(ao);
				else buffer_resync();
		if(paused) pause_cycle=(int)(LOOP_CYCLES/mpg123_tpf(fr));

		if(mpg123_seek_frame(fr, 0, SEEK_SET) < 0)
		error1("Seek to begin failed: %s", mpg123_strerror(fr));

		framenum=0;
		break;
	case NEXT_KEY:
		if(!param.usebuffer) ao->flush(ao);
		else buffer_resync(); /* was: plain_buffer_resync */
	  next_track();
	  break;
	case QUIT_KEY:
		debug("QUIT");
		if(stopped)
		{
			stopped = 0;
			if(param.usebuffer)
			{
				buffer_resync();
				buffer_start();
			}
		}
		set_intflag();
		offset = 0;
	  break;
	case PAUSE_KEY:
  	  paused=1-paused;
	  if(paused) {
			/* Not really sure if that is what is wanted
			   This jumps in audio output, but has direct reaction to pausing loop. */
			if(param.usebuffer) buffer_resync();

		  pause_cycle=(int)(LOOP_CYCLES/mpg123_tpf(fr));
		  offset -= pause_cycle;
	  }
		if(stopped)
		{
			stopped=0;
			if(param.usebuffer) buffer_start();
		}
	  fprintf(stderr, "%s", (paused) ? PAUSED_STRING : EMPTY_STRING);
	  break;
	case STOP_KEY:
	case ' ':
		/* when seeking while stopped and then resuming, I want to prevent the chirp from the past */
		if(!param.usebuffer) ao->flush(ao);
	  stopped=1-stopped;
	  if(paused) {
		  paused=0;
		  offset -= pause_cycle;
	  }
		if(param.usebuffer)
		{
			if(stopped) buffer_stop();
			else
			{
				/* When we stopped buffer for seeking, we must resync. */
				if(offset) buffer_resync();

				buffer_start();
			}
		}
	  fprintf(stderr, "%s", (stopped) ? STOPPED_STRING : EMPTY_STRING);
	  break;
	case FINE_REWIND_KEY:
	  if(param.usebuffer) seekmode();
	  offset--;
	  break;
	case FINE_FORWARD_KEY:
	  seekmode();
	  offset++;
	  break;
	case REWIND_KEY:
	  seekmode();
  	  offset-=10;
	  break;
	case FORWARD_KEY:
	  seekmode();
	  offset+=10;
	  break;
	case FAST_REWIND_KEY:
	  seekmode();
	  offset-=50;
	  break;
	case FAST_FORWARD_KEY:
	  seekmode();
	  offset+=50;
	  break;
	case VOL_UP_KEY:
		mpg123_volume_change(fr, 0.02);
	break;
	case VOL_DOWN_KEY:
		mpg123_volume_change(fr, -0.02);
	break;
	case VERBOSE_KEY:
		param.verbose++;
		if(param.verbose > VERBOSE_MAX)
		{
			param.verbose = 0;
			clear_stat();
		}
	break;
	case RVA_KEY:
		if(++param.rva > MPG123_RVA_MAX) param.rva = 0;
		mpg123_param(fr, MPG123_RVA, param.rva, 0);
		mpg123_volume(fr, -1);
	break;
	case HELP_KEY:
	  fprintf(stderr,"\n\n -= terminal control keys =-\n[%c] or space bar\t interrupt/restart playback (i.e. 'pause')\n[%c]\t next track\n[%c]\t back to beginning of track\n[%c]\t pause while looping current sound chunk\n[%c]\t forward\n[%c]\t rewind\n[%c]\t fast forward\n[%c]\t fast rewind\n[%c]\t fine forward\n[%c]\t fine rewind\n[%c]\t volume up\n[%c]\t volume down\n[%c]\t RVA switch\n[%c]\t verbose switch\n[%c]\t this help\n[%c]\t quit\n\n",
		        STOP_KEY, NEXT_KEY, BACK_KEY, PAUSE_KEY, FORWARD_KEY, REWIND_KEY, FAST_FORWARD_KEY, FAST_REWIND_KEY, FINE_FORWARD_KEY, FINE_REWIND_KEY, VOL_UP_KEY, VOL_DOWN_KEY, RVA_KEY, VERBOSE_KEY, HELP_KEY, QUIT_KEY);
	break;
	case FRAME_INDEX_KEY:
		print_index(fr);
	break;
	default:
	  ;
      }
    }
  }
}

void term_restore(void)
{
  
  if(!term_enable)
    return;

  tcsetattr(0,TCSAFLUSH,&old_tio);
}

#endif

