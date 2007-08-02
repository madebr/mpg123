/*
	common: anything can happen here... frame reading, output, messages

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123.h"

/* #include <ctype.h> */
#include <sys/stat.h>
#include <sys/time.h>
#include <math.h>

#include <fcntl.h>

#include "id3.h"
#include "icy.h"
#include "common.h"

#ifdef WIN32
#include <winsock.h>
#endif

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
#define TRACK_MAX_FRAMES ULONG_MAX/4/1152

/* this could become a struct... */
scale_t lastscale = -1; /* last used scale */
int rva_level[2] = {-1,-1}; /* significance level of stored rva */
float rva_gain[2] = {0,0}; /* mix, album */
float rva_peak[2] = {0,0};
const char* rva_name[3] = { "off", "mix", "album" };

static double mean_framesize;
static unsigned long mean_frames;
static int do_recover = 0;
struct 
{
	off_t data[INDEX_SIZE];
	size_t fill;
	unsigned long step;
} frame_index;

unsigned char *pcm_sample;
int pcm_point = 0;
int audiobufsize = AUDIOBUFSIZE;

#define RESYNC_LIMIT 1024

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
#ifdef FLOATOUT
		* 2
#endif
		* ai->channels
		* (int) (((sammy - samf) < 0.5) ? samf : ( sammy-samf > 0.5 ? samf+1 : ((unsigned long) samf % 2 == 0 ? samf : samf + 1)));
}
#endif

void audio_flush(int outmode, struct audio_info_struct *ai)
{
	/* the gapless code is not in effect for buffered mode... as then condition for audio_flush is never met */
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
				error("The buffer doesn't work like that... I shouldn't ever be getting here.");
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

void read_frame_init (struct frame* fr)
{
	fr->num = -1;
	oldhead = 0;
	firsthead = 0;
	vbr = CBR;
	abr_rate = 0;
	track_frames = 0;
	mean_frames = 0;
	mean_framesize = 0;
	rva_level[0] = -1;
	rva_level[1] = -1;
	#ifdef GAPLESS
	/* one can at least skip the delay at beginning - though not add it at end since end is unknown */
	if(param.gapless) layer3_gapless_init(DECODER_DELAY+GAP_SHIFT, 0);
	#endif
	frame_index.fill = 0;
	frame_index.step = 1;
	reset_id3();
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

void do_volume(double factor)
{
	if(factor < 0) factor = 0;
	/* change the output scaling and apply with rva */
	outscale = (double) MAXOUTBURST * factor;
	do_rva();
}

/* adjust the volume, taking both outscale and rva values into account */
void do_rva()
{
	double rvafact = 1;
	float peak = 0;
	scale_t newscale;

	if(param.rva)
	{
		int rt = 0;
		/* Should one assume a zero RVA as no RVA? */
		if(param.rva == 2 && rva_level[1] != -1) rt = 1;
		if(rva_level[rt] != -1)
		{
			rvafact = pow(10,rva_gain[rt]/20);
			peak = rva_peak[rt];
			if(param.verbose > 1) fprintf(stderr, "Note: doing RVA with gain %f\n", rva_gain[rt]);
		}
		else
		{
			warning("no RVA value found");
		}
	}

	newscale = outscale*rvafact;

	/* if peak is unknown (== 0) this check won't hurt */
	if((peak*newscale) > MAXOUTBURST)
	{
		newscale = (scale_t) ((double) MAXOUTBURST/peak);
		warning2("limiting scale value to %li to prevent clipping with indicated peak factor of %f", newscale, peak);
	}
	/* first rva setting is forced with lastscale < 0 */
	if(newscale != lastscale)
	{
		debug3("changing scale value from %li to %li (peak estimated to %li)", lastscale != -1 ? lastscale : outscale, newscale, (long) (newscale*peak));
		opt_make_decode_tables(newscale); /* the actual work */
		lastscale = newscale;
	}
}


int read_frame_recover(struct frame* fr)
{
	int ret;
	do_recover = 1;
	ret = read_frame(fr);
	do_recover = 0;
	return ret;
}

/*****************************************************************

 * read next frame
 */
int read_frame(struct frame *fr)
{
	/* TODO: rework this thing */
  unsigned long newhead;
  static unsigned char ssave[34];
	off_t framepos;
  int give_note = param.verbose > 1 ? 1 : (do_recover ? 0 : 1 );
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
	if(!firsthead && !head_check(newhead) && !free_format_header(newhead)) {
		int i;

		/* check for id3v2; first three bytes (of 4) are "ID3" */
		if((newhead & (unsigned long) 0xffffff00) == (unsigned long) 0x49443300)
		{
			int id3length = 0;
			id3length = parse_new_id3(newhead, rd);
			goto read_again;
		}
		else if(param.verbose > 1) fprintf(stderr,"Note: Junk at the beginning (0x%08lx)\n",newhead);

		/* I even saw RIFF headers at the beginning of MPEG streams ;( */
		if(newhead == ('R'<<24)+('I'<<16)+('F'<<8)+'F') {
			if(param.verbose > 1) fprintf(stderr, "Note: Looks like a RIFF header.\n");
			if(!rd->head_read(rd,&newhead))
				return 0;
			while(newhead != ('d'<<24)+('a'<<16)+('t'<<8)+'a') {
				if(!rd->head_shift(rd,&newhead))
					return 0;
			}
			if(!rd->head_read(rd,&newhead))
				return 0;
			if(param.verbose > 1) fprintf(stderr,"Note: Skipped RIFF header!\n");
			goto read_again;
		}
		/* unhandled junk... just continue search for a header */
		/* step in byte steps through next 64K */
		debug("searching for header...");
		for(i=0;i<65536;i++) {
			if(!rd->head_shift(rd,&newhead))
				return 0;
			/* if(head_check(newhead)) */
			if(head_check(newhead) && decode_header(fr, newhead))
				break;
		}
		if(i == 65536) {
			if(!param.quiet) error("Giving up searching valid MPEG header after 64K of junk.");
			return 0;
		}
		else debug("hopefully found one...");
		/* 
		 * should we additionaly check, whether a new frame starts at
		 * the next expected position? (some kind of read ahead)
		 * We could implement this easily, at least for files.
		 */
	}
#endif

	/* first attempt of read ahead check to find the real first header; cannot believe what junk is out there! */
	/* for now, a spurious first free format header screws up here; need free format support for detecting false free format headers... */
	if(!firsthead && rd->flags & READER_SEEKABLE && head_check(newhead) && decode_header(fr, newhead))
	{
		unsigned long nexthead = 0;
		int hd = 0;
		off_t start = rd->tell(rd);
		debug1("doing ahead check with BPF %d", fr->framesize+4);
		/* step framesize bytes forward and read next possible header*/
		if(rd->back_bytes(rd, -fr->framesize))
		{
			error("cannot seek!");
			return 0;
		}
		hd = rd->head_read(rd,&nexthead);
		if(rd->back_bytes(rd, rd->tell(rd)-start))
		{
			error("cannot seek!");
			return 0;
		}
		if(!hd)
		{
			warning("cannot read next header, a one-frame stream? Duh...");
		}
		else
		{
			debug2("does next header 0x%08lx match first 0x%08lx?", nexthead, newhead);
			/* not allowing free format yet */
			if(!head_check(nexthead) || (nexthead & HDRCMPMASK) != (newhead & HDRCMPMASK))
			{
				debug("No, the header was not valid, start from beginning...");
				oldhead = 0; /* start over */
				/* try next byte for valid header */
				if(rd->back_bytes(rd, 3))
				{
					error("cannot seek!");
					return 0;
				}
				goto read_again;
			}
		}
	}

    /* why has this head check been avoided here before? */
    if(!head_check(newhead))
    {
      if(!firsthead && free_format_header(newhead))
      {
        error1("Header 0x%08lx seems to indicate a free format stream; I do not handle that yet", newhead);
        goto read_again;
        return 0;
      }
    /* and those ugly ID3 tags */
      if((newhead & 0xffffff00) == ('T'<<24)+('A'<<16)+('G'<<8)) {
           rd->skip_bytes(rd,124);
	   if (param.verbose > 1) fprintf(stderr,"Note: Skipped ID3 Tag!\n");
           goto read_again;
      }
      /* duplicated code from above! */
      /* check for id3v2; first three bytes (of 4) are "ID3" */
      if((newhead & (unsigned long) 0xffffff00) == (unsigned long) 0x49443300)
      {
        int id3length = 0;
        id3length = parse_new_id3(newhead, rd);
        goto read_again;
      }
      else if (give_note)
      {
        fprintf(stderr,"Note: Illegal Audio-MPEG-Header 0x%08lx at offset 0x%lx.\n", newhead,rd->tell(rd)-4);
      }

      if(give_note && (newhead & 0xffffff00) == ('b'<<24)+('m'<<16)+('p'<<8)) fprintf(stderr,"Note: Could be a BMP album art.\n");
      if (param.tryresync || do_recover) {
        int try = 0;
        /* TODO: make this more robust, I'd like to cat two mp3 fragments together (in a dirty way) and still have mpg123 beign able to decode all it somehow. */
        if(give_note) fprintf(stderr, "Note: Trying to resync...\n");
            /* Read more bytes until we find something that looks
               reasonably like a valid header.  This is not a
               perfect strategy, but it should get us back on the
               track within a short time (and hopefully without
               too much distortion in the audio output).  */
        do {
          if(!rd->head_shift(rd,&newhead))
		return 0;
          debug2("resync try %i, got newhead 0x%08lx", try, newhead);
          if (!oldhead)
          {
            debug("going to init_resync...");
            goto init_resync;       /* "considered harmful", eh? */
          }
         /* we should perhaps collect a list of valid headers that occured in file... there can be more */
         /* Michael's new resync routine seems to work better with the one frame readahead (and some input buffering?) */
         } while
         (
           ++try < RESYNC_LIMIT
           && (newhead & HDRCMPMASK) != (oldhead & HDRCMPMASK)
           && (newhead & HDRCMPMASK) != (firsthead & HDRCMPMASK)
         );
         /* too many false positives 
         }while (!(head_check(newhead) && decode_header(fr, newhead))); */
         if(try == RESYNC_LIMIT)
         {
           error("giving up resync - your stream is not nice... perhaps an improved routine could catch up");
           return 0;
         }

        if (give_note)
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
	/* if filepos is invalid, so is framepos */
	framepos = rd->filepos - 4;
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
				debug("do we have lame tag?");
				/* only search for tag when all zero before it (apart from checksum) */
				for(i=2; i < lame_offset; ++i) if(bsbuf[i] != 0) break;
				if(i == lame_offset)
				{
					debug("possibly...");
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
						if(param.verbose > 1) fprintf(stderr, "Note: Xing/Lame/Info header detected\n");
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
		debug1("firsthead: %08lx", firsthead);
		/* now adjust volume */
		do_rva();
		/* and print id3/stream info */
		if(!param.quiet)
		{
			print_id3_tag(rd->flags & READER_ID3TAG ? rd->id3buf : NULL);
			if(icy.name.fill) fprintf(stderr, "ICY-NAME: %s\n", icy.name.p);
			if(icy.url.fill) fprintf(stderr, "ICY-URL: %s\n", icy.url.p);
		}
	}
  bsi.bitindex = 0;
  bsi.wordpointer = (unsigned char *) bsbuf;

  if (param.halfspeed && fr->lay == 3)
    memcpy (ssave, bsbuf, ssize);

	debug2("N %08lx %i", newhead, fr->framesize);
	if(++mean_frames != 0)
	{
		mean_framesize = ((mean_frames-1)*mean_framesize+compute_bpf(fr)) / mean_frames ;
	}
	++fr->num; /* 0 for the first! */
	/* index the position */
	if(INDEX_SIZE > 0) /* any sane compiler should make a no-brainer out of this */
	{
		if(fr->num == frame_index.fill*frame_index.step)
		{
			if(frame_index.fill == INDEX_SIZE)
			{
				size_t c;
				/* increase step, reduce fill */
				frame_index.step *= 2;
				frame_index.fill /= 2; /* divisable by 2! */
				for(c = 0; c < frame_index.fill; ++c)
				{
					frame_index.data[c] = frame_index.data[2*c];
				}
			}
			if(fr->num == frame_index.fill*frame_index.step)
			{
				frame_index.data[frame_index.fill] = framepos;
				++frame_index.fill;
			}
		}
	}
  return 1;
}

void print_frame_index(FILE* out)
{
	size_t c;
	for(c=0; c < frame_index.fill;++c) fprintf(out, "[%lu] %lu: %li (+%li)\n", (unsigned long) c, (unsigned long) c*frame_index.step, (long)frame_index.data[c], (long) (c ? frame_index.data[c]-frame_index.data[c-1] : 0));
}

/*
	find the best frame in index just before the wanted one, seek to there
	then step to just before wanted one with read_frame
	do not care tabout the stuff that was in buffer but not played back
	everything that left the decoder is counted as played
	
	Decide if you want low latency reaction and accurate timing info or stable long-time playback with buffer!
*/

off_t frame_index_find(unsigned long want_frame, unsigned long* get_frame)
{
	/* default is file start if no index position */
	off_t gopos = 0;
	*get_frame = 0;
	if(frame_index.fill)
	{
		/* find in index */
		size_t fi;
		/* at index fi there is frame step*fi... */
		fi = want_frame/frame_index.step;
		if(fi >= frame_index.fill) fi = frame_index.fill - 1;
		*get_frame = fi*frame_index.step;
		gopos = frame_index.data[fi];
	}
	return gopos;
}

/* dead code?  -  see readers.c */
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
        return 0; /* exit() here really is too much, isn't it? */
      }
      if(fr->mpeg25) {
        fr->sampling_frequency = 6 + ((newhead>>10)&0x3);
      }
      else
        fr->sampling_frequency = ((newhead>>10)&0x3) + (fr->lsf*3);
    }

    #ifdef DEBUG
    if((((newhead>>16)&0x1)^0x1) != fr->error_protection) debug("changed crc bit!");
    #endif
    fr->error_protection = ((newhead>>16)&0x1)^0x1; /* seen a file where this varies (old lame tag without crc, track with crc) */
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
debug2("bitrate index: %i (%i)", fr->bitrate_index, tabsel_123[fr->lsf][1][fr->bitrate_index] );
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
void print_remote_header(struct frame* fr)
{
	static char *modes[4] = {"Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel"};
	generic_sendmsg("S %s %d %ld %s %d %d %d %d %d %d %d %d %d",
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
			perror("failed to allocate memory for dir name");
			return 0;
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

/* Way too many parameters - heck, this fr and ai is always the same! */
int position_info(struct frame* fr, unsigned long no, long buffsize, struct audio_info_struct* ai,
                   unsigned long* frames_left, double* current_seconds, double* seconds_left)
{
	double tpf;
	double dt = 0.0;

	if(!rd || !fr)
	{
		debug("reader troubles!");
		return -1;
	}

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
			return -2;
	}
#endif

	tpf = compute_tpf(fr);
	if(buffsize > 0 && ai && ai->rate > 0 && ai->channels > 0) {
		dt = (double) buffsize / ai->rate / ai->channels;
		if( (ai->format & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_16)
			dt *= 0.5;
	}

	(*frames_left) = 0;

	if((track_frames != 0) && (track_frames >= fr->num)) (*frames_left) = no < track_frames ? track_frames - no : 0;
	else
	if(rd->filelen >= 0)
	{
		double bpf;
		long t = rd->tell(rd);
		bpf = mean_framesize ? mean_framesize : compute_bpf(fr);
		(*frames_left) = (unsigned long)((double)(rd->filelen-t)/bpf);
		/* no can be different for prophetic purposes, file pointer is always associated with fr->num! */
		if(fr->num != no)
		{
			if(fr->num > no) *frames_left += fr->num - no;
			else
			{
				if(*frames_left >= (no - fr->num)) *frames_left -= no - fr->num;
				else *frames_left = 0; /* uh, oh! */
			}
		}
		/* I totally don't understand why we should re-estimate the given correct(?) value */
		/* fr->num = (unsigned long)((double)t/bpf); */
	}

	/* beginning with 0 or 1?*/
	(*current_seconds) = (double) no*tpf-dt;
	(*seconds_left) = (double)(*frames_left)*tpf+dt;
#if 0
	(*current_seconds) = (*current_seconds) < 0 ? 0.0 : (*current_seconds);
#endif
	if((*seconds_left) < 0)
	{
		warning("seconds_left < 0!");
		(*seconds_left) = 0.0;
	}
	return 0;
}

long time_to_frame(struct frame *fr, double seconds)
{
	return (long) (seconds/compute_tpf(fr));
}

unsigned int roundui(double val)
{
	double base = floor(val);
	return (unsigned int) ((val-base) < 0.5 ? base : base + 1 );
}

void print_stat(struct frame *fr,unsigned long no,long buffsize,struct audio_info_struct *ai)
{
	double tim1,tim2;
	unsigned long rno;
	if(!position_info(fr, no, buffsize, ai, &rno, &tim1, &tim2))
	{
		/* All these sprintf... only to avoid two writes to stderr in case of using buffer?
		   I guess we can drop that. */
		fprintf(stderr, "\rFrame# %5lu [%5lu], Time: %02lu:%02u.%02u [%02u:%02u.%02u], RVA:%6s, Vol: %3u(%3u)",
		        no,rno,
		        (unsigned long) tim1/60, (unsigned int)tim1%60, (unsigned int)(tim1*100)%100,
		        (unsigned int)tim2/60, (unsigned int)tim2%60, (unsigned int)(tim2*100)%100,
		        rva_name[param.rva], roundui((double)outscale/MAXOUTBURST*100), roundui((double)lastscale/MAXOUTBURST*100) );
		if(param.usebuffer) fprintf(stderr,", [%8ld] ",(long)buffsize);
	}
	if(icy.changed && icy.data)
	{
		fprintf(stderr, "\nICY-META: %s\n", icy.data);
		icy.changed = 0;
	}
}

void clear_stat()
{
	fprintf(stderr, "\r                                                                                       \r");
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


