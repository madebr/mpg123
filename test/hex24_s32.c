/* Convert ASCII/Hex 24bit numbers to native signed 32bit integers (pushing the 24bits into the 3 most significant bytes). */

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

int main()
{
	union { uint32_t u; int32_t s; } num;
	while(scanf("%x", &num.u) == 1)
	{
		num.u <<=8; /* It's been 24 lower bits, now 24higher bits. Actually signed, but that doesn't matter here. */
		write(STDOUT_FILENO, &num.u, 4);
		/* fprintf(stderr, "%li\n", (long)(num.s/(1<<8))); */
	}
	return 0;
}
