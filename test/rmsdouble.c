#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
	int fda, fdb;
	double iso_rms_limit, iso_diff_limit;
	double av, bv, rms, maxdiff;
	long count;

	fprintf(stderr,"Computing RMS full scale for double float data (thus, full scale is 2).\n");
	/* Reference values relative to full scale 2 (from -1 to +1). They are _defined_ relative to full scale. */
	iso_rms_limit  = pow(2.,-15)/sqrt(12.);
	iso_diff_limit = pow(2.,-14);
	fprintf(stderr, "ISO limit values: RMS=%g maxdiff=%g\n", iso_rms_limit, iso_diff_limit);

	rms = 0;
	maxdiff = 0;
	count = 0;

	fda = open(argv[1], O_RDONLY);
	fdb = open(argv[2], O_RDONLY);
	if(fda < 0 || fdb < 0){ fprintf(stderr, "cannot open files\n");return 1; }

	while( (read(fda, &av, sizeof(double)) == sizeof(double))
	   &&  (read(fdb, &bv, sizeof(double)) == sizeof(double)) )
	{
		double vd = (double)av-(double)bv;
		++count;
		rms += vd*vd;
		if(vd < 0) vd *= -1;
		if(vd > maxdiff) maxdiff = vd;
	}

	close(fda);
	close(fdb);

	rms /= count;
	rms  = sqrt(rms);
	rms /= 2.; /* full scale... */
	maxdiff /= 2.; /* full scale again */
	printf("RMS=%g (%s) maxdiff=%g (%s)\n",
		rms, rms<iso_rms_limit ? "PASS" : "FAIL",
		maxdiff, maxdiff<iso_diff_limit ? "PASS" : "FAIL");
	return 0;
}
