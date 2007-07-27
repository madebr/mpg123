/*
	term: terminal control

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123.h"

#ifdef HAVE_TERMIOS

#include <termios.h>
#include <ctype.h>
#include <sys/time.h>

#include "buffer.h"
#include "term.h"
#include "common.h"

extern int buffer_pid;

static int term_enable = 0;
static struct termios old_tio;

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

static long term_handle_input(struct frame *,int);

static int stopped = 0;
static int paused = 0;
static int pause_cycle;

long term_control(struct frame *fr, struct audio_info_struct *ai)
{
	long offset = 0;

	if(!term_enable) return 0;

	if(paused)
	{
		if(!--pause_cycle)
		{
			pause_cycle=(int)(LOOP_CYCLES/compute_tpf(fr));
			offset-=pause_cycle;
			if(param.usebuffer)
			{
				while(paused && xfermem_get_usedspace(buffermem))
				{
					buffer_ignore_lowmem();
					offset += term_handle_input(fr, TRUE);
				}
				if(!paused)	offset += pause_cycle;
			}
		}
	}

	do
	{
		offset += term_handle_input(fr, stopped);
		if((offset < 0) && (-offset > fr->num)) offset = - fr->num;
		if(param.verbose && offset != 0)
		print_stat(fr,fr->num+offset,0,ai);
	} while (stopped);

	return offset;
}

static long term_handle_input(struct frame *fr, int do_delay)
{
  int n = 1;
  long offset = 0;
  extern struct audio_info_struct ai;
  
  while(n > 0) {
    fd_set r;
    struct timeval t;
    char val;

    t.tv_sec=0;
    t.tv_usec=(do_delay) ? 1000 : 0;
    
    FD_ZERO(&r);
    FD_SET(0,&r);
    n = select(1,&r,NULL,NULL,&t);
    if(n > 0 && FD_ISSET(0,&r)) {
      if(read(0,&val,1) <= 0)
        break;

      switch(tolower(val)) {
	case BACK_KEY:
        if(!param.usebuffer) audio_queueflush(&ai);
	  /*
	   * NOTE: rd->rewind() calls buffer_resync() that blocks until
	   * buffer process returns ACK. If buffer process is stopped, we
	   * end up in a deadlock. The only acceptable workaround was to
	   * resume playing as soon as BACK_KEY is pressed. This is not
	   * necessary when running non-buffered but I chose to remain
	   * compatible. [dk]
	   */
	  if(stopped) {
		  stopped = 0;
		  if(param.usebuffer)
		  	buffer_start();
		  fprintf(stderr, "%s", EMPTY_STRING);
	  }
	  if(paused)
		  pause_cycle=(int)(LOOP_CYCLES/compute_tpf(fr));
          rd->rewind(rd);
          fr->num=0;
          break;
	case NEXT_KEY:
		if(!param.usebuffer) audio_queueflush(&ai);
		plain_buffer_resync();
	  next_track();
	  break;
	case QUIT_KEY:
	  if (buffer_pid)
		  kill(buffer_pid, SIGTERM);
	  kill(getpid(), SIGTERM);
	  break;
	case PAUSE_KEY:
  	  paused=1-paused;
	  if(paused) {
		  pause_cycle=(int)(LOOP_CYCLES/compute_tpf(fr));
		  offset -= pause_cycle;
	  }
	  if(stopped) {
		stopped=0;
		buffer_start();
	  }
	  fprintf(stderr, "%s", (paused) ? PAUSED_STRING : EMPTY_STRING);
	  break;
	case STOP_KEY:
	case ' ':
		/* when seeking while stopped and then resuming, I want to prevent the chirp from the past */
		if(!param.usebuffer) audio_queueflush(&ai);
	  stopped=1-stopped;
	  if(paused) {
		  paused=0;
		  offset -= pause_cycle;
	  }
	  if(stopped) buffer_stop(); else buffer_start();
	  fprintf(stderr, "%s", (stopped) ? STOPPED_STRING : EMPTY_STRING);
	  break;
	case FINE_REWIND_KEY:
	  offset--;
	  break;
	case FINE_FORWARD_KEY:
	  offset++;
	  break;
	case REWIND_KEY:
  	  offset-=10;
	  break;
	case FORWARD_KEY:
	  offset+=10;
	  break;
	case FAST_REWIND_KEY:
	  offset-=50;
	  break;
	case FAST_FORWARD_KEY:
	  offset+=50;
	  break;
	case VOL_UP_KEY:
		do_volume((double) outscale / MAXOUTBURST + 0.02);
	break;
	case VOL_DOWN_KEY:
		do_volume((double) outscale / MAXOUTBURST - 0.02);
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
		param.rva++;
		if(param.rva > RVA_MAX) param.rva = 0;
		do_rva();
	break;
	case HELP_KEY:
	  fprintf(stderr,"\n\n -= terminal control keys =-\n[%c] or space bar\t interrupt/restart playback (i.e. 'pause')\n[%c]\t next track\n[%c]\t back to beginning of track\n[%c]\t pause while looping current sound chunk\n[%c]\t forward\n[%c]\t rewind\n[%c]\t fast forward\n[%c]\t fast rewind\n[%c]\t fine forward\n[%c]\t fine rewind\n[%c]\t volume up\n[%c]\t volume down\n[%c]\t RVA switch\n[%c]\t verbose switch\n[%c]\t this help\n[%c]\t quit\n\n",
		        STOP_KEY, NEXT_KEY, BACK_KEY, PAUSE_KEY, FORWARD_KEY, REWIND_KEY, FAST_FORWARD_KEY, FAST_REWIND_KEY, FINE_FORWARD_KEY, FINE_REWIND_KEY, VOL_UP_KEY, VOL_DOWN_KEY, RVA_KEY, VERBOSE_KEY, HELP_KEY, QUIT_KEY);
	break;
	case FRAME_INDEX_KEY:
		print_frame_index(stderr);
	break;
	default:
	  ;
      }
    }
  }
  return offset;
}

void term_restore(void)
{
  
  if(!term_enable)
    return;

  tcsetattr(0,TCSAFLUSH,&old_tio);
}

#endif

