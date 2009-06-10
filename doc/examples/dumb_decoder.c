/*
	A simple and dumb decoder that does not even query the mpg123 output format.
	It shall get MPG123_NEW_FORMAT once, but after that data!

	Beware: It writes raw audio to standard out!
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <mpg123.h>

void decode_track(mpg123_handle *mh);

int main(int argc, char **argv)
{
	int i;
	int err;
	mpg123_handle* mh;
	mpg123_init();
	mh = mpg123_new(NULL, NULL);
	for(i=1; i<argc; ++i)
	{
		fprintf(stderr, "Working on %s\n", argv[i]);

		err = mpg123_open(mh, argv[i]);
		if(err == MPG123_OK)
		{
			decode_track(mh);
			mpg123_close(mh);
		}
		else fprintf(stderr, "Failed to open file: %s\n", mpg123_strerror(mh));
	}
	mpg123_exit();
}

void decode_track(mpg123_handle *mh)
{
	unsigned char buf[16*1024];
	size_t fill;
	off_t sum = 0;
	int ret;
	while( (ret = mpg123_read(mh, buf, sizeof(buf), &fill)) != MPG123_DONE)
	{
		if(fill == MPG123_ERR)
		{
			fprintf(stderr, "error decoding: %s\n", mpg123_strerror(mh));
			return;
		}
		fprintf(stderr, "read returned: %i\n", ret);
		if(fill > 0)
		{
			sum += fill;
			write(STDOUT_FILENO, buf, fill);
		}
	}
	fprintf(stderr, "decoded bytes: %lli\n", (long long)sum);
}
