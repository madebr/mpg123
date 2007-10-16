/*
	control_generic.c: control interface for frontends and real console warriors

	copyright 1997-99,2004-6 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Andreas Neuhaus and Michael Hipp
	reworked by Thomas Orgis - it was the entry point for eventually becoming maintainer...
*/

#include <stdarg.h>
#include <sys/time.h>
#ifndef WIN32
#include <sys/wait.h>
#include <sys/socket.h>
#else
#include <winsock.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "mpg123app.h"
#include "common.h"
#include "buffer.h"
#include "genre.h"
#define MODE_STOPPED 0
#define MODE_PLAYING 1
#define MODE_PAUSED 2

extern audio_output_t ao;
extern int buffer_pid;
#ifdef FIFO
#include <sys/stat.h>
int control_file = STDIN_FILENO;
#else
#define control_file STDIN_FILENO
#endif
FILE *outstream;
static int mode = MODE_STOPPED;
static int init = 0;

void generic_sendmsg (const char *fmt, ...)
{
	va_list ap;
	fprintf(outstream, "@");
	va_start(ap, fmt);
	vfprintf(outstream, fmt, ap);
	va_end(ap);
	fprintf(outstream, "\n");
}

void generic_sendstat (mpg123_handle *fr)
{
	long current_frame, frames_left;
	double current_seconds, seconds_left;
	if(!mpg123_position(fr, 0, xfermem_get_usedspace(buffermem), &current_frame, &frames_left, &current_seconds, &seconds_left))
	generic_sendmsg("F %li %li %3.2f %3.2f", current_frame, frames_left, current_seconds, seconds_left);
}

void generic_sendinfoid3(mpg123_handle *mh)
{
	char info[125] = "";
	int i;
	mpg123_id3v1 *v1;
	mpg123_id3v2 *v2;
	if(MPG123_OK != mpg123_id3(mh, &v1, &v2))
	{
		error1("Cannot get ID3 data: %s", mpg123_strerror(mh));
		return;
	}
	if(v1 == NULL) return;
	memcpy(info,    v1->title,   	30);
	memcpy(info+30, v1->artist,  30);
	memcpy(info+60, v1->album,   30);
	memcpy(info+90, v1->year,     4);
	memcpy(info+94, v1->comment, 30);
	for(i=0;i<124; ++i) if(info[i] == 0) info[i] = ' ';
	info[i] = 0;
	generic_sendmsg("I ID3:%s%s", info, (v1->genre<=genre_count) ? genre_table[v1->genre] : "Unknown");
}

void generic_sendinfo (char *filename)
{
	char *s, *t;
	s = strrchr(filename, '/');
	if (!s)
		s = filename;
	else
		s++;
	t = strrchr(s, '.');
	if (t)
		*t = 0;
	generic_sendmsg("I %s", s);
}

static void generic_load(mpg123_handle *fr, char *arg, int state)
{
	if(mode != MODE_STOPPED)
	{
		close_track();
		mode = MODE_STOPPED;
	}
	if(!open_track(arg))
	{
		generic_sendmsg("E Error opening stream: %s", arg);
		generic_sendmsg("P 0");
		return;
	}
	if(mpg123_meta_check(fr) & MPG123_NEW_ID3) generic_sendinfoid3(fr);
	else generic_sendinfo(arg);

	if(htd.icy_name.fill) generic_sendmsg("I ICY-NAME: %s", htd.icy_name.p);
	if(htd.icy_url.fill)  generic_sendmsg("I ICY-URL: %s", htd.icy_url.p);
	mode = state;
	init = 1;
	generic_sendmsg(mode == MODE_PAUSED ? "P 1" : "P 2");
}

int control_generic (mpg123_handle *fr)
{
	struct timeval tv;
	fd_set fds;
	int n;

	/* ThOr */
	char alive = 1;
	char silent = 0;

	/* responses to stderr for frontends needing audio data from stdout */
	if (param.remote_err)
 		outstream = stderr;
 	else
 		outstream = stdout;
 		
#ifndef WIN32
 	setlinebuf(outstream);
#else /* perhaps just use setvbuf as it's C89 */
	fprintf(outstream, "You are on Win32 and want to use the control interface... tough luck: We need a replacement for select on STDIN first.\n");
	return 0;
	setvbuf(outstream, (char*)NULL, _IOLBF, 0);
#endif
	/* the command behaviour is different, so is the ID */
	/* now also with version for command availability */
	fprintf(outstream, "@R MPG123 (ThOr) v2\n");
#ifdef FIFO
	if(param.fifo)
	{
		if(param.fifo[0] == 0)
		{
			error("You wanted an empty FIFO name??");
			return 1;
		}
		unlink(param.fifo);
		if(mkfifo(param.fifo, 0666) == -1)
		{
			error2("Failed to create FIFO at %s (%s)", param.fifo, strerror(errno));
			return 1;
		}
		debug("going to open named pipe ... blocking until someone gives command");
		control_file = open(param.fifo,O_RDONLY);
		debug("opened");
	}
#endif

	while (alive)
	{
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(control_file, &fds);
		/* play frame if no command needs to be processed */
		if (mode == MODE_PLAYING) {
			n = select(32, &fds, NULL, NULL, &tv);
			if (n == 0) {
				if (!play_frame())
				{
					mode = MODE_STOPPED;
					close_track();
					generic_sendmsg("P 0");
					continue;
				}
				if (init) {
					print_remote_header(fr);
					init = 0;
				}
				if(silent == 0)
				{
					generic_sendstat(fr);
					if(mpg123_meta_check(fr) & MPG123_NEW_ICY)
					{
						char *meta;
						if(mpg123_icy(fr, &meta) == MPG123_OK)
						generic_sendmsg("I ICY-META: %s", meta != NULL ? meta : "<nil>");
					}
				}
			}
		}
		else {
			/* wait for command */
			while (1) {
				n = select(32, &fds, NULL, NULL, NULL);
				if (n > 0)
					break;
			}
		}

		/*  on error */
		if (n < 0) {
			fprintf(stderr, "Error waiting for command: %s\n", strerror(errno));
			return 1;
		}

		/* read & process commands */
		if (n > 0)
		{
			short int len = 1; /* length of buffer */
			char *cmd, *arg; /* variables for parsing, */
			char *comstr = NULL; /* gcc thinks that this could be used uninitialited... */ 
			char buf[REMOTE_BUFFER_SIZE];
			short int counter;
			char *next_comstr = buf; /* have it initialized for first command */

			/* read as much as possible, maybe multiple commands */
			/* When there is nothing to read (EOF) or even an error, it is the end */
			if((len = read(control_file, buf, REMOTE_BUFFER_SIZE)) < 1)
			{
#ifdef FIFO
				if(len == 0 && param.fifo)
				{
					debug("fifo ended... reopening");
					close(control_file);
					control_file = open(param.fifo,O_RDONLY|O_NONBLOCK);
					if(control_file < 0){ error1("open of fifo failed... %s", strerror(errno)); break; }
					continue;
				}
#endif
				if(len < 0) error1("command read error: %s", strerror(errno));
				break;
			}

			debug1("read %i bytes of commands", len);
			/* one command on a line - separation by \n -> C strings in a row */
			for(counter = 0; counter < len; ++counter)
			{
				/* line end is command end */
				if( (buf[counter] == '\n') || (buf[counter] == '\r') )
				{
					debug1("line end at counter=%i", counter);
					buf[counter] = 0; /* now it's a properly ending C string */
					comstr = next_comstr;

					/* skip the additional line ender of \r\n or \n\r */
					if( (counter < (len - 1)) && ((buf[counter+1] == '\n') || (buf[counter+1] == '\r')) ) buf[++counter] = 0;

					/* next "real" char is first of next command */
					next_comstr = buf + counter+1;

					/* directly process the command now */
					debug1("interpreting command: %s", comstr);
				if(strlen(comstr) == 0) continue;

				/* PAUSE */
				if (!strcasecmp(comstr, "P") || !strcasecmp(comstr, "PAUSE")) {
					if (!(mode == MODE_STOPPED))
					{	
						if (mode == MODE_PLAYING) {
							mode = MODE_PAUSED;
							buffer_stop();
							generic_sendmsg("P 1");
						} else {
							mode = MODE_PLAYING;
							buffer_start();
							generic_sendmsg("P 2");
						}
					}
					continue;
				}

				/* STOP */
				if (!strcasecmp(comstr, "S") || !strcasecmp(comstr, "STOP")) {
					if (mode != MODE_STOPPED) {
						close_track();
						mode = MODE_STOPPED;
						generic_sendmsg("P 0");
					}
					continue;
				}

				/* SILENCE */
				if(!strcasecmp(comstr, "SILENCE")) {
					silent = 1;
					generic_sendmsg("silence");
					continue;
				}

				/* QUIT */
				if (!strcasecmp(comstr, "Q") || !strcasecmp(comstr, "QUIT")){
					alive = FALSE; continue;
				}

				/* some HELP */
				if (!strcasecmp(comstr, "H") || !strcasecmp(comstr, "HELP")) {
					generic_sendmsg("HELP/H: command listing (LONG/SHORT forms), command case insensitve");
					generic_sendmsg("LOAD/L <trackname>: load and start playing resource <trackname>");
					generic_sendmsg("LOADPAUSED/LP <trackname>: load and start playing resource <trackname>");
					generic_sendmsg("PAUSE/P: pause playback");
					generic_sendmsg("STOP/S: stop playback (closes file)");
					generic_sendmsg("JUMP/J <frame>|<+offset>|<-offset>|<[+|-]seconds>s: jump to mpeg frame <frame> or change position by offset, same in seconds if number followed by \"s\"");
					generic_sendmsg("VOLUME/V <percent>: set volume in % (0..100...); float value");
					generic_sendmsg("RVA off|(mix|radio)|(album|audiophile): set rva mode");
					generic_sendmsg("EQ/E <channel> <band> <value>: set equalizer value for frequency band on channel");
					generic_sendmsg("SEEK/K <sample>|<+offset>|<-offset>: jump to output sample position <samples> or change position by offset");
					generic_sendmsg("SEQ <bass> <mid> <treble>: simple eq setting...");
					generic_sendmsg("SILENCE: be silent during playback (meaning silence in text form)");
					generic_sendmsg("meaning of the @S stream info:");
					generic_sendmsg(remote_header_help);
					continue;
				}

				/* commands with arguments */
				cmd = NULL;
				arg = NULL;
				cmd = strtok(comstr," \t"); /* get the main command */
				arg = strtok(NULL,""); /* get the args */

				if (cmd && strlen(cmd) && arg && strlen(arg))
				{
					/* Simple EQ: SEQ <BASS> <MID> <TREBLE>  */
					if (!strcasecmp(cmd, "SEQ")) {
						double b,m,t;
						int cn;
						if(sscanf(arg, "%lf %lf %lf", &b, &m, &t) == 3)
						{
							/* Consider adding mpg123_seq()... but also, on could define a nicer courve for that. */
							if ((t >= 0) && (t <= 3))
							for(cn=0; cn < 1; ++cn)	mpg123_eq(fr, MPG123_LEFT|MPG123_RIGHT, cn, b);

							if ((m >= 0) && (m <= 3))
							for(cn=1; cn < 2; ++cn) mpg123_eq(fr, MPG123_LEFT|MPG123_RIGHT, cn, m);

							if ((b >= 0) && (b <= 3))
							for(cn=2; cn < 32; ++cn) mpg123_eq(fr, MPG123_LEFT|MPG123_RIGHT, cn, t);

							generic_sendmsg("bass: %f mid: %f treble: %f", b, m, t);
						}
						else generic_sendmsg("E invalid arguments for SEQ: %s", arg);
						continue;
					}

					/* Equalizer control :) (JMG) */
					if (!strcasecmp(cmd, "E") || !strcasecmp(cmd, "EQ")) {
						double e; /* ThOr: equalizer is of type real... whatever that is */
						int c, v;
						/*generic_sendmsg("%s",updown);*/
						if(sscanf(arg, "%i %i %lf", &c, &v, &e) == 3)
						{
							mpg123_eq(fr, c, v, e);
							generic_sendmsg("%i : %i : %f", c, v, e);
						}
						else generic_sendmsg("E invalid arguments for EQ: %s", arg);
						continue;
					}

					/* SEEK to a sample offset */
					if(!strcasecmp(cmd, "K") || !strcasecmp(cmd, "SEEK"))
					{
						off_t soff;
						char *spos = arg;
						int whence = SEEK_SET;
						if(!spos || (mode == MODE_STOPPED)) continue;

						soff = atol(spos);
						if(spos[0] == '-' || spos[0] == '+') whence = SEEK_CUR;
						if(0 > (soff = mpg123_seek(fr, soff, whence)))
						{
							generic_sendmsg("E Error while seeking: %s", mpg123_strerror(fr));
							mpg123_seek(fr, 0, SEEK_SET);
						}
						generic_sendmsg("K %li", (long)mpg123_tell(fr));
						continue;
					}
					/* JUMP */
					if (!strcasecmp(cmd, "J") || !strcasecmp(cmd, "JUMP")) {
						char *spos;
						off_t offset;
						double secs;

						spos = arg;
						if (!spos)
							continue;
						if (mode == MODE_STOPPED)
							continue;

						if(spos[strlen(spos)-1] == 's' && sscanf(arg, "%lf", &secs) == 1) offset = mpg123_timeframe(fr, secs);
						else offset = atol(spos);
						/* totally replaced that stuff - it never fully worked
						   a bit usure about why +pos -> spos+1 earlier... */
						if (spos[0] == '-' || spos[0] == '+') offset += framenum;

						if(0 > (framenum = mpg123_seek_frame(fr, offset, SEEK_SET)))
						{
							generic_sendmsg("E Error while seeking");
							mpg123_seek_frame(fr, 0, SEEK_SET);
						}
						if(param.usebuffer)	buffer_resync();

<<<<<<< .working
						#ifdef GAPLESS
						if(param.gapless && (fr->lay == 3))
						{
							prepare_audioinfo(fr, &pre_ao);
							layer3_gapless_set_position(fr->num, fr, &pre_ao);
							layer3_gapless_set_ignore(frame_before, fr, &pre_ao);
						}
						#endif

						generic_sendmsg("J %d", fr->num+frame_before);
=======
						generic_sendmsg("J %d", framenum);
>>>>>>> .merge-right.r998
						continue;
					}

					/* VOLUME in percent */
					if(!strcasecmp(cmd, "V") || !strcasecmp(cmd, "VOLUME"))
					{
						double v;
						mpg123_volume(fr, atof(arg)/100);
						mpg123_getvolume(fr, &v, NULL, NULL); /* Necessary? */
						generic_sendmsg("V %f%%", v * 100);
						continue;
					}

					/* RVA mode */
					if(!strcasecmp(cmd, "RVA"))
					{
						if(!strcasecmp(arg, "off")) param.rva = MPG123_RVA_OFF;
						else if(!strcasecmp(arg, "mix") || !strcasecmp(arg, "radio")) param.rva = MPG123_RVA_MIX;
						else if(!strcasecmp(arg, "album") || !strcasecmp(arg, "audiophile")) param.rva = MPG123_RVA_ALBUM;
						mpg123_volume(fr, -1);
						generic_sendmsg("RVA %s", rva_name[param.rva]);
						continue;
					}

					/* LOAD - actually play */
					if (!strcasecmp(cmd, "L") || !strcasecmp(cmd, "LOAD")){ generic_load(fr, arg, MODE_PLAYING); continue; }

					/* LOADPAUSED */
					if (!strcasecmp(cmd, "LP") || !strcasecmp(cmd, "LOADPAUSED")){ generic_load(fr, arg, MODE_PAUSED); continue; }

					/* no command matched */
					generic_sendmsg("E Unknown command: %s", cmd); /* unknown command */
				} /* end commands with arguments */
				else generic_sendmsg("E Unknown command or no arguments: %s", comstr); /* unknown command */

				} /* end of single command processing */
			} /* end of scanning the command buffer */

			/*
			   when last command had no \n... should I discard it?
			   Ideally, I should remember the part and wait for next
				 read() to get the rest up to a \n. But that can go
				 to infinity. Too long commands too quickly are just
				 bad. Cannot/Won't change that. So, discard the unfinished
				 command and have fingers crossed that the rest of this
				 unfinished one qualifies as "unknown". 
			*/
			if(buf[len-1] != 0)
			{
				char lasti = buf[len-1];
				buf[len-1] = 0;
				generic_sendmsg("E Unfinished command: %s%c", comstr, lasti);
			}
		} /* end command reading & processing */
	} /* end main (alive) loop */

	/* quit gracefully */
#ifndef NOXFERMEM
	if (param.usebuffer) {
		kill(buffer_pid, SIGINT);
		xfermem_done_writer(buffermem);
		waitpid(buffer_pid, NULL, 0);
		xfermem_done(buffermem);
	}
#endif
	if (param.outmode == DECODE_AUDIO)
		ao.close(&ao);
	if (param.outmode == DECODE_WAV)
		wav_close();

#ifdef FIFO
	close(control_file); /* be it FIFO or STDIN */
	if(param.fifo) unlink(param.fifo);
#endif
	return 0;
}

/* EOF */
