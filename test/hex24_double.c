/* Convert ASCII/Hex 24bit numbers to native signed 32bit integers (pushing the 24bits into the 3 most significant bytes). */

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

int main()
{
	union { uint32_t u; int32_t s; } num;
	while(scanf("%x", &num.u) == 1)
	{
		num.u <<=8;
		double dnum = (double)num.s/((double)2147483647+1);
		write(STDOUT_FILENO, &dnum, sizeof(double));
	}
	return 0;
}
