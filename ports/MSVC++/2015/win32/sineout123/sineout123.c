#include <out123.h>
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>
#include "waves.h"

int _tmain(int argc, TCHAR **argv)
{
	struct wave_table *wv = NULL;
	double frequency = 440.0;
	out123_handle *ao = NULL;
	char *driver = NULL;
	char *device = NULL;
	long rate = 44100;
	int channels = 2;
	int encoding = MPG123_ENC_SIGNED_16;
	const char *encname = NULL;
	int framesize = 0;
	size_t buffer_size = 0;
	unsigned char* buffer = NULL;
	size_t done = 0;
	size_t played = 0;

	ao = out123_new();
	if (!ao)
	{
		fprintf(stderr, "Cannot create output handle.\n");
		out123_del(ao);
		return -1;
	}

	if (out123_open(ao, NULL, NULL) != OUT123_OK)
	{
		fprintf(stderr, "Trouble with out123: %s\n", out123_strerror(ao));
		out123_del(ao);
		return -1;
	}

	out123_driver_info(ao, &driver, &device);
	printf("Effective output driver: %s\n", driver ? driver : "<nil> (default)");
	printf("Effective output file:   %s\n", device ? device : "<nil> (default)");

	encname = out123_enc_name(encoding);
	printf("Playing with %i channels and %li Hz, encoding %s.\n"
		, channels, rate, encname ? encname : "???");

	if (out123_start(ao, rate, channels, encoding)
		|| out123_getformat(ao, NULL, NULL, NULL, &framesize))
	{
		fprintf(stderr, "Cannot start output / get framesize: %s\n"
			, out123_strerror(ao));
		out123_del(ao);
		return -1;
	}

	buffer_size = rate * framesize;
	buffer = malloc(buffer_size);

	wv = wave_table_new(rate, channels, encoding,
		1, &frequency, NULL, NULL, rate);

	int length_in_seconds = 5;

	while(length_in_seconds--)
	{
		done = wave_table_extract(wv, buffer, rate);

		played = out123_play(ao, buffer, done * framesize);
		if (played != buffer_size)
		{
			fprintf(stderr
				, "Warning: written less than gotten from sineout123: %li != %li\n"
				, (long)played, (long)buffer_size);
		}
	}

	wave_table_del(wv);
	free(buffer);

	return 0;
}
