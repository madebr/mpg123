/*
	common: misc stuff... audio flush, status display...

	copyright ?-2015 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123app.h"
#include "out123.h"
#include <sys/stat.h>
#include "common.h"

#ifdef HAVE_TERMIOS
#include <termios.h>
#include <sys/ioctl.h>
#endif

#include "debug.h"

int stopped = 0;
int paused = 0;

/* Also serves as a way to detect if we have an interactive terminal. */
int term_width(int fd)
{
#ifdef HAVE_TERMIOS
	struct winsize geometry;
	geometry.ws_col = 0;
	if(ioctl(fd, TIOCGWINSZ, &geometry) >= 0)
		return (int)geometry.ws_col;
#endif
	return -1;
}

const char* rva_name[3] = { "v", "m", "a" }; /* vanilla, mix, album */
static const char *modes[5] = {"Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel", "Invalid" };
static const char *smodes[5] = { "stereo", "joint-stereo", "dual-channel", "mono", "invalid" };
static const char *layers[4] = { "Unknown" , "I", "II", "III" };
static const char *versions[4] = {"1.0", "2.0", "2.5", "x.x" };
static const int samples_per_frame[4][4] =
{
	{ -1,384,1152,1152 },	/* MPEG 1 */
	{ -1,384,1152,576 },	/* MPEG 2 */
	{ -1,384,1152,576 },	/* MPEG 2.5 */
	{ -1,-1,-1,-1 },		/* Unknown */
};

/* concurring to print_rheader... here for control_generic */
const char* remote_header_help = "S <mpeg-version> <layer> <sampling freq> <mode(stereo/mono/...)> <mode_ext> <framesize> <stereo> <copyright> <error_protected> <emphasis> <bitrate> <extension> <vbr(0/1=yes/no)>";
void print_remote_header(mpg123_handle *mh)
{
	struct mpg123_frameinfo i;
	mpg123_info(mh, &i);
	if(i.mode >= 4 || i.mode < 0) i.mode = 4;
	if(i.version >= 3 || i.version < 0) i.version = 3;
	generic_sendmsg("S %s %d %ld %s %d %d %d %d %d %d %d %d %d",
		versions[i.version],
		i.layer,
		i.rate,
		modes[i.mode],
		i.mode_ext,
		i.framesize,
		i.mode == MPG123_M_MONO ? 1 : 2,
		i.flags & MPG123_COPYRIGHT ? 1 : 0,
		i.flags & MPG123_CRC ? 1 : 0,
		i.emphasis,
		i.bitrate,
		i.flags & MPG123_PRIVATE ? 1 : 0,
		i.vbr);
}

void print_header(mpg123_handle *mh)
{
	struct mpg123_frameinfo i;
	mpg123_info(mh, &i);
	if(i.mode > 4 || i.mode < 0) i.mode = 4;
	if(i.version > 3 || i.version < 0) i.version = 3;
	if(i.layer > 3 || i.layer < 0) i.layer = 0;
	fprintf(stderr,"MPEG %s, Layer: %s, Freq: %ld, mode: %s, modext: %d, BPF : %d\n", 
		versions[i.version],
		layers[i.layer], i.rate,
		modes[i.mode],i.mode_ext,i.framesize);
	fprintf(stderr,"Channels: %d, copyright: %s, original: %s, CRC: %s, emphasis: %d.\n",
		i.mode == MPG123_M_MONO ? 1 : 2,i.flags & MPG123_COPYRIGHT ? "Yes" : "No",
		i.flags & MPG123_ORIGINAL ? "Yes" : "No", i.flags & MPG123_CRC ? "Yes" : "No",
		i.emphasis);
	fprintf(stderr,"Bitrate: ");
	switch(i.vbr)
	{
		case MPG123_CBR:
			if(i.bitrate) fprintf(stderr, "%d kbit/s", i.bitrate);
			else fprintf(stderr, "%d kbit/s (free format)", (int)((double)(i.framesize+4)*8*i.rate*0.001/samples_per_frame[i.version][i.layer]+0.5));
			break;
		case MPG123_VBR: fprintf(stderr, "VBR"); break;
		case MPG123_ABR: fprintf(stderr, "%d kbit/s ABR", i.abr_rate); break;
		default: fprintf(stderr, "???");
	}
	fprintf(stderr, " Extension value: %d\n",	i.flags & MPG123_PRIVATE ? 1 : 0);
}

void print_header_compact(mpg123_handle *mh)
{
	struct mpg123_frameinfo i;
	mpg123_info(mh, &i);
	if(i.mode > 4 || i.mode < 0) i.mode = 4;
	if(i.version > 3 || i.version < 0) i.version = 3;
	if(i.layer > 3 || i.layer < 0) i.layer = 0;
	
	fprintf(stderr,"MPEG %s layer %s, ", versions[i.version], layers[i.layer]);
	switch(i.vbr)
	{
		case MPG123_CBR:
			if(i.bitrate) fprintf(stderr, "%d kbit/s", i.bitrate);
			else fprintf(stderr, "%d kbit/s (free format)", (int)((double)i.framesize*8*i.rate*0.001/samples_per_frame[i.version][i.layer]+0.5));
			break;
		case MPG123_VBR: fprintf(stderr, "VBR"); break;
		case MPG123_ABR: fprintf(stderr, "%d kbit/s ABR", i.abr_rate); break;
		default: fprintf(stderr, "???");
	}
	fprintf(stderr,", %ld Hz %s\n", i.rate, smodes[i.mode]);
}

unsigned int roundui(double val)
{
	double base = floor(val);
	return (unsigned int) ((val-base) < 0.5 ? base : base + 1 );
}

/* Split into mm:ss.xx or hh:mm:ss, depending on value. */
static void settle_time(double tim, unsigned long *times, char *sep)
{
	if(tim >= 3600.)
	{
		*sep = ':';
		times[0] = (unsigned long) tim/3600;
		tim -= times[0]*3600;
		times[1] = (unsigned long) tim/60;
		tim -= times[1]*60;
		times[2] = (unsigned long) tim;
	}
	else
	{
		*sep = '.';
		times[0] = (unsigned long) tim/60;
		times[1] = (unsigned long) tim%60;
		times[2] = (unsigned long) (tim*100)%100;
	}
}

/* Print output buffer fill. */
void print_buf(const char* prefix, audio_output_t *ao)
{
	long rate;
	int framesize;
	double tim;
	unsigned long times[3];
	char timesep;
	size_t buffsize;

	buffsize = out123_buffered(ao);
	if(out123_getformat(ao, &rate, NULL, NULL, &framesize))
		return;
	tim = (double)(buffsize/framesize)/rate;
	settle_time(tim, times, &timesep);
	fprintf( stderr, "\r%s[%02lu:%02lu%c%02lu]"
	,	prefix, times[0], times[1], timesep, times[2] );
}

/* Note about position info with buffering:
   Negative positions mean that the previous track is still playing from the
   buffer. It's a countdown. The frame counter always relates to the last
   decoded frame, what entered the buffer right now. */
void print_stat(mpg123_handle *fr, long offset, audio_output_t *ao)
{
	double tim[3];
	off_t rno, no;
	double basevol, realvol;
	char *icy;
	size_t buffsize;
	long rate;
	int framesize;

	buffsize = out123_buffered(ao);
	if(out123_getformat(ao, &rate, NULL, NULL, &framesize))
		return;
#ifndef WIN32
#ifndef GENERIC
/* Only generate new stat line when stderr is ready... don't overfill... */
	{
		struct timeval t;
		fd_set serr;
		int n,errfd = fileno(stderr);

		t.tv_sec=t.tv_usec=0;

		FD_ZERO(&serr);
		FD_SET(errfd,&serr);
		n = select(errfd+1,NULL,&serr,NULL,&t);
		if(n <= 0) return;
	}
#endif
#endif
	if(  MPG123_OK == mpg123_position(fr, offset, buffsize, &no, &rno, tim, tim+1)
	  && MPG123_OK == mpg123_getvolume(fr, &basevol, &realvol, NULL) )
	{
		char line[255]; /* Stat lines cannot grow too much. */
		int len;
		int ti;
		/* Deal with overly long times. */
		unsigned long times[3][3];
		char timesep[3];
		char sign[3] = {' ', ' ', ' '};
		tim[2] = (double)(buffsize/framesize)/rate;
		for(ti=0; ti<3; ++ti)
		{
			if(tim[ti] < 0.){ sign[ti] = '-'; tim[ti] = -tim[ti]; }
			settle_time(tim[ti], times[ti], &timesep[ti]);
		}
		memset(line, 0, sizeof(line));
		len = snprintf( line, sizeof(line)-1
		,	"%c %5"OFF_P"[%5"OFF_P"] %c%02lu:%02lu%c%02lu[%c%02lu:%02lu%c%02lu] V(%s)=%3u(%3u)"
		,	stopped ? '_' : (paused ? '=' : '>')
		,	(off_p)no, (off_p)rno
		,	sign[0]
		,	times[0][0], times[0][1], timesep[0], times[0][2]
		,	sign[1]
		,	times[1][0], times[1][1], timesep[1], times[1][2]
		,	rva_name[param.rva], roundui(basevol*100), roundui(realvol*100)
		);
		if(len >= 0 && param.usebuffer && len < sizeof(line) )
		{
			int len_add = snprintf( line+len, sizeof(line)-1-len
			,	" [%02lu:%02lu%c%02lu]"
			,	times[2][0], times[2][1], timesep[2], times[2][2] );
			if(len_add > 0)
				len += len_add;
		}
		if(len >= 0)
		{
			int maxlen = term_width(STDOUT_FILENO);
			if(maxlen > 0 && len > maxlen)
			{
				/* Emergency cut to avoid terminal scrolling. */
				int i;
				/* Blank a word that would have been cut off. */
				for(i=maxlen; i>=0; --i)
				{
					char old = line[i];
					line[i] = ' ';
					if(old == ' ')
						break;
				}
				line[maxlen] = 0;
			}
			fprintf(stderr, "\r%s\r", line);
		}
	}
	/* Check for changed tags here too? */
	if( mpg123_meta_check(fr) & MPG123_NEW_ICY && MPG123_OK == mpg123_icy(fr, &icy) )
	fprintf(stderr, "\nICY-META: %s\n", icy);
}

void clear_stat()
{
	fprintf(stderr, "\r                                                                                       \r");
}
