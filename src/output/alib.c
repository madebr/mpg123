/*
	alib: audio output for HP-UX using alib

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
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

#include "mpg123.h"

#include <fcntl.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>


#include "Alib.h"		/* /opt/audio/include */
#include "CUlib.h"		/* /opt/audio/include */

/*
 * 
 * used :
 * 
 * int audio_open(audio_output_t *ao);
 * int audio_play_samples(audio_output_t *ao,unsigned char *buf,int len);
 * int audio_close(audio_output_t *ao);
 * 
 * unused :
 * 
 * int audio_set_rate(audio_output_t *ao);
 * int audio_set_channels(audio_output_t *ao);
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
 * Use ao->rate and ao->channels
 * Doesn't set any volume
 */

/* return on error leaves stuff dirty here... */
int audio_open(audio_output_t *ao) {
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

  ao->fn = socket( AF_INET, SOCK_STREAM, 0 );
  if(ao->fn<0)
    {error("Socket creation failed"); return -1; }

  Attribs.type = ATSampled;
  Attribs.attr.sampled_attr.sampling_rate = ao->rate;
  Attribs.attr.sampled_attr.channels	  = ao->channels;
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

  status=connect(ao->fn,
		 (struct sockaddr *) &audioStream.tcp_sockaddr,
		 sizeof(struct sockaddr_in) );
  if(status<0){error("Connect failed"); return -1;}

  i=-1;
  tcpProtocolEntry=getprotobyname("tcp");
  setsockopt(ao->fn,tcpProtocolEntry->p_proto,TCP_NODELAY,&i,sizeof(i));

  return ao->fn;
}

/**************************************************************************/

int audio_close(audio_output_t *ao)
{
  close(ao->fn);
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

inline int audio_play_samples(audio_output_t *ao,unsigned char *buf,int len)
{
  return write(ao->fn,buf,len*2);
}

/**************************************************************************/

int audio_get_formats(audio_output_t *ao)
{
  return AUDIO_FORMAT_SIGNED_16;
}

void audio_queueflush(audio_output_t *ao)
{
}


/**************************************************************************
 * T H A T ' S    A L L   F O L K S
 **************************************************************************/
