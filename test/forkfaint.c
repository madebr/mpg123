/*
	A program that forks mpg123 to read it's output, then faints to release a zombie.
	What does mpg123 do?
		- pre 1.0rc2:   Decode wildly until input end reached, writing into the nonexistent pipe.
		- since 1.0rc2: Ending because of I/O error.

I guess that is what is at the root of the asterisk problem in debug mode.
*/

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

int main(int argc, char **argv)
{
	pid_t pid;
	int strm[2];
	int retval;
	if(argc < 2) return -2;

	/* Normally, the decoder gets killed on SIGPIPE when wanting to write to STDOUT,
	   but with this in place, it gets normal I/O error. */
	signal(SIGPIPE, SIG_IGN);

	pipe(strm);
	pid = fork();
	if(pid == 0)
	{
		close(strm[0]);
		dup2(strm[1],STDOUT_FILENO);
		fprintf(stderr, "starting decoder\n");
		execlp("mpg123", "mpg123", "-s", "-q", argv[1], NULL);
		fprintf(stderr, "Still here? That's bad.(%s)\n", strerror(errno));

		return -1;
	}
	printf("Now fainting\n");
	return 0;
}
