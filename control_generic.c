/*
 * Control interface to generic front ends.
 * written/copyrights 1997/99 by Andreas Neuhaus (and Michael Hipp)
 *
 * command parsing/processing reworked and partially rewritten in 2004/5 by Thomas Orgis
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>

#include "mpg123.h"
#include "common.h"

#define MODE_STOPPED 0
#define MODE_PLAYING 1
#define MODE_PAUSED 2

extern struct audio_info_struct ai;
extern int buffer_pid;
extern int tabsel_123[2][3][16];

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

void generic_sendstat (struct frame *fr, int no)
{
	long buffsize;
	double bpf, tpf, tim1, tim2;
	double dt = 0;
	int sno, rno;

	/* this is taken from common.c... need to take it back */

	buffsize = xfermem_get_usedspace(buffermem);
	if (!rd || !fr)
		return;
	bpf = compute_bpf(fr);
	tpf = compute_tpf(fr);
	if (buffsize > 0 && ai.rate > 0 && ai.channels > 0) {
		dt = (double) buffsize / ai.rate / ai.channels;
		if ((ai.format & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_16)
			dt *= .5;
	}
	rno = 0;
	sno = no;
	if (rd->filelen >= 0) {
		long t = rd->tell(rd);
		rno = (int)((double)(rd->filelen-t)/bpf);
		sno = (int)((double)t/bpf);
	}
	tim1 = sno * tpf - dt;
	tim2 = rno * tpf + dt;

	generic_sendmsg("F %d %d %3.2f %3.2f", sno, rno, tim1, tim2);
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

void control_generic (struct frame *fr)
{
	struct timeval tv;
	fd_set fds;
	int n;
	int mode = MODE_STOPPED;
	int init = 0;
	int framecnt = 0;

	/* ThOr */
	char alive = 1;
	char silent = 0;
	
	/* responses to stderr for frontends needing audio data from stdout */
	if (param.remote_err)
 		outstream = stderr;
 	else
 		outstream = stdout;
 		
 	setlinebuf(outstream);
	/* the command behaviour is different, so is the ID */
	/* now also with version for command availability */
	fprintf(outstream, "@R MPG123 (ThOr) v2\n");

	while (alive) {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

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
				play_frame(init,fr);
				if (init) {
					static char tmp[1000];
					make_remote_header(fr, tmp);
					generic_sendmsg(tmp);
					init = 0;
				}
				++framecnt;
				if(silent == 0) generic_sendstat(fr, framecnt);
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

		/* exit on error */
		if (n < 0) {
			fprintf(stderr, "Error waiting for command: %s\n", strerror(errno));
			exit(1);
		}

		/* process command */
		if (n > 0) {
			short int len = 1; /* length of buffer */
			short int cnum = 0; /* number of commands */ 
			short int cind = 0; /* index for commands */
			char *cmd, *comstr, *arg; /* variables for parsing, */
			char buf[REMOTE_BUFFER_SIZE];
			char **coms; /* list of commands */
			short int counter;
			coms = malloc(sizeof(*coms)); /* malloc magic */
			coms[0] = &buf[0]; /* first command string */
			
			/* read as much as possible, maybe multiple commands */
			/* When there is nothing to read (EOF) or even an error, it is the end */
			if((len = read(STDIN_FILENO, buf, REMOTE_BUFFER_SIZE)) < 1)	break;
			
			/* one command on a line - separation by \n -> C strings in a row */
			for(counter = 0; counter < len; ++counter) {
				/* line end is command end */
				if( (buf[counter] == '\n') || (buf[counter] == '\r') ) { 
					buf[counter] = 0; /* now it's a properly ending C string */
					/* next "real" char is first of next command */
					if( (counter < (len - 1)) && ((buf[counter+1] == '\n') || (buf[counter+1] == '\r')) )
						++counter; /* skip the additional line ender */
					if(counter < (len - 1)) coms[++cind] = &buf[counter+1];
				}
			}
			cnum = cind+1;

			/*
			   when last command had no \n... should I discard it?
			   Ideally, I should remember the part and wait for next
				 read() to get the rest up to a \n. But that can go
				 to infinity. Too long commands too quickly are just
				 bad. Cannot/Won't change that. So, discard the unfinished
				 command and have fingers crossed that the rest of this
				 unfinished one qualifies as "unknown". 
			*/
			if(buf[len-1] != 0){
				char lasti = buf[len-1];
				buf[len-1] = 0;
				generic_sendmsg("E Unfinished command: %s%c", coms[cind], lasti);
				--cnum;
			}
			
			for(cind = 0; cind < cnum; ++cind) {
				comstr = coms[cind];
				if(strlen(comstr) == 0) continue;

				/* PAUSE */
				if (!strcasecmp(comstr, "P") || !strcasecmp(comstr, "PAUSE")) {
					if (!(mode == MODE_STOPPED))
					{	
						if (mode == MODE_PLAYING) {
							mode = MODE_PAUSED;
							audio_flush(param.outmode, &ai);
							if (param.usebuffer)
								kill(buffer_pid, SIGSTOP);
							generic_sendmsg("P 1");
						} else {
							mode = MODE_PLAYING;
							if (param.usebuffer)
								kill(buffer_pid, SIGCONT);
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
					generic_sendmsg("JUMP/J <frame>: jump to mpeg frame <frame>");
					generic_sendmsg("EQ/E <channenl> <band> <value>: set equalizer value for frequency band on channel");
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
						/* ThOr: The type of real can vary greatly; on my XP1000 with OSF1 I got FPE on setting seq...
							now using the same ifdefs as mpg123.h for the definition of real
							I'd like to have the conversion specifier as constant.
							
							Also, I' not sure _how_ standard these conversion specifiers and their flags are... I have them from a glibc man page.
						*/
						#ifdef REAL_IS_FLOAT
						if(sscanf(arg, "%f %f %f", &b, &m, &t) == 3){
						#elif defined(REAL_IS_LONG_DOUBLE)
						if(sscanf(arg, "%Lf %Lf %Lf", &b, &m, &t) == 3){
						#elif defined(REAL_IS_FIXED)
						if(sscanf(arg, "%ld %ld %ld", &b, &m, &t) == 3){
						#else
						if(sscanf(arg, "%lf %lf %lf", &b, &m, &t) == 3){
						#endif
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
						/* ThOr: This must be done in the header alongside the definition of real somehow! */
						#ifdef REAL_IS_FLOAT
						if(sscanf(arg, "%i %i %f", &c, &v, &e) == 3){
						#elif defined(REAL_IS_LONG_DOUBLE)
						if(sscanf(arg, "%i %i %Lf", &c, &v, &e) == 3){
						#elif defined(REAL_IS_FIXED)
						if(sscanf(arg, "%i %i %ld", &c, &v, &e) == 3){
						#else
						if(sscanf(arg, "%i %i %lf", &c, &v, &e) == 3){
						#endif
							equalizer[c][v] = e;
							generic_sendmsg("%i : %i : %f", c, v, e);
						}
						else generic_sendmsg("E invalid arguments for EQ: %s", arg);
						continue;
					}

					/* JUMP */
					if (!strcasecmp(cmd, "J") || !strcasecmp(cmd, "JUMP")) {
						char *spos;
						int pos, ok;

						spos = arg;
						if (!spos)
							continue;
						if (spos[0] == '-')
							pos = framecnt + atoi(spos);
						else if (spos[0] == '+')
							pos = framecnt + atoi(spos+1);
						else
							pos = atoi(spos);

						if (mode == MODE_STOPPED)
							continue;
						ok = 1;
						if (pos < framecnt) {
							rd->rewind(rd);
							read_frame_init();
							for (framecnt=0; ok && framecnt<pos; framecnt++) {
								ok = read_frame(fr);
								if (fr->lay == 3)
									set_pointer(512);
							}
						} else {
							for (; ok && framecnt<pos; framecnt++) {
								ok = read_frame(fr);
								if (fr->lay == 3)
									set_pointer(512);
							}
						}
						generic_sendmsg("J %d", framecnt);
						continue;
					}

					/* LOAD - actually play */
					if (!strcasecmp(cmd, "L") || !strcasecmp(cmd, "LOAD")) {
						audio_flush(param.outmode, &ai);
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
						mode = MODE_PLAYING;
						init = 1;
						framecnt = 0;
						read_frame_init();
						generic_sendmsg("P 2");
						continue;
					}

					/* LOADPAUSED */
					if (!strcasecmp(cmd, "LP") || !strcasecmp(cmd, "LOADPAUSED")) {
						audio_flush(param.outmode, &ai);
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
						framecnt = 0;
						read_frame_init();
						generic_sendmsg("P 1");
						continue;
					}

					/* no command matched */
					generic_sendmsg("E Unknown command: %s", cmd); /* unknown command */
				} /* end commands with arguments */
				else generic_sendmsg("E Unknown command or no arguments: %s", comstr); /* unknown command */

			} /*end command processing loop */

			free(coms); /* release memory of command string (pointer) array */
				
		} /* end command reading & processing */

	} /* end main (alive) loop */

	/* quit gracefully */
	if (param.usebuffer) {
		kill(buffer_pid, SIGINT);
		xfermem_done_writer(buffermem);
		waitpid(buffer_pid, NULL, 0);
		xfermem_done(buffermem);
	} else {
		audio_flush(param.outmode, &ai);
		free(pcm_sample);
	}
	if (param.outmode == DECODE_AUDIO)
		audio_close(&ai);
	if (param.outmode == DECODE_WAV)
		wav_close();
	exit(0);
}

/* EOF */
