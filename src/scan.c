#include "mpg123.h"

int main(int argc, char **argv)
{
	mpg123_handle *m;
	int i;
	mpg123_init();
	m = mpg123_new(NULL, NULL);
	for(i = 1; i < argc; ++i)
	{
		off_t a, b;
		mpg123_open(m, argv[i]);
		a = mpg123_length(m);
		mpg123_scan(m);
		b = mpg123_length(m);
		printf("File %i: estimated %li vs. scanned %li\n", i, (long)a, (long)b);
	}
	mpg123_delete(m);
	mpg123_exit();
	return 0;
}
