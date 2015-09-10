/*
	buffer.c: output buffer

	copyright 1997-2015 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Oliver Fromme

	I (ThOr) am reviewing this file at about the same daytime as Oliver's timestamp here:
	Mon Apr 14 03:53:18 MET DST 1997
	- dammed night coders;-)

	This has been heavily reworked to be barely recognizable for the creation of
	libout123. There is more structure in the communication, as is necessary if
	the libout123 functionality is offered via some API to unknown client
	programs instead of being used from mpg123 alone. The basic idea is the same,
	the xfermem part only sligthly modified for more synchronization, as I sensed
	potential deadlocks. --ThOr
*/

/*
	Communication to the buffer is normally via xfermem_putcmd() and blocking
	on a response, relying on the buffer process periodically checking for
	pending commands.

	For more immediate concerns, you can send SIGINT. The only result is that this
	interrupts a current device writing operation and causes the buffer to wait
	for a following command.
*/

#include "buffer.h"
#include "out123_int.h"
#include "xfermem.h"
#include <errno.h>
#include "debug.h"
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#define BUF_CMD_OPEN     XF_CMD_CUSTOM1
#define BUF_CMD_CLOSE    XF_CMD_CUSTOM2
#define BUF_CMD_START    XF_CMD_CUSTOM3
#define BUF_CMD_STOP     XF_CMD_CUSTOM4
#define BUF_CMD_AUDIOCAP XF_CMD_CUSTOM5
#define BUF_CMD_PARAM    XF_CMD_CUSTOM6
#define BUF_CMD_NDRAIN   XF_CMD_CUSTOM7
#define BUF_CMD_AUDIOFMT XF_CMD_CUSTOM8

/* TODO: Dynamically allocate that to allow multiple instances. */
int outburst = 32768;

/* This is static and global for the forked buffer process.
   Another forked buffer process will have its on value. */
static int intflag = FALSE;

static void catch_interrupt (void)
{
	intflag = TRUE;
}

static int write_string(out123_handle *ao, int who, const char *buf);
static int read_record(out123_handle *ao
,	int who, void **buf, byte *prebuf, int *preoff, int presize, size_t *recsize);
static int buffer_loop(out123_handle *ao);

static void catch_child(void)
{
	/* Disabled for now. We do not really need that.
	   Rather get return status in a controlled way in buffer_exit(). */
	/* while (waitpid(-1, NULL, WNOHANG) > 0); */
}

/*
	Functions called from the controlling process.
*/

/* Start a buffer process. */
int buffer_init(out123_handle *ao, size_t bytes)
{
	buffer_exit(ao);
	if(bytes < outburst) bytes = 2*outburst;

#ifdef DONT_CATCH_SIGNALS
#error I really need to catch signals here!
#endif
	xfermem_init(&ao->buffermem, bytes, 0, 0);
	/* Is catch_child() really useful? buffer_exit() does waitpid().
	   And if buffer_exit() is not called, the main process might be
	   killed off and not be able to run a signal handler anyway. */
	catchsignal(SIGCHLD, catch_child);
	switch((ao->buffer_pid = fork()))
	{
		case -1: /* error */
			error("cannot fork!");
			goto buffer_init_bad;
		case 0: /* child */
		{
			int ret;
			/*
				Ensure the normal default value for buffer_pid to be able
				to call normal out123 routines from the buffer proess.
				One could keep it at zero and even use this for detecting the
				buffer process and do special stuff for that. But the point
				is that there shouldn't be special stuff.
			*/
			ao->buffer_pid = -1;
			/* Not preparing audio output anymore, that comes later. */
			xfermem_init_reader(ao->buffermem);
			ret = buffer_loop(ao); /* Here the work happens. */
			xfermem_done_reader(ao->buffermem);
			xfermem_done(ao->buffermem);
			/* Proper cleanup of output handle, including out123_close(). */
			out123_del(ao);
			exit(ret);
		}
		default: /* parent */
		{
			int cmd;
			xfermem_init_writer(ao->buffermem);
			debug("waiting for inital pong from buffer process");
			if( (cmd=xfermem_getcmd(ao->buffermem->fd[XF_WRITER], TRUE))
			    != XF_CMD_PONG )
			{
				if(!AOQUIET)
					error2("Got %i instead of expected initial response %i. Killing rogue buffer process."
					,	cmd, XF_CMD_PONG);
				kill(ao->buffer_pid, SIGKILL);
				buffer_exit(ao);
				return -1;
			}
		}
	}

	return 0;
buffer_init_bad:
	if(ao->buffermem)
	{
		xfermem_done(ao->buffermem);
		ao->buffermem = NULL;
	}
	return -1;
}

/* End a buffer process. */
void buffer_exit(out123_handle *ao)
{
	int status = 0;
	if(ao->buffer_pid == -1) return;

	debug("ending buffer");
	buffer_stop(ao); /* Puts buffer into waiting-for-command mode. */
	buffer_end(ao);  /* Gives command to end operation. */
	xfermem_done_writer(ao->buffermem);
	waitpid(ao->buffer_pid, &status, 0);
	xfermem_done(ao->buffermem);
	ao->buffermem = NULL;
	ao->buffer_pid = -1;
	if(WIFEXITED(status))
	{
		int ret = WEXITSTATUS(status);
		if(ret && !AOQUIET)
			error1("Buffer process isses arose, non-zero return value %i.", ret);
	}
	else if(!AOQUIET)
		error("Buffer process did not exit normally.");
}

/*
	Communication from writer to reader (buffer process).
	Remember: The ao struct here is the writer's instance.
*/

static int buffer_cmd_finish(out123_handle *ao)
{
	/* Only if buffer returns XF_CMD_OK we got lucky. Otherwise, we expect
	   the buffer to deliver a reason right after XF_CMD_ERROR. */
	switch(xfermem_getcmd(ao->buffermem->fd[XF_WRITER], TRUE))
	{
		case XF_CMD_OK: return 0;
		case XF_CMD_ERROR:
			if(!GOOD_READVAL(ao->buffermem->fd[XF_WRITER], ao->errcode))
				ao->errcode = OUT123_BUFFER_ERROR;
			return -1;
		break;
		default:
			ao->errcode = OUT123_BUFFER_ERROR;
			return -1;
	}
}

int buffer_sync_param(out123_handle *ao)
{
	int writerfd = ao->buffermem->fd[XF_WRITER];
	if(xfermem_putcmd(writerfd, BUF_CMD_PARAM) != 1)
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return -1;
	}
	/* Calling an external serialization routine to avoid forgetting
	   any fresh parameters here. */
	if(write_parameters(ao, writerfd))
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return -1;
	}
	return buffer_cmd_finish(ao);
}

int buffer_open(out123_handle *ao, const char* driver, const char* device)
{
	int writerfd = ao->buffermem->fd[XF_WRITER];

	if(xfermem_putcmd(writerfd, BUF_CMD_OPEN) != 1)
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return -1;
	}
	/* Passing over driver and device name. */
	if(  write_string(ao, XF_WRITER, driver)
	  || write_string(ao, XF_WRITER, device) )
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return -1;
	}

	if(buffer_cmd_finish(ao) == 0)
	{
		/* Retrieve driver and device name. */
		if(
			read_record(ao, XF_WRITER, (void**)&ao->driver, NULL, NULL, 0, NULL)
		||	read_record(ao, XF_WRITER, (void**)&ao->device, NULL, NULL, 0, NULL)
		){
			ao->errcode = OUT123_BUFFER_ERROR;
			return -1;
		}
	}
	return 0;
}

int buffer_encodings(out123_handle *ao)
{
	int writerfd = ao->buffermem->fd[XF_WRITER];

	if(xfermem_putcmd(writerfd, BUF_CMD_AUDIOCAP) != 1)
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return -1;
	}
	/* Now shoving over the parameters for opening the device. */
	if(
		!GOOD_WRITEVAL(writerfd, ao->channels)
	||	!GOOD_WRITEVAL(writerfd, ao->rate)
	)
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return -1;
	}

	if(buffer_cmd_finish(ao) == 0)
	{
		int encodings;
		/* If all good, the answer can be read how. */
		if(!GOOD_READVAL(writerfd, encodings))
		{
			ao->errcode = OUT123_BUFFER_ERROR;
			return -1;
		}
		else return encodings;
	}
	else return -1;
}

int buffer_formats( out123_handle *ao, const long *rates, int ratecount
                  , int minchannels, int maxchannels
                  , struct mpg123_fmt **fmtlist )
{
	int writerfd = ao->buffermem->fd[XF_WRITER];
	size_t ratesize;

	if(xfermem_putcmd(writerfd, BUF_CMD_AUDIOFMT) != 1)
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return -1;
	}

	ratesize = ratecount*sizeof(rates);

	if(
		!GOOD_WRITEVAL(writerfd, maxchannels)
	||	!GOOD_WRITEVAL(writerfd, minchannels)
	||	!GOOD_WRITEVAL(writerfd, ratesize)
	||	!GOOD_WRITEBUF(writerfd, rates, ratesize)
	){
		ao->errcode = OUT123_BUFFER_ERROR;
		return -1;
	}
	if(buffer_cmd_finish(ao) == 0)
	{
		int fmtcount;
		size_t fmtsize;
		if(
			!GOOD_READVAL(writerfd, fmtcount)
		||	read_record(ao, XF_WRITER, (void**)fmtlist, NULL, NULL, 0, &fmtsize)
		){
			ao->errcode = OUT123_BUFFER_ERROR;
			return -1;
		} else
			return fmtsize/sizeof(struct mpg123_fmt);
	}
	else return -1;
}

int buffer_start(out123_handle *ao)
{
	int writerfd = ao->buffermem->fd[XF_WRITER];
	if(xfermem_putcmd(writerfd, BUF_CMD_START) != 1)
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return -1;
	}
	/* Now shoving over the parameters for opening the device. */
	if(
		!GOOD_WRITEVAL(writerfd, ao->format)
	||	!GOOD_WRITEVAL(writerfd, ao->channels)
	|| !GOOD_WRITEVAL(writerfd, ao->rate)
	)
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return -1;
	}

	return buffer_cmd_finish(ao);
}

#define BUFFER_SIMPLE_CONTROL(name, cmd) \
void name(out123_handle *ao) \
{ \
	xfermem_putcmd(ao->buffermem->fd[XF_WRITER], cmd); \
	xfermem_getcmd(ao->buffermem->fd[XF_WRITER], TRUE); \
}

BUFFER_SIMPLE_CONTROL(buffer_stop,  BUF_CMD_STOP)
BUFFER_SIMPLE_CONTROL(buffer_continue, XF_CMD_CONTINUE)
BUFFER_SIMPLE_CONTROL(buffer_ignore_lowmem, XF_CMD_IGNLOW)
BUFFER_SIMPLE_CONTROL(buffer_drain, XF_CMD_DRAIN)
BUFFER_SIMPLE_CONTROL(buffer_end, XF_CMD_TERMINATE)
BUFFER_SIMPLE_CONTROL(buffer_close, BUF_CMD_CLOSE)

#define BUFFER_SIGNAL_CONTROL(name, cmd) \
void name(out123_handle *ao) \
{ \
	kill(ao->buffer_pid, SIGINT); \
	xfermem_putcmd(ao->buffermem->fd[XF_WRITER], cmd); \
	xfermem_getcmd(ao->buffermem->fd[XF_WRITER], TRUE); \
}

BUFFER_SIGNAL_CONTROL(buffer_pause, XF_CMD_PAUSE)
BUFFER_SIGNAL_CONTROL(buffer_drop, XF_CMD_DROP)

size_t buffer_fill(out123_handle *ao)
{
	return xfermem_get_usedspace(ao->buffermem);
}

void buffer_ndrain(out123_handle *ao, size_t bytes)
{
	size_t oldfill;
	int writerfd = ao->buffermem->fd[XF_WRITER];

	oldfill = buffer_fill(ao);
	if(xfermem_putcmd(writerfd, BUF_CMD_NDRAIN) != 1)
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return;
	}
	/* Now shoving over the parameters for opening the device. */
	if(  !GOOD_WRITEVAL(writerfd, bytes)
	  || !GOOD_WRITEVAL(writerfd, oldfill) )
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return;
	}

	buffer_cmd_finish(ao);
}

/* The workhorse: Send data to the buffer with some synchronization and even
   error checking. */
size_t buffer_write(out123_handle *ao, void *buffer, size_t bytes)
{
	/*
		Writing the whole buffer in one piece is no good as that means
		waiting for the buffer being empty. That is called a buffer underrun.
		We want to refill the buffer before that happens. So, what is sane?
	*/
	size_t written = 0;
	size_t max_piece = ao->buffermem->size / 2;
	while(bytes)
	{
		size_t count_piece = bytes > max_piece
		?	max_piece
		:	bytes;
		int ret = xfermem_write(ao->buffermem
		,	(char*)buffer+written, count_piece);
		if(ret)
		{
			if(!AOQUIET)
				error1("writing to buffer memory failed (%i)", ret);
			if(ret == XF_CMD_ERROR)
			{
				/* Buffer tells me that it has an error waiting. */
				if(!GOOD_READVAL(ao->buffermem->fd[XF_WRITER], ao->errcode))
					ao->errcode = OUT123_BUFFER_ERROR;
			}
			return 0;
		}
		bytes   -= count_piece;
		written += count_piece;
	}
	return written;
}


/*
	Code for the buffer process itself.
*/

/*

buffer loop:

{
	1. normal operation: get data, feed to audio device
	   (if device open and alive, if data there, if no other command pending)
	2. command response: pause/unpause, open module/device, query caps

	One command at a time, synchronized ... writer process blocks, waiting for
	response.
}

*/

/*
	Fill buffer to that value when starting playback from stopped state or after
	experiencing a serious underrun.
	One might also define intermediate preload to recover from underruns. Earlier
	code used 1/8 of the buffer.
*/
static size_t preload_size(out123_handle *ao)
{
	size_t preload = 0;
	txfermem *xf = ao->buffermem;
	/* Fill configured part of buffer on first run before starting to play.
	 * Live mp3 streams constantly approach buffer underrun otherwise. [dk]
	 */
	if(ao->preload > 0.)     preload = (size_t)(ao->preload*xf->size);
	if(preload > xf->size/2) preload = xf->size/2;

	return preload;
}

/* Play one piece of audio from the buffer after settling preload etc.
   On error, the device is closed and this naturally stops playback
   as that depends on ao->state == play_live.
   This plays _at_ _most_ the given amount of bytes, usually less. */
static void buffer_play(out123_handle *ao, size_t bytes)
{
	int written;
	txfermem *xf = ao->buffermem;

	/* Settle amount of bytes accessible in one block. */
	if (bytes > xf->size - xf->readindex)
		bytes = xf->size - xf->readindex;
	/* Not more than configured output block. */
	if (bytes > outburst)
		bytes = outburst;
	/* The output can only take multiples of framesize. */
	bytes -= bytes % ao->framesize;
	/* Now do a normal ao->write(), with interruptions by signals
		being expected. */
	errno = 0;
	written = ao->write(ao, (unsigned char*)xf->data+xf->readindex, (int)bytes);
	debug2("buffer wrote %i B / %i B to device", written, (int)bytes);
	if(written >= 0)
		/* Advance read pointer by the amount of written bytes. */
		xf->readindex = (xf->readindex + written) % xf->size;
	else if(errno != EINTR && errno != ERESTART)
	{
		ao->errcode = OUT123_DEV_PLAY;
		if(!(ao->flags & OUT123_QUIET))
			error1("Error in writing audio (%s?)!", strerror(errno));
		out123_close(ao);
	}
}

/* Now I'm getting really paranoid: Helper to skip bytes from command
   channel if we cannot allocate enough memory to hold the data. */
static void skip_bytes(int fd, size_t count)
{
	while(count)
	{
		char buf[1024];
		if(!unintr_read(fd, buf, (count < sizeof(buf) ? count : sizeof(buf))))
			return;
	}
}

/* Write a string to command channel.
   Return 0 on success, set ao->errcode on issues. */
static int write_string(out123_handle *ao, int who, const char *buf)
{
	txfermem *xf = ao->buffermem;
	int my_fd = xf->fd[who];
	size_t len;

	/* A NULL string is passed als zero bytes. */
	len = buf ? (strlen(buf)+1) : 0;
	if( !GOOD_WRITEVAL(my_fd, len)
	 || !GOOD_WRITEBUF(my_fd, buf, len) )
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return 2;
	}
	return 0;
}

/* Read a value from command channel with prebuffer.
   This assumes responsible use and avoids needless checking of input.
   And, yes, it modifies the preoff argument!
   Returns 0 on success, modifies prebuffer fill. */
int read_buf(int fd, void *addr, size_t size, byte *prebuf, int *preoff, int presize)
{
	size_t need = size;

	if(prebuf)
	{
		int have = presize - *preoff;
		if(have > need)
			have = need;
		memcpy(addr, prebuf+*preoff, have);
		*preoff += have;
		addr = (char*)addr+have;
		need -= have;
	}
	if(need)
		return !GOOD_READBUF(fd, addr, need);
	else
		return 0;
}

/* Read a record of unspecified type from command channel.
   Return 0 on success, set ao->errcode on issues. */
static int read_record(out123_handle *ao
,	int who, void **buf, byte *prebuf, int *preoff, int presize
,	size_t *reclen)
{
	txfermem *xf = ao->buffermem;
	int my_fd = xf->fd[who];
	size_t len;

	if(*buf)
		free(*buf);
	*buf = NULL;

	if(read_buf(my_fd, &len, sizeof(len), prebuf, preoff, presize))
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		return 2;
	}
	if(reclen)
		*reclen = len;
	/* If there is an insane length of given, that shall be handled. */
	if(len && !(*buf = malloc(len)))
	{
		ao->errcode = OUT123_DOOM;
		skip_bytes(my_fd, len);
		return -1;
	}
	if(read_buf(my_fd, *buf, len, prebuf, preoff, presize))
	{
		ao->errcode = OUT123_BUFFER_ERROR;
		free(*buf);
		*buf = NULL;
		return 2;
	}
	return 0;
}


/* The main loop, returns 0 when no issue occured. */
int buffer_loop(out123_handle *ao)
{
	txfermem *xf = ao->buffermem;
	int my_fd = xf->fd[XF_READER];
	int preloading = FALSE;

	/* Be prepared to use SIGINT for communication. */
	catchsignal (SIGINT, catch_interrupt);
	/* sigprocmask (SIG_SETMASK, oldsigset, NULL); */
	/* Say hello to the writer. */
	xfermem_putcmd(my_fd, XF_CMD_PONG);

	debug1("buffer with preload %g", ao->preload);
	while(1)
	{
		/* If a device is opened and playing, it is our first duty to keep it playing. */
		if(ao->state == play_live)
		{
			size_t bytes = xfermem_get_usedspace(xf);
			debug2( "Play or preload? Got %"SIZE_P" B / %"SIZE_P" B."
			,	(size_p)bytes, (size_p)preload_size(ao) );
			if(preloading)
				preloading = (bytes < preload_size(ao));
			if(!preloading)
			{
				if(bytes < outburst)
					preloading = TRUE;
				else
					buffer_play(ao, bytes);
			}
		}
		/* Now always check for a pending command, in a blocking way if there is
		   no playback. */
		debug1("Buffer cmd? (Interruped: %i)", intflag);
		/*
			The writer only ever signals before sending a command and also waiting
			for a response. So, the right place to reset the flag is any time
			before giving the response. But let's ensure two things:
			1. The flag really is only cleared when a command response is given.
			2. Command parsing does not stop until a command demanding a response
			   was handled.
		*/
		do
		{
			/* Getting a whole block of commands to efficiently process those
			   XF_CMD_DATA messages. */
			byte cmd[100];
			int cmdcount;
			int i;

			cmdcount = xfermem_getcmds( my_fd
			,	(preloading || intflag || (ao->state != play_live))
			,	cmd
			,	sizeof(cmd) );
			if(cmdcount < 0)
			{
				error1("Reading a command set returned %i, my link is broken.", cmdcount);
				return 1;
			}
#ifdef DEBUG
			for(i=0; i<cmdcount; ++i)
				debug2("cmd[%i]=%u", i, cmd[i]);
#endif
			/*
				These actions should rely heavily on calling the normal out123
				API functions, just with some parameter passing and error checking
				wrapped around. If there is much code here, it is wrong.
			*/
			for(i=0; i<cmdcount;) switch(cmd[i++])
			{
#define GOOD_READVAL_BUF(fd, val) \
	!read_buf(my_fd, &val, sizeof(val), cmd, &i, cmdcount)
				case XF_CMD_DATA:
					debug("got new data");
				break;
				case XF_CMD_PING:
					intflag = FALSE;
					/* Expecting ping-pong only while playing! Otherwise, the writer
					   could get stuck waiting for free space forever. */
					if(ao->state == play_live)
						xfermem_putcmd(my_fd, XF_CMD_PONG);
					else
					{
						xfermem_putcmd(my_fd, XF_CMD_ERROR);
						if(ao->errcode == OUT123_OK)
							ao->errcode = OUT123_NOT_LIVE;
						if(!GOOD_WRITEVAL(my_fd, ao->errcode))
							return 2;
					}
				break;
				case BUF_CMD_PARAM:
					intflag = FALSE;
					/* If that does not work, communication is broken anyway and
					   writer will notice soon enough. */
					read_parameters(ao, my_fd, cmd, &i, cmdcount);
					xfermem_putcmd(my_fd, XF_CMD_OK);
				break;
				case BUF_CMD_OPEN:
				{
					char *driver  = NULL;
					char *device  = NULL;
					int success;

					intflag = FALSE;
					success = (
						!read_record( ao, XF_READER, (void**)&driver
						,	cmd, &i, cmdcount, NULL )
					&&	!read_record( ao, XF_READER, (void**)&device
						,	cmd, &i, cmdcount, NULL )
					&&	!out123_open(ao, driver, device)
					);
					free(device);
					free(driver);
					if(success)
					{
						xfermem_putcmd(my_fd, XF_CMD_OK);
						if(  write_string(ao, XF_READER, ao->driver)
						  || write_string(ao, XF_READER, ao->device) )
							return 2;
					}
					else
					{
						xfermem_putcmd(my_fd, XF_CMD_ERROR);
						/* Again, no sense to bitch around about communication errors,
						   just quit. */
						if(!GOOD_WRITEVAL(my_fd, ao->errcode))
							return 2;
					}
				}
				break;
				case BUF_CMD_CLOSE:
					intflag = FALSE;
					out123_close(ao);
					xfermem_putcmd(my_fd, XF_CMD_OK);
				break;
				case BUF_CMD_AUDIOCAP:
				{
					int encodings;

					intflag = FALSE;
					if(
						!GOOD_READVAL_BUF(my_fd, ao->channels)
					||	!GOOD_READVAL_BUF(my_fd, ao->rate)
					)
						return 2;
					encodings = out123_encodings(ao, ao->rate, ao->channels);
					if(encodings >= 0)
					{
						xfermem_putcmd(my_fd, XF_CMD_OK);
						if(!GOOD_WRITEVAL(my_fd, encodings))
							return 2;
					}
					else
					{
						xfermem_putcmd(my_fd, XF_CMD_ERROR);
						if(!GOOD_WRITEVAL(my_fd, ao->errcode))
							return 2;
					}
				}
				break;
				case BUF_CMD_AUDIOFMT:
				{
					size_t ratesize;
					long *rates = NULL;
					int minchannels;
					int maxchannels;
					struct mpg123_fmt *fmtlist;
					int fmtcount = -1;

					if(
						!GOOD_READVAL_BUF(my_fd, maxchannels)
					||	!GOOD_READVAL_BUF(my_fd, minchannels)
					)
						return 2;
					if(
						read_record( ao, XF_READER, (void**)&rates
						,	cmd, &i, cmdcount, &ratesize )
					){
						xfermem_putcmd(my_fd, XF_CMD_ERROR);
						if(!GOOD_WRITEVAL(my_fd, ao->errcode))
							return 2;
					}
					fmtcount = out123_formats( ao, rates
					,	(int)(ratesize/sizeof(*rates))
					,	minchannels, maxchannels, &fmtlist );
					free(rates);
					if(fmtcount >= 0)
					{
						int success;
						xfermem_putcmd(my_fd, XF_CMD_OK);
						success = GOOD_WRITEBUF( my_fd
						,	fmtlist, sizeof(*fmtlist)*fmtcount );
						free(fmtlist);
						if(!success)
							return 2;
					} else
					{
						xfermem_putcmd(my_fd, XF_CMD_ERROR);
						if(!GOOD_WRITEVAL(my_fd, ao->errcode))
							return 2;
					}
				}
				break;
				case BUF_CMD_START:
					intflag = FALSE;
					if(
						!GOOD_READVAL_BUF(my_fd, ao->format)
					||	!GOOD_READVAL_BUF(my_fd, ao->channels)
					||	!GOOD_READVAL_BUF(my_fd, ao->rate)
					)
						return 2;
					if(!out123_start(ao, ao->rate, ao->channels, ao->format))
					{
						preloading = TRUE;
						xfermem_putcmd(my_fd, XF_CMD_OK);
					}
					else
					{
						xfermem_putcmd(my_fd, XF_CMD_ERROR);
						if(!GOOD_WRITEVAL(my_fd, ao->errcode))
							return 2;
					}
				break;
				case BUF_CMD_STOP:
					intflag = FALSE;
					if(ao->state == play_live)
					{ /* Drain is implied! */
						size_t bytes;
						while((bytes = xfermem_get_usedspace(xf)))
							buffer_play(ao, bytes);
					}
					out123_stop(ao);
					xfermem_putcmd(my_fd, XF_CMD_OK);
				break;
				case XF_CMD_CONTINUE:
					intflag = FALSE;
					out123_continue(ao);
					preloading = TRUE;
					xfermem_putcmd(my_fd, XF_CMD_OK);
				break;
				case XF_CMD_IGNLOW:
					intflag = FALSE;
					preloading = FALSE;
					xfermem_putcmd(my_fd, XF_CMD_OK);
				break;
				case XF_CMD_DRAIN:
					intflag = FALSE;
					if(ao->state == play_live)
					{
						size_t bytes;
						while(
							(bytes = xfermem_get_usedspace(xf))
						&&	bytes > ao->framesize
						)
							buffer_play(ao, bytes);
						out123_drain(ao);
					}
					xfermem_putcmd(my_fd, XF_CMD_OK);
				break;
				case BUF_CMD_NDRAIN:
				{
					size_t limit;
					size_t oldfill;

					intflag = FALSE;
					if(
						!GOOD_READVAL_BUF(my_fd, limit)
					||	!GOOD_READVAL_BUF(my_fd, oldfill)
					)
						return 2;
					if(ao->state == play_live)
					{
						size_t bytes;
						while(
							(bytes = xfermem_get_usedspace(xf))
						&&	bytes > ao->framesize
						&&	oldfill >= bytes /* paranoia, overflow would handle it anyway */
						&&	(oldfill-bytes) < limit
						)
							buffer_play(ao, bytes > limit ? limit : bytes);
						out123_drain(ao);
						debug2( "buffer drained %"SIZE_P" / %"SIZE_P
						,	oldfill-bytes, limit );
					}
					xfermem_putcmd(my_fd, XF_CMD_OK);
				}
				break;
				case XF_CMD_TERMINATE:
					intflag = FALSE;
					/* Will that response always reach the writer? Well, at worst,
					   it's an ignored error on xfermem_getcmd(). */
					xfermem_putcmd(my_fd, XF_CMD_OK);
					return 0;
				case XF_CMD_PAUSE:
					intflag = FALSE;
					out123_pause(ao);
					xfermem_putcmd(my_fd, XF_CMD_OK);
				break;
				case XF_CMD_DROP:
					intflag = FALSE;
					xf->readindex = xf->freeindex;
					out123_drop(ao);
					xfermem_putcmd(my_fd, XF_CMD_OK);
				break;
				default:
					if(!AOQUIET)
						error1("Unknown command %u encountered. Confused Suicide!", cmd[i]);
					return 1;
#undef GOOD_READVAL_BUF
			}
		} /* Ensure that an interrupt-giving command has been received. */
		while(intflag);
		if(intflag && !AOQUIET)
			error("buffer: The intflag should not be set anymore.");
		intflag = FALSE; /* Any possible harm by _not_ ensuring that the flag is cleared here? */
	}
}
