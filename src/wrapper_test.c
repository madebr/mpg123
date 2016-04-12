#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>


void play_with_decoder(FILE* control, FILE* status);
double timediff(struct timeval *past, struct timeval *present);

int main()
{

int dec_stdin[2], dec_err[2];
int pid;
int retval = 0;
struct timeval ta, tb;

gettimeofday(&ta, NULL);

pipe(dec_stdin);
pipe(dec_err);

pid=fork();
if (pid == 0)
{
	/* decoder process */
	close(dec_stdin[1]);
	close(dec_err[0]);

	dup2(dec_stdin[0], STDIN_FILENO);
	dup2(dec_err[1], STDERR_FILENO);

	execlp("mpg123", "-q", "-s", "-R", "--remote-err", NULL);
	exit(1); /* error here */
}

/* back again at main process */
if(waitpid(pid, &retval, WNOHANG) == 0 )
{
	FILE *status;
	FILE *control;
	close(dec_stdin[0]);
	close(dec_err[1]);

	/* not sure if that's neded here */
	fcntl(dec_stdin[1], F_SETFD, FD_CLOEXEC); //close on exec (after forking)
	fcntl(dec_err[0], F_SETFD, FD_CLOEXEC); //close on exec (after forking)

	status  = compat_fdopen(dec_err[0],"r"); /* the responses or errors */
	control = compat_fdopen(dec_stdin[1],"a"); /* here we can dump orders */

	/* Read and write lines of commands ... a bit of buffering makes sense for that. */
	setlinebuf(control);
	setlinebuf(status);

	play_with_decoder(control, status);

	fprintf(control, "quit\n");
	/* wait for end */
	waitpid(pid, &retval, 0);

	/* Now play ping-pong with mpg123, either in one process/thread via nonblocking read (select()) on status and the OSC input, interchanged with writes to control, or with a set of threads that block on one I/O path each. */
	/* play_with_decoder_and_osc(control, status); */
}

gettimeofday(&tb, NULL);
printf("Needed %f ms for the whole deal\n", timediff(&ta, &tb));
return 0;

}

/* time difference in milliseconds */
double timediff(struct timeval *past, struct timeval *present)
{
	double ms;
	struct timeval diff;
	timersub(present, past, &diff);
	ms = (double)diff.tv_sec*1000 + (double)diff.tv_usec/1000;
	return ms;
}

void play_with_decoder(FILE* control, FILE* status)
{
	char line[4096]; /* lazy fixed line length, just for testing! */
	struct timeval ta, tb, tc;
	int count = 100;
	int i = 0;
	double sum = 0.;

	if((fgets(line, sizeof(line)/sizeof(char), status)) == NULL) return;
	fprintf(stderr, "Got greeting: %s", line);

	while(++i <= count)
	{
		double ms;
		/* use a simple command with not much effect to test communication latency */
		gettimeofday(&ta, NULL);
		fprintf(control, "silence\n");
		if(fgets(line, sizeof(line)/sizeof(char), status) == NULL) return;
		gettimeofday(&tb, NULL);
		ms = timediff(&ta, &tb);
		sum += ms;
		fprintf(stderr, "sent silcence, got %s", line);
		fprintf(stderr, "ping-pong: %f ms \n", ms);
	}
	fprintf(stderr, "mean send/receive latency: %f ms\n", sum/count);
}
