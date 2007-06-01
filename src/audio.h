/*
	audio: audio output interface

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

/* 
 * Audio 'LIB' defines
 */


#ifndef _MPG123_AUDIO_H_
#define _MPG123_AUDIO_H_

#define AUDIO_OUT_HEADPHONES       0x01
#define AUDIO_OUT_INTERNAL_SPEAKER 0x02
#define AUDIO_OUT_LINE_OUT         0x04

enum {
	DECODE_TEST,
	DECODE_AUDIO,
	DECODE_FILE,
	DECODE_BUFFER,
	DECODE_WAV,
	DECODE_AU,
	DECODE_CDR,
	DECODE_AUDIOFILE
};

#define AUDIO_FORMAT_MASK	  0x100
#define AUDIO_FORMAT_16		  0x100
#define AUDIO_FORMAT_8		  0x000

#define AUDIO_FORMAT_SIGNED_16    0x110
#define AUDIO_FORMAT_UNSIGNED_16  0x120
#define AUDIO_FORMAT_UNSIGNED_8   0x1
#define AUDIO_FORMAT_SIGNED_8     0x2
#define AUDIO_FORMAT_ULAW_8       0x4
#define AUDIO_FORMAT_ALAW_8       0x8

/* 3% rate tolerance */
#define AUDIO_RATE_TOLERANCE	  3



struct audio_info_struct
{
  int fn; /* filenumber */
  void *handle;	/* driver specific pointer */

  long rate;
  long gain;
  int output;

  char *device;
  int channels;
  int format;

};

struct audio_name {
  int  val;
  char *name;
  char *sname;
};


/* ------ Declarations from "audio.c" ------ */

extern void audio_info_struct_init(struct audio_info_struct *);
extern void audio_info_struct_dump(struct audio_info_struct *ai);
extern void audio_capabilities(struct audio_info_struct *);
extern int audio_fit_capabilities(struct audio_info_struct *ai,int c,int r);
extern char *audio_encoding_name(int format);


/* ------ Declarations from "audio_*.c" ------ */

extern int audio_open(struct audio_info_struct *);
extern int audio_get_formats(struct audio_info_struct *);
extern int audio_play_samples(struct audio_info_struct *, unsigned char *,int);
extern void audio_queueflush(struct audio_info_struct *ai);
extern int audio_close(struct audio_info_struct *);

#endif

