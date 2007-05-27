/*
	audio: audio output interface

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp
*/

#include <stdlib.h>
#include "config.h"
#include "mpg123.h"
#include "debug.h"

void audio_info_struct_init(struct audio_info_struct *ai)
{
  ai->fn = -1;
  ai->rate = -1;
  ai->gain = -1;
  ai->output = -1;
  ai->handle = NULL;
  ai->device = NULL;
  ai->channels = -1;
  ai->format = -1;
}


void audio_info_struct_dump(struct audio_info_struct *ai)
{
	fprintf(stderr, "ai->fn=%d\n", ai->fn);
	fprintf(stderr, "ai->handle=%p\n", ai->handle);
	fprintf(stderr, "ai->rate=%ld\n", ai->rate);
	fprintf(stderr, "ai->gain=%ld\n", ai->gain);
	fprintf(stderr, "ai->output=%d\n", ai->output);
	fprintf(stderr, "ai->device='%s'\n", ai->device);
	fprintf(stderr, "ai->channels=%d\n", ai->channels);
	fprintf(stderr, "ai->format=%d\n", ai->format);
}


#define NUM_CHANNELS 2
#define NUM_ENCODINGS 6
#define NUM_RATES 10

struct audio_name audio_val2name[NUM_ENCODINGS+1] = {
 { AUDIO_FORMAT_SIGNED_16  , "signed 16 bit" , "s16 " } ,
 { AUDIO_FORMAT_UNSIGNED_16, "unsigned 16 bit" , "u16 " } ,  
 { AUDIO_FORMAT_UNSIGNED_8 , "unsigned 8 bit" , "u8  " } ,
 { AUDIO_FORMAT_SIGNED_8   , "signed 8 bit" , "s8  " } ,
 { AUDIO_FORMAT_ULAW_8     , "mu-law (8 bit)" , "ulaw " } ,
 { AUDIO_FORMAT_ALAW_8     , "a-law (8 bit)" , "alaw " } ,
 { -1 , NULL }
};

#if 0
static char *channel_name[NUM_CHANNELS] = 
 { "mono" , "stereo" };
#endif

static int channels[NUM_CHANNELS] = { 1 , 2 };
static int rates[NUM_RATES] = { 
	 8000, 11025, 12000, 
	16000, 22050, 24000,
	32000, 44100, 48000,
	8000	/* 8000 = dummy for user forced */

};
static int encodings[NUM_ENCODINGS] = {
 AUDIO_FORMAT_SIGNED_16, 
 AUDIO_FORMAT_UNSIGNED_16,
 AUDIO_FORMAT_UNSIGNED_8,
 AUDIO_FORMAT_SIGNED_8,
 AUDIO_FORMAT_ULAW_8,
 AUDIO_FORMAT_ALAW_8
};

static char capabilities[NUM_CHANNELS][NUM_ENCODINGS][NUM_RATES];

void print_capabilities(struct audio_info_struct *ai)
{
	int j,k,k1=NUM_RATES-1;
	if(param.force_rate) {
		rates[NUM_RATES-1] = param.force_rate;
		k1 = NUM_RATES;
	}
	fprintf(stderr,"\nAudio device: %s\nAudio capabilities:\n        |", ai->device != NULL ? ai->device : "<none>");
	for(j=0;j<NUM_ENCODINGS;j++) {
		fprintf(stderr," %5s |",audio_val2name[j].sname);
	}
	fprintf(stderr,"\n --------------------------------------------------------\n");
	for(k=0;k<k1;k++) {
		fprintf(stderr," %5d  |",rates[k]);
		for(j=0;j<NUM_ENCODINGS;j++) {
			if(capabilities[0][j][k]) {
				if(capabilities[1][j][k])
					fprintf(stderr,"  M/S  |");
				else
					fprintf(stderr,"   M   |");
			}
			else if(capabilities[1][j][k])
				fprintf(stderr,"   S   |");
			else
				fprintf(stderr,"       |");
		}
		fprintf(stderr,"\n");
	}
	fprintf(stderr,"\n");
}


void audio_capabilities(struct audio_info_struct *ai)
{
	int fmts;
	int i,j,k,k1=NUM_RATES-1;
	struct audio_info_struct ai1 = *ai;

        if (param.outmode != DECODE_AUDIO) {
		memset(capabilities,1,sizeof(capabilities));
		return;
	}

	memset(capabilities,0,sizeof(capabilities));
	if(param.force_rate) {
		rates[NUM_RATES-1] = param.force_rate;
		k1 = NUM_RATES;
	}

	/* if audio_open fails, the device is just not capable of anything... */
	if(audio_open(&ai1) < 0) {
		perror("audio");
	}
	else
	{
		for(i=0;i<NUM_CHANNELS;i++) {
			for(j=0;j<NUM_RATES;j++) {
				ai1.channels = channels[i];
				ai1.rate = rates[j];
				fmts = audio_get_formats(&ai1);
				if(fmts < 0)
					continue;
				for(k=0;k<NUM_ENCODINGS;k++) {
					if((fmts & encodings[k]) == encodings[k])
						capabilities[i][k][j] = 1;
				}
			}
		}
		audio_close(&ai1);
	}

	if(param.verbose > 1) print_capabilities(ai);
}

static int rate2num(int r)
{
	int i;
	for(i=0;i<NUM_RATES;i++) 
		if(rates[i] == r)
			return i;
	return -1;
}


static int audio_fit_cap_helper(struct audio_info_struct *ai,int rn,int f0,int f2,int c)
{
	int i;

        if(rn >= 0) {
                for(i=f0;i<f2;i++) {
                        if(capabilities[c][i][rn]) {
                                ai->rate = rates[rn];
                                ai->format = encodings[i];
                                ai->channels = channels[c];
				return 1;
                        }
                }
        }
	return 0;

}

/*
 * c=num of channels of stream
 * r=rate of stream
 * return 0 on error
 */
int audio_fit_capabilities(struct audio_info_struct *ai,int c,int r)
{
	int rn;
	int f0=0;
	
	if(param.force_8bit) f0 = 2; /* skip the 16bit encodings */

	c--; /* stereo=1 ,mono=0 */

	/* force stereo is stronger */
	if(param.force_mono) c = 0;
	if(param.force_stereo) c = 1;

	if(param.force_rate) {
		rn = rate2num(param.force_rate);
		/* 16bit encodings */
		if(audio_fit_cap_helper(ai,rn,f0,2,c)) return 1;
		/* 8bit encodings */
		if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c)) return 1;

		/* try again with different stereoness */
		if(c == 1 && !param.force_stereo)	c = 0;
		else if(c == 0 && !param.force_mono) c = 1;

		/* 16bit encodings */
		if(audio_fit_cap_helper(ai,rn,f0,2,c)) return 1;
		/* 8bit encodings */
		if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c)) return 1;

		error3("Unable to set up %ibit output format with forced rate %li%s!",
		       (param.force_8bit ? 8 : 16),
		       param.force_rate,
		       (param.force_stereo ? " (you forced stereo)" :
		        (param.force_mono ? " (you forced mono)" : "")));
		if(param.verbose <= 1) print_capabilities(ai);
		return 0;
	}

	/* try different rates with 16bit */
	rn = rate2num(r>>0);
	if(audio_fit_cap_helper(ai,rn,f0,2,c))
		return 1;
	rn = rate2num(r>>1);
	if(audio_fit_cap_helper(ai,rn,f0,2,c))
		return 1;
	rn = rate2num(r>>2);
	if(audio_fit_cap_helper(ai,rn,f0,2,c))
		return 1;

	/* try different rates with 8bit */
	rn = rate2num(r>>0);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c))
		return 1;
	rn = rate2num(r>>1);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c))
		return 1;
	rn = rate2num(r>>2);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c))
		return 1;

	/* try again with different stereoness */
	if(c == 1 && !param.force_stereo)	c = 0;
	else if(c == 0 && !param.force_mono) c = 1;

	/* 16bit */
	rn = rate2num(r>>0);
	if(audio_fit_cap_helper(ai,rn,f0,2,c)) return 1;
	rn = rate2num(r>>1);
	if(audio_fit_cap_helper(ai,rn,f0,2,c)) return 1;
	rn = rate2num(r>>2);
	if(audio_fit_cap_helper(ai,rn,f0,2,c)) return 1;

	/* 8bit */
	rn = rate2num(r>>0);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c)) return 1;
	rn = rate2num(r>>1);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c)) return 1;
	rn = rate2num(r>>2);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c)) return 1;

	error2("Unable to set up %ibit output format with any known rate%s!",
	       (param.force_8bit ? 8 : 16),
	       (param.force_stereo ? " (you forced stereo)" :
	        (param.force_mono ? " (you forced mono)" : "")));
	if(param.verbose <= 1) print_capabilities(ai);
	return 0;
}

char *audio_encoding_name(int format)
{
	int i;

	for(i=0;i<NUM_ENCODINGS;i++) {
		if(audio_val2name[i].val == format)
			return audio_val2name[i].name;
	}
	return "Unknown";
}
