/*
	common: anything can happen here... frame reading, output, messages

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp
*/

#include <ctype.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <math.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <fcntl.h>

#include "config.h"

#ifdef READ_MMAP
#include <sys/mman.h>
#ifndef MAP_FAILED
#define MAP_FAILED ( (void *) -1 )
#endif
#endif

#include "mpg123.h"
#include "genre.h"
#include "common.h"
#include "debug.h"

/* bitrates for [mpeg1/2][layer] */
int tabsel_123[2][3][16] = {
   { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
     {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
     {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

   { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};

long freqs[9] = { 44100, 48000, 32000, 22050, 24000, 16000 , 11025 , 12000 , 8000 };

struct bitstream_info bsi;

static int fsizeold=0,ssize;
static unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
static unsigned char *bsbuf=bsspace[1],*bsbufold;
static int bsnum=0;

static unsigned long oldhead = 0;
unsigned long firsthead=0;
#define CBR 0
#define VBR 1
#define ABR 2
int vbr = CBR; /* variable bitrate flag */
int abr_rate = 0;
#ifdef GAPLESS
#include "layer3.h"
#endif
unsigned long track_frames = 0;
/* a limit for number of frames in a track; beyond that unsigned long may not be enough to hold byte addresses */
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef ULONG_MAX
/* hm, is this portable across preprocessors? */
#define ULONG_MAX ((unsigned long)-1)
#endif
#define TRACK_MAX_FRAMES ULONG_MAX/4/1152


/* this could become a struct... */
long lastscale = -1; /* last used scale */
int rva_level[2] = {-1,-1}; /* significance level of stored rva */
float rva_gain[2] = {0,0}; /* mix, album */
float rva_peak[2] = {0,0};

unsigned char *pcm_sample;
int pcm_point = 0;
int audiobufsize = AUDIOBUFSIZE;

#ifdef VARMODESUPPORT
	/*
	 *   This is a dirty hack!  It might burn your PC and kill your cat!
	 *   When in "varmode", specially formatted layer-3 mpeg files are
	 *   expected as input -- it will NOT work with standard mpeg files.
	 *   The reason for this:
	 *   Varmode mpeg files enable my own GUI player to perform fast
	 *   forward and backward functions, and to jump to an arbitrary
	 *   timestamp position within the file.  This would be possible
	 *   with standard mpeg files, too, but it would be a lot harder to
	 *   implement.
	 *   A filter for converting standard mpeg to varmode mpeg is
	 *   available on request, but it's really not useful on its own.
	 *
	 *   Oliver Fromme  <oliver.fromme@heim3.tu-clausthal.de>
	 *   Mon Mar 24 00:04:24 MET 1997
	 */
int varmode = FALSE;
int playlimit;
#endif

static int decode_header(struct frame *fr,unsigned long newhead);

/*
	* trying to parse ID3v2.3 and ID3v2.4 tags... actually, for a start only v2.4 RVA2 frames
	*
	* returns:  0 = read-error
	*          -1 = illegal ID3 header; maybe extended to mean unparseable (to new) header in future
	*           1 = somehow ok...
	*/
static int parse_new_id3(unsigned long first4bytes, struct reader *rds)
{
	#define UNSYNC_FLAG 128
	#define EXTHEAD_FLAG 64
	#define EXP_FLAG 32
	#define FOOTER_FLAG 16
	#define UNKNOWN_FLAGS 15 /* 00001111*/
	unsigned char buf[6];
	unsigned long length=0;
	unsigned char flags = 0;
	int ret = 1;
	unsigned char* tagdata = NULL;
	unsigned char major = first4bytes & 0xff;
	debug1("ID3v2: major tag version: %i", major);
	if(major == 0xff) return -1;
	if(!rds->read_frame_body(rds,buf,6))       /* read more header information */
	return 0;

	if(buf[0] == 0xff) /* major version, will never be 0xff */
	return -1;
	debug1("ID3v2: revision %i", buf[0]);
	/* second new byte are some nice flags, if these are invalid skip the whole thing */
	flags = buf[1];
	debug1("ID3v2: flags 0x%08x", flags);
	/* use 4 bytes from buf to construct 28bit uint value and return 1; return 0 if bytes are not syncsafe */
	#define syncsafe_to_long(buf,res) \
	( \
		(((buf)[0]|(buf)[1]|(buf)[2]|(buf)[3]) & 0x80) ? 0 : \
		(res =  (((unsigned long) (buf)[0]) << 27) \
		     | (((unsigned long) (buf)[1]) << 14) \
		     | (((unsigned long) (buf)[2]) << 7) \
		     |  ((unsigned long) (buf)[3]) \
		,1) \
	)
	/* length-10 or length-20 (footer present); 4 synchsafe integers == 28 bit number  */
	/* we have already read 10 bytes, so left are length or length+10 bytes belonging to tag */
	if(!syncsafe_to_long(buf+2,length)) return -1;
	debug1("ID3v2: tag data length %lu", length);
	/* skip if unknown version/scary flags, parse otherwise */
	if((flags & UNKNOWN_FLAGS) || (major > 4))
	{
		/* going to skip because there are unknown flags set */
		warning2("ID3v2: Won't parse the ID3v2 tag with major version %u and flags 0x%xu - some extra code may be needed", major, flags);
		if(!rds->skip_bytes(rds,length)) /* will not store data in backbuff! */
		ret = 0;
	}
	else
	{
		/* try to interpret that beast */
		if((tagdata = (unsigned char*) malloc(length+1)) != NULL)
		{
			debug("ID3v2: analysing frames...");
			if(rds->read_frame_body(rds,tagdata,length))
			{
				unsigned long tagpos = 0;
				/* going to apply strlen for strings inside frames, make sure that it doesn't overflow! */
				tagdata[length] = 0;
				if(flags & EXTHEAD_FLAG)
				{
					debug("ID3v2: skipping extended header");
					if(!syncsafe_to_long(tagdata, tagpos)) ret = -1;
				}
				if(ret >= 0)
				{
					char id[5];
					unsigned long framesize;
					unsigned long fflags; /* need 16 bits, actually */
					id[4] = 0;
					/* pos now advanced after ext head, now a frame has to follow */
					while(tagpos < length-10) /* I want to read at least a full header */
					{
						int tt = -1;
						int i = 0;
						unsigned long pos = tagpos;
						/* level 1,2,3 - 0 is info from lame/info tag! */
						const char rva_type[3][5] = { "COMM", "TXXX", "RVA2" }; /* ordered with ascending significance */
						/* we may have entered the padding zone or any other strangeness: check if we have valid frame id characters */
						for(; i< 4; ++i) if( !( ((tagdata[tagpos+i] > 47) && (tagdata[tagpos+i] < 58))
						                     || ((tagdata[tagpos+i] > 64) && (tagdata[tagpos+i] < 91)) ) )
						{
							debug1("ID3v2: real tag data apparently ended after %lu bytes", tagpos);
							ret = -1;
							break;
						}
						if(ret >= 0)
						{
							/* 4 bytes id */
							strncpy(id, (char) tagdata+pos, 4);
							pos += 4;
							/* size as 32 syncsafe bits */
							if(!syncsafe_to_long(tagdata+pos, framesize))
							{
								ret = -1;
								error("ID3v2: non-syncsafe frame size, aborting");
								break;
							}
							debug2("ID3v2: %s frame of size %lu", id, framesize);
							tagpos += 10 + framesize; /* the important advancement in whole tag */
							pos += 4;
							fflags = (((unsigned long) tagdata[pos]) << 8) | ((unsigned long) tagdata[pos+1]);
							pos += 2;
							/* for sanity, after full parsing tagpos should be == pos */
							/* debug4("ID3v2: found %s frame, size %lu (as bytes: 0x%08lx), flags 0x%016lx", id, framesize, framesize, fflags); */
							/* %0abc0000 %0h00kmnp */
							#define BAD_FFLAGS (unsigned long) 36784
							#define PRES_TAG_FFLAG 16384
							#define PRES_FILE_FFLAG 8192
							#define READ_ONLY_FFLAG 4096
							#define GROUP_FFLAG 64
							#define COMPR_FFLAG 8
							#define ENCR_FFLAG 4
							#define UNSYNC_FFLAG 2
							#define DATLEN_FFLAG 1
							/* shall not or want not handle these */
							if(fflags & (BAD_FFLAGS | COMPR_FFLAG | ENCR_FFLAG))
							{
								warning("ID3v2: skipping invalid/unsupported frame");
								continue;
							}
							
							for(i = 0; i < 4; ++i)
							if(!strncmp(rva_type[i], id, 4)){ tt = i+1; break; }
							
							if(tt > 0) /* 1,2,3 */
							{
								int rva_mode = -1; /* mix / album */
								unsigned long realsize = framesize;
								unsigned char* realdata = tagdata+pos;
								if((flags & UNSYNC_FLAG) || (fflags & UNSYNC_FFLAG))
								{
									unsigned long ipos = 0;
									unsigned long opos = 0;
									debug("Id3v2: going to de-unsync the frame data");
									/* de-unsync: FF00 -> FF; real FF00 is simply represented as FF0000 ... */
									/* damn, that means I have to delete bytes from withing the data block... thus need temporal storage */
									/* standard mandates that de-unsync should always be safe if flag is set */
									realdata = (unsigned char*) malloc(framesize); /* will need <= bytes */
									if(realdata == NULL)
									{
										error("ID3v2: unable to allocate working buffer for de-unsync");
										continue;
									}
									/* now going byte per byte through the data... */
									realdata[0] = tagdata[pos];
									opos = 1;
									for(ipos = pos+1; ipos < pos+framesize; ++ipos)
									{
										if(!((tagdata[ipos] == 0) && (tagdata[ipos-1] == 0xff)))
										{
											realdata[opos++] = tagdata[ipos];
										}
									}
									realsize = opos;
									debug2("ID3v2: de-unsync made %lu out of %lu bytes", realsize, framesize);
								}
								pos = 0; /* now at the beginning again... */
								switch(tt)
								{
									case 1: /* a comment that perhaps is a RVA / RVA_ALBUM/AUDIOPHILE / RVA_MIX/RADIO one */
									{
										/* Text encoding          $xx */
										/* Language               $xx xx xx */
										if(realdata[0] == 0) /* only latin1 / ascii */
										{
											/* don't care about language */
											pos = 4;
											if(   !strcasecmp((char*)realdata+pos, "rva")
											   || !strcasecmp((char*)realdata+pos, "rva_mix")
											   || !strcasecmp((char*)realdata+pos, "rva_radio"))
											rva_mode = 0;
											else if(   !strcasecmp((char*)realdata+pos, "rva_album")
											        || !strcasecmp((char*)realdata+pos, "rva_audiophile")
											        || !strcasecmp((char*)realdata+pos, "rva_user"))
											rva_mode = 1;
											if((rva_mode > -1) && (rva_level[rva_mode] <= tt))
											{
												char* comstr;
												size_t comsize = realsize-4-(strlen((char*)realdata+pos)+1);
												debug1("ID3v2: evaluating %s data for RVA", realdata+pos);
												if((comstr = (char*) malloc(comsize+1)) != NULL)
												{
													memcpy(comstr,realdata+realsize-comsize, comsize);
													comstr[comsize] = 0;
													rva_gain[rva_mode] = atof(comstr);
													debug1("ID3v2: RVA value %fdB", rva_gain[rva_mode]);
													rva_peak[rva_mode] = 0;
													rva_level[rva_mode] = tt;
													free(comstr);
												}
												else error("could not allocate memory for rva comment interpretation");
											}
										}
									}
									break;
									case 2: /* perhaps foobar2000's work */
									{
										/* Text encoding          $xx */
										if(realdata[0] == 0) /* only latin1 / ascii for now */
										{
											int is_peak = 0;
											pos = 1;
											
											if(!strncasecmp((char*)realdata+pos, "replaygain_track_",17))
											{
												debug("ID3v2: track gain/peak");
												rva_mode = 0;
												if(!strcasecmp((char*)realdata+pos, "replaygain_track_peak")) is_peak = 1;
												else if(strcasecmp((char*)realdata+pos, "replaygain_track_gain")) rva_mode = -1;
											}
											else
											if(!strncasecmp((char*)realdata+pos, "replaygain_album_",17))
											{
												debug("ID3v2: album gain/peak");
												rva_mode = 1;
												if(!strcasecmp((char*)realdata+pos, "replaygain_album_peak")) is_peak = 1;
												else if(strcasecmp((char*)realdata+pos, "replaygain_album_gain")) rva_mode = -1;
											}
											if((rva_mode > -1) && (rva_level[rva_mode] <= tt))
											{
												char* comstr;
												size_t comsize = realsize-1-(strlen((char*)realdata+pos)+1);
												debug1("ID3v2: evaluating %s data for RVA", realdata+pos);
												if((comstr = (char*) malloc(comsize+1)) != NULL)
												{
													memcpy(comstr,realdata+realsize-comsize, comsize);
													comstr[comsize] = 0;
													if(is_peak)
													{
														rva_peak[rva_mode] = atof(comstr);
														debug1("ID3v2: RVA peak %fdB", rva_peak[rva_mode]);
													}
													else
													{
														rva_gain[rva_mode] = atof(comstr);
														debug1("ID3v2: RVA gain %fdB", rva_gain[rva_mode]);
													}
													rva_level[rva_mode] = tt;
													free(comstr);
												}
												else error("could not allocate memory for rva comment interpretation");
											}
										}
									}
									break;
									case 3: /* "the" RVA tag */
									{
										#ifdef HAVE_INTTYPES_H
										/* starts with null-terminated identification */
										debug1("ID3v2: RVA2 identification \"%s\"", realdata);
										/* default: some individual value, mix mode */
										rva_mode = 0;
										if( !strncasecmp((char*)realdata, "album", 5)
										    || !strncasecmp((char*)realdata, "audiophile", 10)
										    || !strncasecmp((char*)realdata, "user", 4))
										rva_mode = 1;
										if(rva_level[rva_mode] <= tt)
										{
											pos += strlen((char*) realdata) + 1;
											if(realdata[pos] == 1)
											{
												++pos;
												/* only handle master channel */
												debug("ID3v2: it is for the master channel");
												/* two bytes adjustment, one byte for bits representing peak - n bytes for peak */
												/* 16 bit signed integer = dB * 512 */
												rva_gain[rva_mode] = (float) ((((int16_t) realdata[pos]) << 8) | ((int16_t) realdata[pos+1])) / 512;
												pos += 2;
												debug1("ID3v2: RVA value %fdB", rva_gain[rva_mode]);
												/* heh, the peak value is represented by a number of bits - but in what manner? Skipping that part */
												rva_peak[rva_mode] = 0;
												rva_level[rva_mode] = tt;
											}
										}
										#else
										warning("ID3v2: Cannot parse RVA2 value because I don't have a guaranteed 16 bit signed integer type");
										#endif
									}
									break;
									default: error1("ID3v2: unknown rva_type %i", tt);
								}
								if((flags & UNSYNC_FLAG) || (fflags & UNSYNC_FFLAG)) free(realdata);
							}
							#undef BAD_FFLAGS
							#undef PRES_TAG_FFLAG
							#undef PRES_FILE_FFLAG
							#undef READ_ONLY_FFLAG
							#undef GROUP_FFLAG
							#undef COMPR_FFLAG
							#undef ENCR_FFLAG
							#undef UNSYNC_FFLAG
							#undef DATLEN_FFLAG
						}
						else break;
					}
				}
			}
			else
			{
				error("ID3v2: Duh, not able to read ID3v2 tag data.");
				ret = 0;
			}
			free(tagdata);
		}
		else
		{
			error1("ID3v2Arrg! Unable to allocate %lu bytes for interpreting ID3v2 data - trying to skip instead.", length);
			if(!rds->skip_bytes(rds,length)) /* will not store data in backbuff! */
			ret = 0;
		}
	}
	/* skip footer if present */
	if((flags & FOOTER_FLAG) && (!rds->skip_bytes(rds,length))) ret = 0;
	return ret;
	#undef UNSYNC_FLAG
	#undef EXTHEAD_FLAG
	#undef EXP_FLAG
	#undef FOOTER_FLAG
	#undef UNKOWN_FLAGS
}

#ifdef GAPLESS
/* take into account: channels, bytes per sample, resampling (integer samples!) */
unsigned long samples_to_bytes(unsigned long s, struct frame *fr , struct audio_info_struct* ai)
{
	/* rounding positive number... */
	double sammy, samf;
	sammy = (1.0*s) * (1.0*ai->rate)/freqs[fr->sampling_frequency];
	debug4("%lu samples to bytes with freq %li (ai.rate %li); sammy %f", s, freqs[fr->sampling_frequency], ai->rate, sammy);
	samf = floor(sammy);
	return (unsigned long)
		(((ai->format & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_16) ? 2 : 1)
		* ai->channels
		* (int) (((sammy - samf) < 0.5) ? samf : ( sammy-samf > 0.5 ? samf+1 : ((unsigned long) samf % 2 == 0 ? samf : samf + 1)));
}
#endif

void audio_flush(int outmode, struct audio_info_struct *ai)
{
	#ifdef GAPLESS
	if(param.gapless) layer3_gapless_buffercheck();
	#endif
	if(pcm_point)
	{
		switch(outmode)
		{
			case DECODE_FILE:
				write (OutputDescriptor, pcm_sample, pcm_point);
			break;
			case DECODE_AUDIO:
				audio_play_samples (ai, pcm_sample, pcm_point);
			break;
			case DECODE_BUFFER:
				write (buffer_fd[1], pcm_sample, pcm_point);
			break;
			case DECODE_WAV:
			case DECODE_CDR:
			case DECODE_AU:
				wav_write(pcm_sample, pcm_point);
			break;
		}
		pcm_point = 0;
	}
}

#if !defined(WIN32) && !defined(GENERIC)
void (*catchsignal(int signum, void(*handler)()))()
{
  struct sigaction new_sa;
  struct sigaction old_sa;

#ifdef DONT_CATCH_SIGNALS
  fprintf (stderr, "Not catching any signals.\n");
  return ((void (*)()) -1);
#endif

  new_sa.sa_handler = handler;
  sigemptyset(&new_sa.sa_mask);
  new_sa.sa_flags = 0;
  if (sigaction(signum, &new_sa, &old_sa) == -1)
    return ((void (*)()) -1);
  return (old_sa.sa_handler);
}
#endif

void read_frame_init (void)
{
	oldhead = 0;
	firsthead = 0;
	vbr = CBR;
	abr_rate = 0;
	track_frames = 0;
	rva_level[0] = -1;
	rva_level[1] = -1;
	#ifdef GAPLESS
	/* one can at least skip the delay at beginning - though not add it at end since end is unknown */
	if(param.gapless) layer3_gapless_init(DECODER_DELAY+GAP_SHIFT, 0);
	#endif
}

#define free_format_header(head) ( ((head & 0xffe00000) == 0xffe00000) && ((head>>17)&3) && (((head>>12)&0xf) == 0x0) && (((head>>10)&0x3) != 0x3 ))

/* compiler is smart enought to inline this one or should I really do it as macro...? */
int head_check(unsigned long head)
{
	if
	(
		/* first 11 bits are set to 1 for frame sync */
		((head & 0xffe00000) != 0xffe00000)
		||
		/* layer: 01,10,11 is 1,2,3; 00 is reserved */
		(!((head>>17)&3))
		||
		/* 1111 means bad bitrate */
		(((head>>12)&0xf) == 0xf)
		||
		/* 0000 means free format... */
		(((head>>12)&0xf) == 0x0)
		||
		/* sampling freq: 11 is reserved */
		(((head>>10)&0x3) == 0x3 )
		/* here used to be a mpeg 2.5 check... re-enabled 2.5 decoding due to lack of evidence that it is really not good */
	)
	{
		return FALSE;
	}
	/* if no check failed, the header is valid (hopefully)*/
	else
	{
		return TRUE;
	}
}

static void do_rva()
{
	if(param.rva != -1)
	{
		int rt = 0;
		/* Should one assume a zero RVA as no RVA? */
		if(param.rva == 1 && rva_level[1] != -1) rt = 1;
		if(rva_level[rt] != -1)
		{
			long newscale = outscale*pow(10,rva_gain[rt]/20);
			debug1("doing RVA with gain %f", rva_gain[rt]);
			/* if peak is unknown (== 0) this check won't hurt */
			if((rva_peak[rt]*newscale) > MAXOUTBURST)
			{
				newscale = (long) ((float) MAXOUTBURST/rva_peak[rt]);
				warning2("limiting scale value to %li to prevent clipping with indicated peak factor of %f", newscale, rva_peak[rt]);
			}
			if(lastscale < 0) lastscale = outscale;
			if(newscale != lastscale)
			{
				debug3("changing scale value from %li to %li (peak estimated to %li)", lastscale, newscale, (long) (newscale*rva_peak[rt]));
				make_decode_tables(newscale);
				lastscale = newscale;
			}
		}
		else
		{
			warning("no RVA value found");
		}
	}
}


/*****************************************************************

 * read next frame
 */
int read_frame(struct frame *fr)
{
	/* TODO: rework this thing */
  unsigned long newhead;
  static unsigned char ssave[34];

  fsizeold=fr->framesize;       /* for Layer3 */

  if (param.halfspeed) {
    static int halfphase = 0;
    if (halfphase--) {
      bsi.bitindex = 0;
      bsi.wordpointer = (unsigned char *) bsbuf;
      if (fr->lay == 3)
        memcpy (bsbuf, ssave, ssize);
      return 1;
    }
    else
      halfphase = param.halfspeed - 1;
  }

read_again:
	if(!rd->head_read(rd,&newhead))
	{
		return FALSE;
	}
	/* this if wrap looks like dead code... */
  if(1 || oldhead != newhead || !oldhead)
  {

init_resync:

    fr->header_change = 2;
    if(oldhead) {
      if((oldhead & 0xc00) == (newhead & 0xc00)) {
        if( (oldhead & 0xc0) == 0 && (newhead & 0xc0) == 0)
    	  fr->header_change = 1; 
        else if( (oldhead & 0xc0) > 0 && (newhead & 0xc0) > 0)
	  fr->header_change = 1;
      }
    }

#ifdef SKIP_JUNK
	/* watch out for junk/tags on beginning of stream by invalid header */
	if(!firsthead && !head_check(newhead) ) {
		int i;
		if(free_format_header(newhead))
		{
			error1("Header 0x%08lx seems to indicate a free format stream; I do not handle that yet", newhead);
			return 0;
		}

		if(!param.quiet) fprintf(stderr,"Note: Junk at the beginning (0x%08lx)\n",newhead);
		/* check for id3v2; first three bytes (of 4) are "ID3" */
		if((newhead & (unsigned long) 0xffffff00) == (unsigned long) 0x49443300)
		{
			int id3length = 0;
			if(!param.quiet) fprintf(stderr, "Note: Oh, it's just an ID3V2 tag...\n");
			id3length = parse_new_id3(newhead, rd);
			goto read_again;
		}
		/* I even saw RIFF headers at the beginning of MPEG streams ;( */
		if(newhead == ('R'<<24)+('I'<<16)+('F'<<8)+'F') {
			if(!param.quiet) fprintf(stderr, "Note: Looks like a RIFF header.\n");
			if(!rd->head_read(rd,&newhead))
				return 0;
			while(newhead != ('d'<<24)+('a'<<16)+('t'<<8)+'a') {
				if(!rd->head_shift(rd,&newhead))
					return 0;
			}
			if(!rd->head_read(rd,&newhead))
				return 0;
			if(!param.quiet) fprintf(stderr,"Note: Skipped RIFF header!\n");
			goto read_again;
		}
		/* unhandled junk... just continue search for a header */
		/* step in byte steps through next 64K */
		for(i=0;i<65536;i++) {
			if(!rd->head_shift(rd,&newhead))
				return 0;
			if(head_check(newhead))
				break;
		}
		if(i == 65536) {
			if(!param.quiet) error("Giving up searching valid MPEG header after 64K of junk.");
			return 0;
		}
		/* 
		 * should we additionaly check, whether a new frame starts at
		 * the next expected position? (some kind of read ahead)
		 * We could implement this easily, at least for files.
		 */
	}
#endif
    /* why has this head check been avoided here before? */
    if(!head_check(newhead))
    {
      if(!firsthead && free_format_header(newhead))
      {
        error1("Header 0x%08lx seems to indicate a free format stream; I do not handle that yet", newhead);
        return 0;
      }
    /* and those ugly ID3 tags */
      if((newhead & 0xffffff00) == ('T'<<24)+('A'<<16)+('G'<<8)) {
           rd->skip_bytes(rd,124);
	   if (!param.quiet)
             fprintf(stderr,"Note: Skipped ID3 Tag!\n");
           goto read_again;
      }
      if (!param.quiet)
      {
        fprintf(stderr,"Note: Illegal Audio-MPEG-Header 0x%08lx at offset 0x%lx.\n",
              newhead,rd->tell(rd)-4);
        if((newhead & 0xffffff00) == ('b'<<24)+('m'<<16)+('p'<<8))
        fprintf(stderr,"Note: Could be a BMP album art.\n");
      }
      if (param.tryresync) {
        int try = 0;
        /* TODO: make this more robust, I'd like to cat two mp3 fragments together (in a dirty way) and still have mpg123 beign able to decode all it somehow. */
        if(!param.quiet) fprintf(stderr, "Note: Trying to resync...\n");
            /* Read more bytes until we find something that looks
               reasonably like a valid header.  This is not a
               perfect strategy, but it should get us back on the
               track within a short time (and hopefully without
               too much distortion in the audio output).  */
        do {
          try++;
          if(!rd->head_shift(rd,&newhead))
		return 0;
          debug2("resync try %i, got newhead 0x%08lx", try, newhead);
          if (!oldhead)
          {
            debug("going to init_resync...");
            goto init_resync;       /* "considered harmful", eh? */
          }

        } while ((newhead & HDRCMPMASK) != (oldhead & HDRCMPMASK)
              && (newhead & HDRCMPMASK) != (firsthead & HDRCMPMASK));
        if (!param.quiet)
          fprintf (stderr, "Note: Skipped %d bytes in input.\n", try);
      }
      else
      {
        error("not attempting to resync...");
        return (0);
      }
    }

    if (!firsthead) {
      if(!decode_header(fr,newhead))
      {
         error("decode header failed before first valid one, going to read again");
         goto read_again;
      }
    }
    else
      if(!decode_header(fr,newhead))
      {
        error("decode header failed - goto resync");
        /* return 0; */
        goto init_resync;
      }
  }
  else
    fr->header_change = 0;

  /* flip/init buffer for Layer 3 */
  bsbufold = bsbuf;
  bsbuf = bsspace[bsnum]+512;
  bsnum = (bsnum + 1) & 1;
  /* read main data into memory */
	/* 0 is error! */
	if(!rd->read_frame_body(rd,bsbuf,fr->framesize))
		return 0;
	if(!firsthead)
	{
		/* following stuff is actually layer3 specific (in practice, not in theory) */
		if(fr->lay == 3)
		{
			/*
				going to look for Xing or Info at some position after the header
				                                    MPEG 1  MPEG 2/2.5 (LSF)
				Stereo, Joint Stereo, Dual Channel  32      17
				Mono                                17       9
				
				Also, how to avoid false positives? I guess I should interpret more of the header to rule that out(?).
				I hope that ensuring all zeros until tag start is enough.
			*/
			size_t lame_offset = (fr->stereo == 2) ? (fr->lsf ? 17 : 32 ) : (fr->lsf ? 9 : 17);
			if(fr->framesize >= 120+lame_offset) /* traditional Xing header is 120 bytes */
			{
				size_t i;
				int lame_type = 0;
				/* only search for tag when all zero before it (apart from checksum) */
				for(i=2; i < lame_offset; ++i) if(bsbuf[i] != 0) break;
				if(i == lame_offset)
				{
					if
					(
					       (bsbuf[lame_offset] == 'I')
						&& (bsbuf[lame_offset+1] == 'n')
						&& (bsbuf[lame_offset+2] == 'f')
						&& (bsbuf[lame_offset+3] == 'o')
					)
					{
						lame_type = 1; /* We still have to see what there is */
					}
					else if
					(
					       (bsbuf[lame_offset] == 'X')
						&& (bsbuf[lame_offset+1] == 'i')
						&& (bsbuf[lame_offset+2] == 'n')
						&& (bsbuf[lame_offset+3] == 'g')
					)
					{
						lame_type = 2;
						vbr = VBR; /* Xing header means always VBR */
					}
					if(lame_type)
					{
						unsigned long xing_flags;
						
						/* we have one of these headers... */
						if(!param.quiet) fprintf(stderr, "Note: Xing/Lame/Info header detected\n");
						/* now interpret the Xing part, I have 120 bytes total for sure */
						/* there are 4 bytes for flags, but only the last byte contains known ones */
						lame_offset += 4; /* now first byte after Xing/Name */
						/* 4 bytes dword for flags */
						#define make_long(a, o) ((((unsigned long) a[o]) << 24) | (((unsigned long) a[o+1]) << 16) | (((unsigned long) a[o+2]) << 8) | ((unsigned long) a[o+3]))
						/* 16 bit */
						#define make_short(a,o) ((((unsigned short) a[o]) << 8) | ((unsigned short) a[o+1]))
						xing_flags = make_long(bsbuf, lame_offset);
						lame_offset += 4;
						debug1("Xing: flags 0x%08lx", xing_flags);
						if(xing_flags & 1) /* frames */
						{
							/*
								In theory, one should use that value for skipping...
								When I know the exact number of samples I could simply count in audio_flush,
								but that's problematic with seeking and such.
								I still miss the real solution for detecting the end.
							*/
							track_frames = make_long(bsbuf, lame_offset);
							if(track_frames > TRACK_MAX_FRAMES) track_frames = 0; /* endless stream? */
							#ifdef GAPLESS
							/* if no further info there, remove/add at least the decoder delay */
							if(param.gapless)
							{
								unsigned long length = track_frames * spf(fr);
								if(length > 1)
								layer3_gapless_init(DECODER_DELAY+GAP_SHIFT, length+DECODER_DELAY+GAP_SHIFT);
							}
							#endif
							debug1("Xing: %lu frames", track_frames);
							lame_offset += 4;
						}
						if(xing_flags & 0x2) /* bytes */
						{
							#ifdef DEBUG
							unsigned long xing_bytes = make_long(bsbuf, lame_offset);
							debug1("Xing: %lu bytes", xing_bytes);
							#endif
							lame_offset += 4;
						}
						if(xing_flags & 0x4) /* TOC */
						{
							lame_offset += 100; /* just skip */
						}
						if(xing_flags & 0x8) /* VBR quality */
						{
							#ifdef DEBUG
							unsigned long xing_quality = make_long(bsbuf, lame_offset);
							debug1("Xing: quality = %lu", xing_quality);
							#endif
							lame_offset += 4;
						}
						/* I guess that either 0 or LAME extra data follows */
						/* there may this crc16 be floating around... (?) */
						if(bsbuf[lame_offset] != 0)
						{
							unsigned char lame_vbr;
							float replay_gain[2] = {0,0};
							float peak = 0;
							float gain_offset = 0; /* going to be +6 for old lame that used 83dB */
							char nb[10];
							memcpy(nb, bsbuf+lame_offset, 9);
							nb[9] = 0;
							debug1("Info: Encoder: %s", nb);
							if(!strncmp("LAME", nb, 4))
							{
								gain_offset = 6;
								debug("TODO: finish lame detetcion...");
							}
							lame_offset += 9;
							/* the 4 big bits are tag revision, the small bits vbr method */
							lame_vbr = bsbuf[lame_offset] & 15;
							debug1("Info: rev %u", bsbuf[lame_offset] >> 4);
							debug1("Info: vbr mode %u", lame_vbr);
							lame_offset += 1;
							switch(lame_vbr)
							{
								/* from rev1 proposal... not sure if all good in practice */
								case 1:
								case 8: vbr = CBR; break;
								case 2:
								case 9: vbr = ABR; break;
								default: vbr = VBR; /* 00==unknown is taken as VBR */
							}
							/* skipping: lowpass filter value */
							lame_offset += 1;
							/* replaygain */
							/* 32bit float: peak amplitude -- why did I parse it as int before??*/
							/* Ah, yes, lame seems to store it as int since some day in 2003; I've only seen zeros anyway until now, bah! */
							if
							(
								   (bsbuf[lame_offset] != 0)
								|| (bsbuf[lame_offset+1] != 0)
								|| (bsbuf[lame_offset+2] != 0)
								|| (bsbuf[lame_offset+3] != 0)
							)
							{
								debug("Wow! Is there _really_ a non-zero peak value? Now is it stored as float or int - how should I know?");
								peak = *(float*) (bsbuf+lame_offset);
							}
							debug1("Info: peak = %f (I won't use this)", peak);
							peak = 0; /* until better times arrived */
							lame_offset += 4;
							/*
								ReplayGain values - lame only writes radio mode gain...
								16bit gain, 3 bits name, 3 bits originator, sign (1=-, 0=+), dB value*10 in 9 bits (fixed point)
								ignore the setting if name or originator == 000!
								radio 0 0 1 0 1 1 1 0 0 1 1 1 1 1 0 1
								audiophile 0 1 0 0 1 0 0 0 0 0 0 1 0 1 0 0
							*/
							
							for(i =0; i < 2; ++i)
							{
								unsigned char origin = (bsbuf[lame_offset] >> 2) & 0x7; /* the 3 bits after that... */
								if(origin != 0)
								{
									unsigned char gt = bsbuf[lame_offset] >> 5; /* only first 3 bits */
									if(gt == 1) gt = 0; /* radio */
									else if(gt == 2) gt = 1; /* audiophile */
									else continue;
									/* get the 9 bits into a number, divide by 10, multiply sign... happy bit banging */
									replay_gain[0] = ((bsbuf[lame_offset] & 0x2) ? -0.1 : 0.1) * (make_short(bsbuf, lame_offset) & 0x1f);
								}
								lame_offset += 2;
							}
							debug1("Info: Radio Gain = %03.1fdB", replay_gain[0]);
							debug1("Info: Audiophile Gain = %03.1fdB", replay_gain[1]);
							for(i=0; i < 2; ++i)
							{
								if(rva_level[i] <= 0)
								{
									rva_peak[i] = 0; /* at some time the parsed peak should be used */
									rva_gain[i] = replay_gain[i];
									rva_level[i] = 0;
								}
							}
							lame_offset += 1; /* skipping encoding flags byte */
							if(vbr == ABR)
							{
								abr_rate = bsbuf[lame_offset];
								debug1("Info: ABR rate = %u", abr_rate);
							}
							lame_offset += 1;
							/* encoder delay and padding, two 12 bit values... lame does write them from int ...*/
							#ifdef GAPLESS
							if(param.gapless)
							{
								/*
									Temporary hack that doesn't work with seeking and also is not waterproof but works most of the time;
									in future the lame delay/padding and frame number info should be passed to layer3.c and the junk samples avoided at the source.
								*/
								unsigned long length = track_frames * spf(fr);
								unsigned long skipbegin = DECODER_DELAY + ((((int) bsbuf[lame_offset]) << 4) | (((int) bsbuf[lame_offset+1]) >> 4));
								unsigned long skipend = -DECODER_DELAY + (((((int) bsbuf[lame_offset+1]) << 8) | (((int) bsbuf[lame_offset+2]))) & 0xfff);
								debug3("preparing gapless mode for layer3: length %lu, skipbegin %lu, skipend %lu", length, skipbegin, skipend);
								if(length > 1)
								layer3_gapless_init(skipbegin+GAP_SHIFT, (skipend < length) ? length-skipend+GAP_SHIFT : length+GAP_SHIFT);
							}
							#endif
						}
						/* switch buffer back ... */
						bsbuf = bsspace[bsnum]+512;
						bsnum = (bsnum + 1) & 1;
						goto read_again;
					}
				}
			}
		} /* end block for Xing/Lame/Info tag */
		firsthead = newhead; /* _now_ it's time to store it... the first real header */
		/* now adjust volume */
		do_rva();
	}
  bsi.bitindex = 0;
  bsi.wordpointer = (unsigned char *) bsbuf;

  if (param.halfspeed && fr->lay == 3)
    memcpy (ssave, bsbuf, ssize);

  return 1;

}

/****************************************
 * HACK,HACK,HACK: step back <num> frames
 * can only work if the 'stream' isn't a real stream but a file
 */
int back_frame(struct reader *rds,struct frame *fr,int num)
{
  long bytes;
  unsigned long newhead;
  
  if(!firsthead)
    return 0;
  
  bytes = (fr->framesize+8)*(num+2);
  
  if(rds->back_bytes(rds,bytes) < 0)
    return -1;
  if(!rds->head_read(rds,&newhead))
    return -1;
  
  while( (newhead & HDRCMPMASK) != (firsthead & HDRCMPMASK) ) {
    if(!rds->head_shift(rds,&newhead))
      return -1;
  }
  
  if(rds->back_bytes(rds,4) <0)
    return -1;

  read_frame(fr);
  read_frame(fr);
  
  if(fr->lay == 3) {
    set_pointer(512);
  }
  
  return 0;
}


/*
 * decode a header and write the information
 * into the frame structure
 */
static int decode_header(struct frame *fr,unsigned long newhead)
{
    if(!head_check(newhead))
    {
      error("tried to decode obviously invalid header");
      return 0;
    }
    if( newhead & (1<<20) ) {
      fr->lsf = (newhead & (1<<19)) ? 0x0 : 0x1;
      fr->mpeg25 = 0;
    }
    else {
      fr->lsf = 1;
      fr->mpeg25 = 1;
    }
    
    if (!param.tryresync || !oldhead ||
        (((oldhead>>19)&0x3) ^ ((newhead>>19)&0x3))) {
          /* If "tryresync" is false, assume that certain
             parameters do not change within the stream!
	     Force an update if lsf or mpeg25 settings
	     have changed. */
      fr->lay = 4-((newhead>>17)&3);
      if( ((newhead>>10)&0x3) == 0x3) {
        error("Stream error");
        exit(1);
      }
      if(fr->mpeg25) {
        fr->sampling_frequency = 6 + ((newhead>>10)&0x3);
      }
      else
        fr->sampling_frequency = ((newhead>>10)&0x3) + (fr->lsf*3);
      fr->error_protection = ((newhead>>16)&0x1)^0x1;
    }

    fr->bitrate_index = ((newhead>>12)&0xf);
    fr->padding   = ((newhead>>9)&0x1);
    fr->extension = ((newhead>>8)&0x1);
    fr->mode      = ((newhead>>6)&0x3);
    fr->mode_ext  = ((newhead>>4)&0x3);
    fr->copyright = ((newhead>>3)&0x1);
    fr->original  = ((newhead>>2)&0x1);
    fr->emphasis  = newhead & 0x3;

    fr->stereo    = (fr->mode == MPG_MD_MONO) ? 1 : 2;

    oldhead = newhead;

    if(!fr->bitrate_index) {
      error1("encountered free format header %08lx in decode_header - not supported yet",newhead);
      return (0);
    }

    switch(fr->lay) {
      case 1:
	fr->do_layer = do_layer1;
#ifdef VARMODESUPPORT
        if (varmode) {
          error("Sorry, layer-1 not supported in varmode."); 
          return (0);
        }
#endif
        fr->framesize  = (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
        fr->framesize /= freqs[fr->sampling_frequency];
        fr->framesize  = ((fr->framesize+fr->padding)<<2)-4;
        break;
      case 2:
	fr->do_layer = do_layer2;
#ifdef VARMODESUPPORT
        if (varmode) {
          error("Sorry, layer-2 not supported in varmode."); 
          return (0);
        }
#endif
        fr->framesize = (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
        fr->framesize /= freqs[fr->sampling_frequency];
        fr->framesize += fr->padding - 4;
        break;
      case 3:
        fr->do_layer = do_layer3;
        if(fr->lsf)
          ssize = (fr->stereo == 1) ? 9 : 17;
        else
          ssize = (fr->stereo == 1) ? 17 : 32;
        if(fr->error_protection)
          ssize += 2;
        fr->framesize  = (long) tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
        fr->framesize /= freqs[fr->sampling_frequency]<<(fr->lsf);
        fr->framesize = fr->framesize + fr->padding - 4;
        break; 
      default:
        error("unknown layer type (!!)"); 
        return (0);
    }
    if (fr->framesize > MAXFRAMESIZE) {
      error1("Frame size too big: %d", fr->framesize+4-fr->padding);
      return (0);
    }
    return 1;
}

/* concurring to print_rheader... here for control_generic */
const char* remote_header_help = "S <mpeg-version> <layer> <sampling freq> <mode(stereo/mono/...)> <mode_ext> <framesize> <stereo> <copyright> <error_protected> <emphasis> <bitrate> <extension> <vbr(0/1=yes/no)>";
void make_remote_header(struct frame* fr, char *target)
{
	/* redundancy */
	static char *modes[4] = {"Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel"};
	snprintf(target, 1000, "S %s %d %ld %s %d %d %d %d %d %d %d %d %d",
		fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
		fr->lay,
		freqs[fr->sampling_frequency],
		modes[fr->mode],
		fr->mode_ext,
		fr->framesize+4,
		fr->stereo,
		fr->copyright ? 1 : 0,
		fr->error_protection ? 1 : 0,
		fr->emphasis,
		tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index],
		fr->extension,
		vbr);
}


#ifdef MPG123_REMOTE
void print_rheader(struct frame *fr)
{
	static char *modes[4] = { "Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel" };
	static char *layers[4] = { "Unknown" , "I", "II", "III" };
	static char *mpeg_type[2] = { "1.0" , "2.0" };

	/* version, layer, freq, mode, channels, bitrate, BPF, VBR*/
	fprintf(stderr,"@I %s %s %ld %s %d %d %d %i\n",
			mpeg_type[fr->lsf],layers[fr->lay],freqs[fr->sampling_frequency],
			modes[fr->mode],fr->stereo,
			vbr == ABR ? abr_rate : tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index],
			fr->framesize+4,
			vbr);
}
#endif

void print_header(struct frame *fr)
{
	static char *modes[4] = { "Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel" };
	static char *layers[4] = { "Unknown" , "I", "II", "III" };

	fprintf(stderr,"MPEG %s, Layer: %s, Freq: %ld, mode: %s, modext: %d, BPF : %d\n", 
		fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
		layers[fr->lay],freqs[fr->sampling_frequency],
		modes[fr->mode],fr->mode_ext,fr->framesize+4);
	fprintf(stderr,"Channels: %d, copyright: %s, original: %s, CRC: %s, emphasis: %d.\n",
		fr->stereo,fr->copyright?"Yes":"No",
		fr->original?"Yes":"No",fr->error_protection?"Yes":"No",
		fr->emphasis);
	fprintf(stderr,"Bitrate: ");
	switch(vbr)
	{
		case CBR: fprintf(stderr, "%d kbits/s", tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index]); break;
		case VBR: fprintf(stderr, "VBR"); break;
		case ABR: fprintf(stderr, "%d kbit/s ABR", abr_rate); break;
		default: fprintf(stderr, "???");
	}
	fprintf(stderr, " Extension value: %d\n",	fr->extension);
}

void print_header_compact(struct frame *fr)
{
	static char *modes[4] = { "stereo", "joint-stereo", "dual-channel", "mono" };
	static char *layers[4] = { "Unknown" , "I", "II", "III" };
	
	fprintf(stderr,"MPEG %s layer %s, ",
		fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
		layers[fr->lay]);
	switch(vbr)
	{
		case CBR: fprintf(stderr, "%d kbits/s", tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index]); break;
		case VBR: fprintf(stderr, "VBR"); break;
		case ABR: fprintf(stderr, "%d kbit/s ABR", abr_rate); break;
		default: fprintf(stderr, "???");
	}
	fprintf(stderr,", %ld Hz %s\n",
		freqs[fr->sampling_frequency], modes[fr->mode]);
}

void print_id3_tag(unsigned char *buf)
{
	struct id3tag {
		char tag[3];
		char title[30];
		char artist[30];
		char album[30];
		char year[4];
		char comment[30];
		unsigned char genre;
	};
	struct id3tag *tag = (struct id3tag *) buf;
	char title[31]={0,};
	char artist[31]={0,};
	char album[31]={0,};
	char year[5]={0,};
	char comment[31]={0,};
	char genre[31]={0,};

	if(param.quiet)
		return;

	strncpy(title,tag->title,30);
	strncpy(artist,tag->artist,30);
	strncpy(album,tag->album,30);
	strncpy(year,tag->year,4);
	strncpy(comment,tag->comment,30);

	if (tag->genre <= genre_count) {
		strncpy(genre, genre_table[tag->genre], 30);
	} else {
		strncpy(genre,"Unknown",30);
	}
	
	fprintf(stderr,"Title  : %-30s  Artist: %s\n",title,artist);
	fprintf(stderr,"Album  : %-30s  Year  : %4s\n",album,year);
	fprintf(stderr,"Comment: %-30s  Genre : %s\n",comment,genre);
}

#if 0
/* removed the strndup for better portability */
/*
 *   Allocate space for a new string containing the first
 *   "num" characters of "src".  The resulting string is
 *   always zero-terminated.  Returns NULL if malloc fails.
 */
char *strndup (const char *src, int num)
{
	char *dst;

	if (!(dst = (char *) malloc(num+1)))
		return (NULL);
	dst[num] = '\0';
	return (strncpy(dst, src, num));
}
#endif

/*
 *   Split "path" into directory and filename components.
 *
 *   Return value is 0 if no directory was specified (i.e.
 *   "path" does not contain a '/'), OR if the directory
 *   is the same as on the previous call to this function.
 *
 *   Return value is 1 if a directory was specified AND it
 *   is different from the previous one (if any).
 */

int split_dir_file (const char *path, char **dname, char **fname)
{
	static char *lastdir = NULL;
	char *slashpos;

	if ((slashpos = strrchr(path, '/'))) {
		*fname = slashpos + 1;
		*dname = strdup(path); /* , 1 + slashpos - path); */
		if(!(*dname)) {
			perror("memory");
			exit(1);
		}
		(*dname)[1 + slashpos - path] = 0;
		if (lastdir && !strcmp(lastdir, *dname)) {
			/***   same as previous directory   ***/
			free (*dname);
			*dname = lastdir;
			return 0;
		}
		else {
			/***   different directory   ***/
			if (lastdir)
				free (lastdir);
			lastdir = *dname;
			return 1;
		}
	}
	else {
		/***   no directory specified   ***/
		if (lastdir) {
			free (lastdir);
			lastdir = NULL;
		};
		*dname = NULL;
		*fname = (char *)path;
		return 0;
	}
}

void set_pointer(long backstep)
{
  bsi.wordpointer = bsbuf + ssize - backstep;
  if (backstep)
    memcpy(bsi.wordpointer,bsbufold+fsizeold-backstep,backstep);
  bsi.bitindex = 0; 
}

/********************************/

double compute_bpf(struct frame *fr)
{
	double bpf;

        switch(fr->lay) {
                case 1:
                        bpf = tabsel_123[fr->lsf][0][fr->bitrate_index];
                        bpf *= 12000.0 * 4.0;
                        bpf /= freqs[fr->sampling_frequency] <<(fr->lsf);
                        break;
                case 2:
                case 3:
                        bpf = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];
                        bpf *= 144000;
                        bpf /= freqs[fr->sampling_frequency] << (fr->lsf);
                        break;
                default:
                        bpf = 1.0;
        }

	return bpf;
}

double compute_tpf(struct frame *fr)
{
	static int bs[4] = { 0,384,1152,1152 };
	double tpf;

	tpf = (double) bs[fr->lay];
	tpf /= freqs[fr->sampling_frequency] << (fr->lsf);
	return tpf;
}

/*
 * Returns number of frames queued up in output buffer, i.e. 
 * offset between currently played and currently decoded frame.
 */

long compute_buffer_offset(struct frame *fr)
{
	long bufsize;
	
	/*
	 * buffermem->buf[0] holds output sampling rate,
	 * buffermem->buf[1] holds number of channels,
	 * buffermem->buf[2] holds audio format of output.
	 */
	
	if(!param.usebuffer || !(bufsize=xfermem_get_usedspace(buffermem))
		|| !buffermem->buf[0] || !buffermem->buf[1])
		return 0;

	bufsize = (long)((double) bufsize / buffermem->buf[0] / 
			buffermem->buf[1] / compute_tpf(fr));
	
	if((buffermem->buf[2] & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_16)
		return bufsize/2;
	else
		return bufsize;
}

void print_stat(struct frame *fr,unsigned long no,long buffsize,struct audio_info_struct *ai)
{
	double bpf,tpf,tim1,tim2;
	double dt = 0.0;
	unsigned long sno,rno;
	/* that's not funny... overflows waving */
	char outbuf[256];

	if(!rd || !fr)
	{
		debug("reader troubles!");
		return;
	}
	outbuf[0] = 0;

#ifndef GENERIC
	{
		struct timeval t;
		fd_set serr;
		int n,errfd = fileno(stderr);

		t.tv_sec=t.tv_usec=0;

		FD_ZERO(&serr);
		FD_SET(errfd,&serr);
		n = select(errfd+1,NULL,&serr,NULL,&t);
		if(n <= 0)
			return;
	}
#endif

	bpf = compute_bpf(fr);
	tpf = compute_tpf(fr);

	if(buffsize > 0 && ai && ai->rate > 0 && ai->channels > 0) {
		dt = (double) buffsize / ai->rate / ai->channels;
		if( (ai->format & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_16)
			dt *= 0.5;
	}

        rno = 0;
        sno = no;

	if((track_frames != 0) && (track_frames >= sno)) rno = track_frames - sno;
	else
	if(rd->filelen >= 0)
	{
		long t = rd->tell(rd);
		rno = (unsigned long)((double)(rd->filelen-t)/bpf);
		/* I totally don't understand why we should re-estimate the given correct(?) value */
		/* sno = (unsigned long)((double)t/bpf); */
	}

	/* beginning with 0 or 1?*/
	sprintf(outbuf+strlen(outbuf),"\rFrame# %5lu [%5lu], ",sno,rno);
	tim1 = sno*tpf-dt;
	tim2 = rno*tpf+dt;
#if 0
	tim1 = tim1 < 0 ? 0.0 : tim1;
#endif
	tim2 = tim2 < 0 ? 0.0 : tim2;

	sprintf(outbuf+strlen(outbuf),"Time: %02lu:%02u.%02u [%02u:%02u.%02u], ",
			(unsigned long) tim1/60,
			(unsigned int)tim1%60,
			(unsigned int)(tim1*100)%100,
			(unsigned int)tim2/60,
			(unsigned int)tim2%60,
			(unsigned int)(tim2*100)%100);

	if(param.usebuffer)
		sprintf(outbuf+strlen(outbuf),"[%8ld] ",(long)buffsize);
	write(fileno(stderr),outbuf,strlen(outbuf));
#if 0
	fflush(out); /* hmm not really nec. */
#endif
}

int get_songlen(struct frame *fr,int no)
{
	double tpf;
	
	if(!fr)
		return 0;
	
	if(no < 0) {
		if(!rd || rd->filelen < 0)
			return 0;
		no = (double) rd->filelen / compute_bpf(fr);
	}

	tpf = compute_tpf(fr);
	return no*tpf;
}


