/*
	audio_alib.c: audio output for HP-UX using alib

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Erwan Ducroquet
	based on source code from HP (Audio SDK)
*/

/*
 *
 * for mpg123 :
 * hpux:
 *  $(MAKE) \
 *  CC=cc \
 *  LDFLAGS=-L/opt/audio/lib \
 *  AUDIO_LIB=-lAlib \
 *  OBJECTS=decode.o dct64.o \
 *  CFLAGS=-Ae +O3 -DREAL_IS_FLOAT -D_HPUX_SOURCE -DHPUX -I/opt/audio/include \
 *  mpg123
 */

/*
 * Rem:
 * I don't use the "set_rate" and "set_channels" functions
 * these are set directly in the "audio_open" function
 * 
 */

/*
 * For the user :
 * If you launch mpg123 on a XTerm with sound capabilities, it's OK
 * Else, you have to set the environment variable "AUDIO" to the name of
 * an HP Xterm with sound card.
 */

/**************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

#include "config.h"
#include "mpg123.h"
#include "debug.h"

#include "Alib.h"		/* /opt/audio/include */
#include "CUlib.h"		/* /opt/audio/include */

/*
 * 
 * used :
 * 
 * int audio_open(struct audio_info_struct *ai);
 * int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len);
 * int audio_close(struct audio_info_struct *ai);
 * 
 * unused :
 * 
 * int audio_set_rate(struct audio_info_struct *ai);
 * int audio_set_channels(struct audio_info_struct *ai);
 * 
 */

/**************************************************************************/

/*
 * Some things used only for HP Audio
 */

static Audio *audioServer = (Audio *) NULL;
static struct protoent *tcpProtocolEntry;
static ATransID xid;

static void printAudioError(Audio *audio,char *message,int errorCode) {
    char    errorbuff[132];
    AGetErrorText(audio, errorCode, errorbuff, 131);
    fprintf ( stderr, "%s: %s\n", message, errorbuff);
}
static long myHandler(Audio *audio,AErrorEvent *err_event) {
  printAudioError( audio, "Audio error", err_event->error_code ); 
  /* we cannot just do random exists, that messes terminal up
     need proper error propagation in that case for future, setting intflag or such */
  /* exit(1); */
}

/**************************************************************************/

/*
 * Set the fn element of ai
 * Use ai->rate and ai->channels
 * Doesn't set any volume
 */

/* return on error leaves stuff dirty here... */
int audio_open(struct audio_info_struct *ai) {
  AudioAttributes Attribs;
  AudioAttrMask   AttribsMask;
  AGainEntry      gainEntry[4];
  SSPlayParams    playParams;
  SStream	  audioStream;
  AErrorHandler   prevHandler;
  char		  server[1];
  int		  i;
  long            status;

  if(audioServer)
    {error("openAudio: audio already open"); return -1; }

  prevHandler = ASetErrorHandler(myHandler);

  server[0] = '\0';
  audioServer = AOpenAudio( server, NULL );
  if(audioServer==NULL)
    {error("Error: could not open audio\n"); return -1; }

  ai->fn = socket( AF_INET, SOCK_STREAM, 0 );
  if(ai->fn<0)
    {error("Socket creation failed"); return -1; }

  Attribs.type = ATSampled;
  Attribs.attr.sampled_attr.sampling_rate = ai->rate;
  Attribs.attr.sampled_attr.channels	  = ai->channels;
  Attribs.attr.sampled_attr.data_format	  = ADFLin16;
  AttribsMask = ASSamplingRateMask | ASChannelsMask  | ASDataFormatMask;

  gainEntry[0].gain = AUnityGain;
  gainEntry[0].u.o.out_ch  = AOCTMono;
  gainEntry[0].u.o.out_dst = AODTDefaultOutput;

  playParams.gain_matrix.type = AGMTOutput;  /* gain matrix */
  playParams.gain_matrix.num_entries = 1;
  playParams.gain_matrix.gain_entries = gainEntry;
  playParams.play_volume = AUnityGain;       /* play volume */
  playParams.priority = APriorityNormal;     /* normal priority */
  playParams.event_mask = 0;                 /* don't solicit any events */

  xid=APlaySStream(audioServer,AttribsMask,&Attribs,
		   &playParams,&audioStream,NULL);

  status=connect(ai->fn,
		 (struct sockaddr *) &audioStream.tcp_sockaddr,
		 sizeof(struct sockaddr_in) );
  if(status<0){error("Connect failed"); return -1;}

  i=-1;
  tcpProtocolEntry=getprotobyname("tcp");
  setsockopt(ai->fn,tcpProtocolEntry->p_proto,TCP_NODELAY,&i,sizeof(i));

  return ai->fn;
}

/**************************************************************************/

int audio_close(struct audio_info_struct *ai)
{
  close(ai->fn);
  ASetCloseDownMode( audioServer, AKeepTransactions, NULL );
  ACloseAudio( audioServer, NULL );
  audioServer = (Audio *) NULL;
  return 0;
}

/**************************************************************************/

/*
 * very simple
 * deserv to be inline
 */

inline int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
  return write(ai->fn,buf,len*2);
}

/**************************************************************************/

int audio_get_formats(struct audio_info_struct *ai)
{
  return AUDIO_FORMAT_SIGNED_16;
}

void audio_queueflush(struct audio_info_struct *ai)
{
}


/**************************************************************************
 * T H A T ' S    A L L   F O L K S
 **************************************************************************/
