#include <unistd.h>
#include <stdio.h>

int main()
{
	const size_t bufs = 1024;
	size_t got;
	short s[bufs];
	double d[bufs];
	while( (got = fread(s, sizeof(short), bufs, stdin)) )
	{
		size_t fi;
		for(fi=0; fi<got; ++fi)
		d[fi] = (double) s[fi] / 32768.; /* funny: 32767 makes mpg123's s16 compare better with reference */

		write(STDOUT_FILENO, d, sizeof(double)*got);
	}
	return 0;
}
