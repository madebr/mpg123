#include <stdio.h>

unsigned int getcpuid();
unsigned int getextcpuflags();
unsigned int getstdcpuflags();
unsigned int getstd2cpuflags();
/* standard level flags part 1 */
#define FLAG_SSE3      0x00000001
/* standard level flags part 2 */
#define FLAG2_MMX       0x00800000
#define FLAG2_SSE       0x02000000
#define FLAG2_SSE2      0x04000000
#define FLAG2_FPU       0x00000001
/* cpuid extended level 1 (AMD) */
#define XFLAG_MMX      0x00800000
#define XFLAG_3DNOW    0x80000000
#define XFLAG_3DNOWEXT 0x40000000

int main()
{
	int family = getcpuid();
	if(!family){ printf("CPU won't do cpuid (some old i386 or i486)\n"); return 0; }
	family = (family & 0xf00)>>8;
	printf("family: %i\n", family);
	printf("stdcpuflags:  0x%08x\n", getstdcpuflags());
	printf("std2cpuflags: 0x%08x\n", getstd2cpuflags());
	printf("extcpuflags:  0x%08x\n", getextcpuflags());
	if(family > 4 || family == 0)
	{
		unsigned int stdflags = 0;
		unsigned int std2flags = 0;
		unsigned int extflags = 0;
		stdflags = getstdcpuflags();
		std2flags = getstd2cpuflags();
		extflags = getextcpuflags();
		printf("A i586 or better cpu with:");
		if(std2flags & FLAG2_MMX) printf(" mmx");
		if(extflags & XFLAG_3DNOW) printf(" 3dnow");
		if(extflags & XFLAG_3DNOWEXT) printf(" 3dnowext");
		if(std2flags & FLAG2_SSE) printf(" sse");
		if(std2flags & FLAG2_SSE2) printf(" sse2");
		if(stdflags & FLAG_SSE3) printf(" sse3");
		printf("\n");
	}
	else printf("I guess you have some i486\n");
	return 0;
}
