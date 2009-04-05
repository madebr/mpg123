#include <unistd.h>
#include <stdio.h>

int main()
{
	const size_t bufs = 1024;
	size_t got;
	float f[bufs];
	double d[bufs];
	while( (got = fread(f, sizeof(float), bufs, stdin)) )
	{
		size_t fi;
		for(fi=0; fi<got; ++fi)
		d[fi] = (double) f[fi];

		write(STDOUT_FILENO, d, sizeof(double)*got);
	}
	return 0;
}
