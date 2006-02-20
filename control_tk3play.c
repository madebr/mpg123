/*
 * Control interface to front ends.
 * written/copyrights 1997 by Brian Foutz (and Michael Hipp) 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <limits.h>

#include <sys/socket.h>

#include "control_tk3play.h"

#include "mpg123.h"
#include "buffer.h"

int mode = MODE_STOPPED;
int init = 0;
int framecnt = 0;
AudioInfo sai, oldsai;

extern struct audio_info_struct ai;
extern FILE *filept;
extern int tabsel_123[2][3][16];
extern int buffer_pid;
extern long startFrame;
extern long outscale;
int rewindspeed;

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

int tk3play_sendmsg(int type, float data)
{
  int n;
  n = printf("%d %f\n",type,data);
  fflush(stdout);
  return n;
}

int still_playing_old(void)
{
  int channels, outrate, bufferused;

  channels = sai.stereo ? 2 : 1;
  outrate = sai.frequency*channels*2;
  bufferused = xfermem_get_usedspace(buffermem);

  if (mode == MODE_PLAYING_OLD_FINISHED_DECODING_NEW) {

    return (bufferused/(float)outrate > sai.length*8/(float)sai.bitrate);
  }

  return ((bufferused)/(float)outrate > rd->tell(rd)*8/(float)sai.bitrate);
}

int buffer_used(void) 
{
  if (param.usebuffer)
    return xfermem_get_usedspace(buffermem);
  else
   return 0;
}

float calc_time(void)
{
  int channels, outrate, bufferused, old_channels, old_outrate;
  int new_buf_bytes, old_buf_bytes;
  float old_buf_sec, new_buf_sec, old_total_sec;
  float stream_sec, buffer_sec, time;

  if (!param.usebuffer)
    return rd->tell(rd)*8/(float)sai.bitrate;

  channels = sai.stereo ? 2 : 1;
  old_channels = oldsai.stereo ? 2 : 1;
  outrate = sai.frequency*channels*2;
  old_outrate = oldsai.frequency*old_channels*2;
  bufferused = xfermem_get_usedspace(buffermem);

  buffer_sec = bufferused/(float)outrate;
  time = 0.0;

  if (mode == MODE_PLAYING_AND_DECODING) {

    stream_sec = rd->tell(rd)*8/(float)sai.bitrate;
    time = stream_sec - buffer_sec;
  }

  if (mode == MODE_PLAYING_NOT_DECODING) {

    time = sai.length*8/(float)sai.bitrate - buffer_sec;
  }

  if (mode == MODE_PLAYING_OLD_DECODING_NEW) {

    stream_sec = rd->tell(rd)*8/(float)sai.bitrate;
    new_buf_bytes = stream_sec * outrate;
    old_buf_bytes = bufferused - new_buf_bytes;
    old_buf_sec = old_buf_bytes / (float)old_outrate;
    old_total_sec = oldsai.length*8/(float)oldsai.bitrate;
    time = old_total_sec - old_buf_sec;
  }

  if (mode == MODE_PLAYING_OLD_FINISHED_DECODING_NEW) {

    new_buf_sec = sai.length*8/(float)sai.bitrate;
    new_buf_bytes = new_buf_sec * outrate;
    old_buf_bytes = bufferused - new_buf_bytes;
    old_buf_sec = old_buf_bytes / (float)old_outrate;
    old_total_sec = oldsai.length*8/(float)oldsai.bitrate;
    time = old_total_sec - old_buf_sec;
  }

  return time;
}

int tk3play_handlemsg(struct frame *fr,struct timeval *timeout)
{
  char filename[PATH_MAX];
  fd_set readfds;
  char *line;
  int n, scann, ok;
  int rtype;
  int rdata;
  static int oldmode;

  FD_ZERO(&readfds);
  FD_SET(0,&readfds);
  n = select(fileno(stdin)+1,&readfds,NULL,NULL,timeout);
  if (n == 0) return 0;
  scann = scanf("%d %d",&rtype,&rdata);
  while (scann == 2) {

  switch(rtype) {
  case MSG_CTRL:
    switch(rdata) {

    case PLAY_STOP:
      if (mode != MODE_STOPPED) {
	buffer_resync();
	if (mode == MODE_PLAYING_AND_DECODING ||
            mode == MODE_PLAYING_OLD_DECODING_NEW) {
	  rd->close(rd);
	}
	mode = MODE_STOPPED;
      }
      /* tk3play_sendmsg(MSG_RESPONSE,PLAY_STOP); */
      break;

    case PLAY_PAUSE:
      if (mode != MODE_STOPPED) {
	if (mode == MODE_PAUSED) {
	  buffer_start();
	  mode = oldmode;
	}
	else {
	  oldmode = mode;
	  mode = MODE_PAUSED;
	  buffer_stop();
	}
      }
      /* tk3play_sendmsg(MSG_RESPONSE,PLAY_PAUSE); */
      break;
    }
    break;

  case MSG_SONG:
    fcntl(0,F_SETFL,0);
    line = fgets(filename,PATH_MAX,stdin);
    line = fgets(filename,PATH_MAX,stdin);
    fcntl(0,F_SETFL,O_NONBLOCK);

    if (line == NULL) {
      fprintf(stderr,"Error reading filename!\n");
      exit(1);
    }
    *(filename+strlen(filename)-1)=0;

    if (mode == MODE_PLAYING_AND_DECODING ||
	mode == MODE_PLAYING_OLD_DECODING_NEW) {
      fprintf(stderr,"Still decoding old song, filename ignored\n");
    }

    if (mode == MODE_PLAYING_OLD_FINISHED_DECODING_NEW) {
      fprintf(stderr,"One song has been buffered, ignoring new filename\n");
    }

    if (mode == MODE_STOPPED) {
	mode = MODE_PLAYING_AND_DECODING;
	open_stream(filename,-1);
	init = 1;
	framecnt = 0;
	read_frame_init();
    }

    if (mode == MODE_PLAYING_NOT_DECODING) {
      mode = MODE_PLAYING_OLD_DECODING_NEW;
      open_stream(filename,-1);
      init = 1;
      framecnt = 0;
      read_frame_init();
    }

    /* tk3play_sendmsg(MSG_RESPONSE,MSG_SONG); */
    break;

  case MSG_JUMPTO:
    if (buffer_used() > 0)
       buffer_resync();

    ok = 1;
    if (rdata < framecnt) {
      rd->rewind(rd);
      read_frame_init();
      for (framecnt = 0; ok && framecnt < rdata; framecnt++) {
        ok = read_frame(fr);
	if (fr->lay == 3)
	  set_pointer(512);
      }
    } else {
      for (;ok && framecnt < rdata; framecnt++) {
        ok = read_frame(fr);
	if (fr->lay == 3)
	  set_pointer(512);
      }
    }
    mode = MODE_PLAYING_AND_DECODING;
    break;

  case MSG_HALF:
    param.halfspeed = rdata;
    break;

  case MSG_DOUBLE:
    if (rdata < 0) {
       rewindspeed = -rdata;
       param.doublespeed = 0;
    } else {
       param.doublespeed = rdata;
       rewindspeed = 0;
    }
    break;

  case MSG_SCALE:
    outscale = rdata;
    make_decode_tables(outscale);
    break;

  case MSG_QUIT:
#ifndef OS2
    if (param.usebuffer) {
      buffer_resync();
      xfermem_done_writer (buffermem);
      waitpid (buffer_pid, NULL, 0);
      xfermem_done (buffermem);
    }
    else {
#endif
      audio_flush(param.outmode, &ai);
      free (pcm_sample);
#ifndef OS2
    }
#endif

    if(param.outmode==DECODE_AUDIO)
      audio_close(&ai);

    tk3play_sendmsg(MSG_RESPONSE,MSG_QUIT);
    
    exit( 0 );
  }

  scann = scanf("%d %d",&rtype,&rdata);
  }

  if (scann != -1) {
    fprintf(stderr,"scann = %d\n",scann);
    fprintf(stderr,"Error scanning input control line!\n");
    exit(1);
  }

  return 1;
}

  
void control_tk3play(struct frame *fr) 
{
  struct timeval timeout;
  static int hp = 0;

  fcntl(0,F_SETFL,O_NONBLOCK);

  while(1) {
    if (mode == MODE_PLAYING_AND_DECODING) {
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      if (tk3play_handlemsg(fr,&timeout))
	continue;

      if (!read_frame(fr)) {
        sai.length = rd->tell(rd);
        rd->close(rd);
	tk3play_sendmsg(MSG_FRAMES,framecnt);
        tk3play_sendmsg(MSG_NEXT,0);
        if (param.usebuffer) {
	  mode = MODE_PLAYING_NOT_DECODING;
        } else {
          mode = MODE_STOPPED;
        }
	continue;
      }

      framecnt++;
      if (framecnt < startFrame) {
        if (fr->lay == 3)
          set_pointer(512);
        continue;
      }
      if (param.doublespeed && (framecnt % param.doublespeed)) {
	if (fr->lay == 3)
          set_pointer(512);
      } else {
        play_frame(init,fr);
        if (init) {
	  sai.bitrate = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index] 
                        * 1000;
          sai.frequency = freqs[fr->sampling_frequency];
	  sai.stereo = fr->stereo;
	  sai.type = fr->lay;
	  sai.sample = 16;
	  printf("%d %d %d %d %d %d\n",MSG_INFO,sai.bitrate,sai.frequency,
	         sai.stereo,sai.type,sai.sample);
          fflush(stdout);
	  tk3play_sendmsg(MSG_FRAMES,framecnt);
	  tk3play_sendmsg(MSG_BUFFER,buffer_used());
	  tk3play_sendmsg(MSG_TIME,calc_time());
	  init = startFrame = 0;
        }
      }

      if (param.halfspeed) 
	if (hp--) {
          framecnt--;
	}
        else
	  hp = param.halfspeed - 1;

      if (rewindspeed && (framecnt>1)) {
        rd->back_frame(rd,fr,rewindspeed+1);
	framecnt -= rewindspeed+1;
      }

      if(!(framecnt & 0xf)) {
	tk3play_sendmsg(MSG_FRAMES,framecnt);
	tk3play_sendmsg(MSG_BUFFER,buffer_used());
	tk3play_sendmsg(MSG_TIME,calc_time());
      }

    }

    if (mode == MODE_PLAYING_OLD_DECODING_NEW) {
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      if (tk3play_handlemsg(fr,&timeout))
	continue;

      if (!read_frame(fr)) {
	sai.length = rd->tell(rd);
        rd->close(rd);
	tk3play_sendmsg(MSG_FRAMES,framecnt);
	mode = MODE_PLAYING_OLD_FINISHED_DECODING_NEW;
	continue;
      }
      play_frame(init,fr);
      framecnt++;

      if (init) {
	oldsai = sai;
	sai.bitrate = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index] * 1000;
	sai.frequency = freqs[fr->sampling_frequency];
	sai.stereo = fr->stereo;
	sai.type = fr->lay;
	sai.sample = 16;
	tk3play_sendmsg(MSG_FRAMES,framecnt);
	tk3play_sendmsg(MSG_BUFFER,buffer_used());
	tk3play_sendmsg(MSG_TIME,calc_time());
	init = 0;
      }

      if (!still_playing_old()) {
	mode = MODE_PLAYING_AND_DECODING;
	tk3play_sendmsg(MSG_SONGDONE,0);
	tk3play_sendmsg(MSG_BUFFER,buffer_used());
	tk3play_sendmsg(MSG_TIME,calc_time());
	printf("%d %d %d %d %d %d\n",MSG_INFO,sai.bitrate,sai.frequency,
	       sai.stereo,sai.type,sai.sample);
        fflush(stdout);
      }

      if(!(framecnt & 0xf)) {
	tk3play_sendmsg(MSG_FRAMES,framecnt);
	tk3play_sendmsg(MSG_BUFFER,buffer_used());
	tk3play_sendmsg(MSG_TIME,calc_time());
      }
    }

    if (mode == MODE_PLAYING_NOT_DECODING) {
      timeout.tv_sec = 0;
      timeout.tv_usec = 200000;
      if (tk3play_handlemsg(fr,&timeout))
	continue;

      tk3play_sendmsg(MSG_BUFFER,buffer_used());
      tk3play_sendmsg(MSG_TIME,calc_time());
      if (xfermem_get_usedspace(buffermem) == 0) {
	mode = MODE_STOPPED;
	tk3play_sendmsg(MSG_SONGDONE,0);
      }
    }

    if (mode == MODE_PLAYING_OLD_FINISHED_DECODING_NEW) {
      timeout.tv_sec = 0;
      timeout.tv_usec = 200000;
      if (tk3play_handlemsg(fr,&timeout))
	continue;

      tk3play_sendmsg(MSG_BUFFER,buffer_used());
      tk3play_sendmsg(MSG_TIME,calc_time());
      if (!still_playing_old()) {
	mode = MODE_PLAYING_NOT_DECODING;
	tk3play_sendmsg(MSG_SONGDONE,0);
	printf("%d %d %d %d %d %d\n",MSG_INFO,sai.bitrate,sai.frequency,
	       sai.stereo,sai.type,sai.sample);
        fflush(stdout);
	tk3play_sendmsg(MSG_NEXT,0);
      }
    }

    if (mode == MODE_STOPPED || mode == MODE_PAUSED) {
      while (!tk3play_handlemsg(fr,NULL));
    }

  }     /* while (1) */
}

