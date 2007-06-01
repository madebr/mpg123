/*
	control_generic.c: control interface for frontends and real console warriors

	copyright 1997-99,2004-6 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Andreas Neuhaus and Michael Hipp
	reworked by Thomas Orgis - it was the entry point for eventually becoming maintainer...
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/wait.h>
#include <sys/socket.h>
#else
#include <winsock.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#include "config.h"
#include "mpg123.h"
#include "common.h"
#include "buffer.h"
#include "icy.h"
#include "debug.h"
#ifdef GAPLESS
#include "layer3.h"
struct audio_info_struct pre_ai;
#endif
#define MODE_STOPPED 0
#define MODE_PLAYING 1
#define MODE_PAUSED 2

extern struct audio_info_struct ai;
extern int buffer_pid;
#ifdef FIFO
#include <sys/stat.h>
int control_file = STDIN_FILENO;
#else
#define control_file STDIN_FILENO
#endif
FILE *outstream;

void generic_sendmsg (const char *fmt, ...)
{
	va_list ap;
	fprintf(outstream, "@");
	va_start(ap, fmt);
	vfprintf(outstream, fmt, ap);
	va_end(ap);
	fprintf(outstream, "\n");
}

void generic_sendstat (struct frame *fr)
{
	unsigned long frames_left;
	double current_seconds, seconds_left;
	if(!position_info(fr, fr->num, xfermem_get_usedspace(buffermem), &ai, &frames_left, &current_seconds, &seconds_left))
	generic_sendmsg("F %li %lu %3.2f %3.2f", fr->num, frames_left, current_seconds, seconds_left);
}

extern char *genre_table[];
extern int genre_count;
void generic_sendinfoid3 (char *buf)
{
	char info[200] = "", *c;
	int i;
	unsigned char genre;
	for (i=0, c=buf+3; i<124; i++, c++)
		info[i] = *c ? *c : ' ';
	info[i] = 0;
	genre = *c;
	generic_sendmsg("I ID3:%s%s", info, (genre<=genre_count) ? genre_table[genre] : "Unknown");
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

int control_generic (struct frame *fr)
{
	struct timeval tv;
	fd_set fds;
	int n;
	int mode = MODE_STOPPED;
	int init = 0;

	/* ThOr */
	char alive = 1;
	char silent = 0;
	unsigned long frame_before = 0;

	/* responses to stderr for frontends needing audio data from stdout */
	if (param.remote_err)
 		outstream = stderr;
 	else
 		outstream = stdout;
 		
#ifndef WIN32
 	setlinebuf(outstream);
#else /* perhaps just use setvbuf as it's C89 */
	setvbuf(outstream, (char*)NULL, _IOLBF, 0);
#endif
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
	/* the command behaviour is different, so is the ID */
	/* now also with version for command availability */
	fprintf(outstream, "@R MPG123 (ThOr) v2\n");

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
				if (!read_frame(fr)) {
					mode = MODE_STOPPED;
					audio_flush(param.outmode, &ai);
					rd->close(rd);
					generic_sendmsg("P 0");
					continue;
				}
				if(!play_frame(init,fr))
				{
					generic_sendmsg("E play_frame failed");
					audio_flush(param.outmode, &ai);
					rd->close(rd);
					mode = MODE_STOPPED;
					generic_sendmsg("P 0");
				}
				if (init) {
					static char tmp[1000];
					make_remote_header(fr, tmp);
					generic_sendmsg(tmp);
					init = 0;
				}
				if(!frame_before && (silent == 0))
				{
					generic_sendstat(fr);
					if (icy.changed && icy.data)
					{
						generic_sendmsg("I ICY-META: %s", icy.data);
						icy.changed = 0;
					}
				}
				if(frame_before) --frame_before;
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
							audio_flush(param.outmode, &ai);
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
						audio_flush(param.outmode, &ai);
						rd->close(rd);
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
						real b,m,t;
						int cn;
						have_eq_settings = TRUE;
						if(sscanf(arg, REAL_SCANF" "REAL_SCANF" "REAL_SCANF, &b, &m, &t) == 3)
						{
							/* very raw line */
							if ((t >= 0) && (t <= 3))
							for(cn=0; cn < 1; ++cn)
							{
								equalizer[0][cn] = b;
								equalizer[1][cn] = b;
							}
							if ((m >= 0) && (m <= 3))
							for(cn=1; cn < 2; ++cn)
							{
								equalizer[0][cn] = m;
								equalizer[1][cn] = m;
							}
							if ((b >= 0) && (b <= 3))
							for(cn=2; cn < 32; ++cn)
							{
								equalizer[0][cn] = t;
								equalizer[1][cn] = t;
							}
							generic_sendmsg("bass: %f mid: %f treble: %f", b, m, t);
						}
						else generic_sendmsg("E invalid arguments for SEQ: %s", arg);
						continue;
					}

					/* Equalizer control :) (JMG) */
					if (!strcasecmp(cmd, "E") || !strcasecmp(cmd, "EQ")) {
						real e; /* ThOr: equalizer is of type real... whatever that is */
						int c, v;
						have_eq_settings = TRUE;
						/*generic_sendmsg("%s",updown);*/
						if(sscanf(arg, "%i %i "REAL_SCANF, &c, &v, &e) == 3)
						{
							equalizer[c][v] = e;
							generic_sendmsg("%i : %i : "REAL_PRINTF, c, v, e);
						}
						else generic_sendmsg("E invalid arguments for EQ: %s", arg);
						continue;
					}

					/* JUMP */
					if (!strcasecmp(cmd, "J") || !strcasecmp(cmd, "JUMP")) {
						char *spos;
						long offset;
						double secs;
						audio_flush(param.outmode, &ai);

						spos = arg;
						if (!spos)
							continue;
						if (mode == MODE_STOPPED)
							continue;

						if(spos[strlen(spos)-1] == 's' && sscanf(arg, "%lf", &secs) == 1) offset = time_to_frame(fr, secs);
						else offset = atol(spos);
						/* totally replaced that stuff - it never fully worked
						   a bit usure about why +pos -> spos+1 earlier... */
						if (spos[0] == '-' || spos[0] == '+')
							offset += frame_before;
						else
							offset -= fr->num;
						
						/* ah, this offset stuff is twisted - I want absolute numbers */
						#ifdef GAPLESS
						if(param.gapless && (fr->lay == 3) && (mode == MODE_PAUSED))
						{
							if(fr->num+offset > 0)
							{
								--offset;
								frame_before = 1;
								if(fr->num+offset > 0)
								{
									--offset;
									++frame_before;
								}
							}
							else frame_before = 0;
						}
						#endif
						if(rd->back_frame(rd, fr, -offset))
						{
							generic_sendmsg("E Error while seeking");
							rd->rewind(rd);
							fr->num = 0;
						}

						#ifdef GAPLESS
						if(param.gapless && (fr->lay == 3))
						{
							prepare_audioinfo(fr, &pre_ai);
							layer3_gapless_set_position(fr->num, fr, &pre_ai);
							layer3_gapless_set_ignore(frame_before, fr, &pre_ai);
						}
						#endif

						generic_sendmsg("J %d", fr->num+frame_before);
						continue;
					}

					/* VOLUME in percent */
					if(!strcasecmp(cmd, "V") || !strcasecmp(cmd, "VOLUME"))
					{
						do_volume(atof(arg)/100);
						generic_sendmsg("V %f%%", outscale / (double) MAXOUTBURST * 100);
						continue;
					}

					/* RVA mode */
					if(!strcasecmp(cmd, "RVA"))
					{
						if(!strcasecmp(arg, "off")) param.rva = RVA_OFF;
						else if(!strcasecmp(arg, "mix") || !strcasecmp(arg, "radio")) param.rva = RVA_MIX;
						else if(!strcasecmp(arg, "album") || !strcasecmp(arg, "audiophile")) param.rva = RVA_ALBUM;
						do_rva();
						generic_sendmsg("RVA %s", rva_name[param.rva]);
						continue;
					}

					/* LOAD - actually play */
					if (!strcasecmp(cmd, "L") || !strcasecmp(cmd, "LOAD")) {
						#ifdef GAPLESS
						frame_before = 0;
						#endif
						if (mode != MODE_STOPPED) {
							rd->close(rd);
							mode = MODE_STOPPED;
						}
						if( open_stream(arg, -1) < 0 ){
							generic_sendmsg("E Error opening stream: %s", arg);
							generic_sendmsg("P 0");
							continue;
						}
						if (rd && rd->flags & READER_ID3TAG)
							generic_sendinfoid3((char *)rd->id3buf);
						else
							generic_sendinfo(arg);

						if (icy.name.fill) generic_sendmsg("I ICY-NAME: %s", icy.name.p);
						if (icy.url.fill) generic_sendmsg("I ICY-URL: %s", icy.url.p);
						mode = MODE_PLAYING;
						init = 1;
						read_frame_init(fr);
						generic_sendmsg("P 2");
						continue;
					}

					/* LOADPAUSED */
					if (!strcasecmp(cmd, "LP") || !strcasecmp(cmd, "LOADPAUSED")) {
						#ifdef GAPLESS
						frame_before = 0;
						#endif
						if (mode != MODE_STOPPED) {
							rd->close(rd);
							mode = MODE_STOPPED;
						}
						if( open_stream(arg, -1) < 0 ){
							generic_sendmsg("E Error opening stream: %s", arg);
							generic_sendmsg("P 0");
							continue;
						}
						if (rd && rd->flags & READER_ID3TAG)
							generic_sendinfoid3((char *)rd->id3buf);
						else
							generic_sendinfo(arg);
						mode = MODE_PAUSED;
						init = 1;
						read_frame_init(fr);
						generic_sendmsg("P 1");
						continue;
					}

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
	} else {
#endif
		audio_flush(param.outmode, &ai);
		free(pcm_sample);
#ifndef NOXFERMEM
	}
#endif
	if (param.outmode == DECODE_AUDIO)
		audio_close(&ai);
	if (param.outmode == DECODE_WAV)
		wav_close();

#ifdef FIFO
	close(control_file); /* be it FIFO or STDIN */
	if(param.fifo) unlink(param.fifo);
#endif
	return 0;
}

/* EOF */
