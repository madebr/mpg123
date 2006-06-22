
#ifndef _MPG123_CONTROL_TK3PLAY_H_
#define _MPG123_CONTROL_TK3PLAY_H_

typedef struct 
{
        int bitrate;
        int frequency;
        int stereo;
        int type;
        int sample;
        int length;
} AudioInfo;

enum { MSG_CTRL = 0, MSG_BUFFER = 1, MSG_SONG = 2, MSG_QUIT = 3, MSG_NEXT = 4,
	MSG_RESPONSE = 5, MSG_FRAMES = 6, MSG_INFO = 7 ,MSG_TIME = 8,
        MSG_SONGDONE = 9, MSG_JUMPTO = 10, MSG_HALF = 11, MSG_DOUBLE = 12, 
        MSG_SCALE = 13};


/* MSG_CTRL */
enum { PLAY_STOP, PLAY_PAUSE, FORWARD_BEGIN, FORWARD_STEP, FORWARD_END,
	REWIND_BEGIN, REWIND_STEP, REWIND_END };

/* Decoder modes */
enum {MODE_STOPPED, MODE_PLAYING_AND_DECODING, 
      MODE_PLAYING_OLD_DECODING_NEW, MODE_PLAYING_NOT_DECODING,
      MODE_PLAYING_OLD_FINISHED_DECODING_NEW, MODE_PAUSED};

#endif
