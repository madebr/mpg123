#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
	int a = open(argv[1], O_RDONLY);
	int b = open(argv[2], O_RDONLY);
	if(a < 0 || b < 0){ fprintf(stderr, "cannot open files\n");return 1; }
	double av;
	float bv;
	double rms = 0;
	long count = 0;
	while( (read(a, &av, sizeof(double)) == sizeof(double))
	   &&  (read(b, &bv, sizeof(float)) == sizeof(float)) )
	{
		++count;
		rms += pow((double)av-bv,2);
	}
	rms /= count;
	printf("RMS=%g\n", sqrt(rms));
	close(a);
	close(b);
	return 0;
}
