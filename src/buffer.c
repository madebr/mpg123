/*
	buffer.c: output buffer

	copyright 1997-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Oliver Fromme

	I (ThOr) am reviewing this file at about the same daytime as Oliver's timestamp here:
	Mon Apr 14 03:53:18 MET DST 1997
	- dammed night coders;-)
*/

#include <stdlib.h>
#include <errno.h>

#include "config.h"
#include "mpg123.h"

int outburst = MAXOUTBURST;

static int intflag = FALSE;
static int usr1flag = FALSE;

static void catch_interrupt (void)
{
	intflag = TRUE;
}

static void catch_usr1 (void)
{
	usr1flag = TRUE;
}

/* Interfaces to writer process */

extern void buffer_sig(int signal, int block);

void buffer_ignore_lowmem(void)
{
#ifndef NOXFERMEM
	if (!buffermem)
		return;
	if(buffermem->wakeme[XF_READER])
		xfermem_putcmd(buffermem->fd[XF_WRITER], XF_CMD_WAKEUP);
#endif
}

void buffer_end(void)
{
#ifndef NOXFERMEM
	if (!buffermem)
		return;
	xfermem_putcmd(buffermem->fd[XF_WRITER], XF_CMD_TERMINATE);
#endif
}

void buffer_resync(void)
{
	buffer_sig(SIGINT, TRUE);
}

void buffer_reset(void)
{
	buffer_sig(SIGUSR1, TRUE);
}

void buffer_start(void)
{
	buffer_sig(SIGCONT, FALSE);
}

void buffer_stop(void)
{
	buffer_sig(SIGSTOP, FALSE);
}

extern int buffer_pid;

void buffer_sig(int signal, int block)
{
	
#ifndef NOXFERMEM
	if (!buffermem)
		return;

	kill(buffer_pid, signal);
	
	if (!block)
		return;

	if(xfermem_block(XF_WRITER, buffermem) != XF_CMD_WAKEUP) 
		perror("Could not resync/reset buffers");
#endif
	
	return;
}

#ifndef NOXFERMEM

void buffer_loop(struct audio_info_struct *ai, sigset_t *oldsigset)
{
	int bytes;
	int my_fd = buffermem->fd[XF_READER];
	txfermem *xf = buffermem;
	int done = FALSE;
	int preload;

	catchsignal (SIGINT, catch_interrupt);
	catchsignal (SIGUSR1, catch_usr1);
	sigprocmask (SIG_SETMASK, oldsigset, NULL);
	if (param.outmode == DECODE_AUDIO) {
		if (audio_open(ai) < 0) {
			perror("audio");
			exit(1);
		}
	}

	/* Fill complete buffer on first run before starting to play.
	 * Live mp3 streams constantly approach buffer underrun otherwise. [dk]
	 */
	preload = xf->size;

	for (;;) {
		if (intflag) {
			intflag = FALSE;
			if (param.outmode == DECODE_AUDIO)
				audio_queueflush (ai);
			xf->readindex = xf->freeindex;
			if (xf->wakeme[XF_WRITER])
				xfermem_putcmd(my_fd, XF_CMD_WAKEUP);
		}
		if (usr1flag) {
			usr1flag = FALSE;
			/*   close and re-open in order to flush
			 *   the device's internal buffer before
			 *   changing the sample rate.   [OF]
			 */
			/* writer must block when sending SIGUSR1
			 * or we will lose all data processed 
			 * in the meantime! [dk]
			 */
			xf->readindex = xf->freeindex;
			/* We've nailed down the new starting location -
			 * writer is now safe to go on. [dk]
			 */
			if (xf->wakeme[XF_WRITER])
				xfermem_putcmd(my_fd, XF_CMD_WAKEUP);
			if (param.outmode == DECODE_AUDIO) {
				audio_close (ai);
				ai->rate = xf->buf[0]; 
				ai->channels = xf->buf[1]; 
				ai->format = xf->buf[2];
				if (audio_open(ai) < 0) {
					perror("audio");
					exit(1);
				}
			}
		}
		if ( (bytes = xfermem_get_usedspace(xf)) < outburst ) {
			/* if we got a buffer underrun we first
			 * fill 1/8 of the buffer before continue/start
			 * playing */
			if (preload < xf->size>>3)
				preload = xf->size>>3;
			if(preload < outburst)
				preload = outburst;
		}
		if(bytes < preload) {
			int cmd;
			if (done && !bytes) { 
				break;
			}
			
			if(!done) {

				/* Don't spill into errno check below. */
				errno = 0;

				cmd = xfermem_block(XF_READER, xf);

				switch(cmd) {

					/* More input pending. */
					case XF_CMD_WAKEUP_INFO:
						continue;
					/* Yes, we know buffer is low but
					 * know we don't care.
					 */
					case XF_CMD_WAKEUP:
						break;	/* Proceed playing. */
					case XF_CMD_TERMINATE:
						/* Proceed playing without 
						 * blocking any further.
						 */
						done=TRUE;
						break;
					case -1:
						if(errno==EINTR)
							continue;
						if(errno)
							perror("Yuck! Error in buffer handling...");
						done = TRUE;
						xf->readindex = xf->freeindex;
						xfermem_putcmd(xf->fd[XF_READER], XF_CMD_TERMINATE);
						break;
					default:
						fprintf(stderr, "\nEh!? Received unknown command 0x%x in buffer process. Tell Daniel!\n", cmd);
				}
			}
		}
		/* Hack! The writer issues XF_CMD_WAKEUP when first adjust 
		 * audio settings. We do not want to lower the preload mark
		 * just yet!
		 */
		if (!bytes)
			continue;
		preload = outburst; /* set preload to lower mark */
		if (bytes > xf->size - xf->readindex)
			bytes = xf->size - xf->readindex;
		if (bytes > outburst)
			bytes = outburst;

		if (param.outmode == DECODE_FILE)
			bytes = write(OutputDescriptor, xf->data + xf->readindex, bytes);
		else if (param.outmode == DECODE_AUDIO)
			bytes = audio_play_samples(ai,
				(unsigned char *) (xf->data + xf->readindex), bytes);

		if(bytes < 0) {
			bytes = 0;
			if(errno != EINTR) {
				perror("Ouch ... error while writing audio data: ");
				/*
				 * done==TRUE tells writer process to stop
				 * sending data. There might be some latency
				 * involved when resetting readindex to 
				 * freeindex so we might need more than one
				 * cycle to terminate. (The number of cycles
				 * should be finite unless I managed to mess
				 * up something. ;-) [dk]
				 */
				done = TRUE;	
				xf->readindex = xf->freeindex;
				xfermem_putcmd(xf->fd[XF_READER], XF_CMD_TERMINATE);
			}
		}

		xf->readindex = (xf->readindex + bytes) % xf->size;
		if (xf->wakeme[XF_WRITER])
			xfermem_putcmd(my_fd, XF_CMD_WAKEUP);
	}

	if (param.outmode == DECODE_AUDIO)
		audio_close (ai);
}

#endif

/* EOF */
