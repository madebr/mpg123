/*
	optimize: get a grip on the different optimizations

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, inspired by 3DNow stuff in mpg123.[hc]

	Currently, this file contains the struct and function to choose an optimization variant and works only when OPT_MULTI is in effect.
*/

#include "config.h"
#include "mpg123.h" /* includes optimize.h */
#include "debug.h"

#ifdef OPT_MULTI
struct_opts cpu_opts;

#include "getcpuflags.h"

void list_cpu_opt()
{
	printf("CPU options:");
	#ifdef OPT_SSE
	printf(" SSE");
	#endif
	#ifdef OPT_3DNOWEXT
	printf(" 3DNowExt");
	#endif
	#ifdef OPT_3DNOW
	printf(" 3DNow");
	#endif
	#ifdef OPT_MMX
	printf(" MMX");
	#endif
	#ifdef OPT_I586
	printf(" i586");
	#endif
	#ifdef OPT_I586_DITHER
	printf(" i586_dither");
	#endif
	#ifdef OPT_I486
	printf(" i486");
	#endif
	#ifdef OPT_I386
	printf(" i386");
	#endif
	#ifdef OPT_ALTIVEC
	printf(" AltiVec");
	#endif
	#ifdef OPT_GENERIC
	printf(" generic");
	#endif
	printf("\n");
}

void test_cpu_flags()
{
	#ifdef OPT_X86
	struct cpuflags cf;
	getcpuflags(&cf);
	if(cpu_i586(cf))
	{
		printf("Supported decoders:");
		/* not yet: if(cpu_sse2(cf)) printf(" SSE2");
		if(cpu_sse3(cf)) printf(" SSE3"); */
#ifdef OPT_SSE
		if(cpu_sse(cf)) printf(" SSE");
#endif
#ifdef OPT_3DNOWEXT
		if(cpu_3dnowext(cf)) printf(" 3DNowExt");
#endif
#ifdef OPT_3DNOW
		if(cpu_3dnow(cf)) printf(" 3DNow");
#endif
#ifdef OPT_MMX
		if(cpu_mmx(cf)) printf(" MMX");
#endif
#ifdef OPT_I586
		printf(" i586");
#endif
#ifdef OPT_I586_DITHER
		printf(" i586_dither");
#endif
#ifdef OPT_I386
		printf(" i386");
#endif
#ifdef OPT_GENERIC
		printf(" generic");
#endif
		printf("\n");
	}
	else
	{
		printf("You have an i386 or i486... or perhaps a non-x86-32bit CPU?\n");
	}
	#else
	printf("I only know x86 cpus...\n");
	#endif
}

int set_cpu_opt()
{
	#ifdef OPT_X86
	struct cpuflags cf;
	#endif
	char* chosen = ""; /* the chosed decoder opt as string */
	int auto_choose = 0;
	int done = 0;
	if(   (param.cpu == NULL)
	   || (param.cpu[0] == 0)
	   || !strcasecmp(param.cpu, "auto") )
	auto_choose = 1;

	/* covers any i386+ cpu; they actually differ only in the synth_1to1 function... */
	#ifdef OPT_X86

	#ifdef OPT_MMXORSSE
	cpu_opts.decwin = decwin;
	cpu_opts.make_decode_tables   = make_decode_tables;
	cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2;
	cpu_opts.init_layer2_table    = init_layer2_table;
	#endif
	#ifdef OPT_3DNOW
	cpu_opts.dct36 = dct36;
	#endif
	#ifdef OPT_3DNOWEXT
	cpu_opts.dct36 = dct36;
	#endif

	if(getcpuflags(&cf) && cpu_i586(cf))
	{
		debug2("standard flags: 0x%08x\textended flags: 0x%08x\n", cf.std, cf.ext);
		#ifdef OPT_3DNOWEXT
		if(   !done && (auto_choose || !strcasecmp(param.cpu, "3dnowext"))
		   && cpu_3dnow(cf)
		   && cpu_3dnowext(cf)
		   && cpu_mmx(cf) )
		{
			int go = 1;
			if(param.force_rate)
			{
				#if defined(K6_FALLBACK) || defined(PENTIUM_FALLBACK)
				if(!auto_choose) error("I refuse to choose 3DNowExt as this will screw up with forced rate!");
				else if(param.verbose) fprintf(stderr, "Note: Not choosing 3DNowExt because flexible rate not supported.\n");

				go = 0;
				#else
				error("You will hear some awful sound because of flexible rate being chosen with SSE decoder!");
				#endif
			}
			if(go){ /* temporary hack for flexible rate bug, not going indent this - fix it instead! */
			chosen = "3DNowExt";
			cpu_opts.dct36 = dct36_3dnowext;
			cpu_opts.synth_1to1 = synth_1to1_sse;
			cpu_opts.dct64 = dct64_mmx; /* only use the sse version in the synth_1to1_sse */
			cpu_opts.decwin = decwin_mmx;
			cpu_opts.make_decode_tables   = make_decode_tables_mmx;
			cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2_mmx;
			cpu_opts.init_layer2_table    = init_layer2_table_mmx;
			cpu_opts.mpl_dct64 = dct64_3dnowext;
			done = 1;
			}
		}
		#endif
		#ifdef OPT_SSE
		if(   !done && (auto_choose || !strcasecmp(param.cpu, "sse"))
		   && cpu_sse(cf) && cpu_mmx(cf) )
		{
			int go = 1;
			if(param.force_rate)
			{
				#ifdef PENTIUM_FALLBACK
				if(!auto_choose) error("I refuse to choose SSE as this will screw up with forced rate!");
				else if(param.verbose) fprintf(stderr, "Note: Not choosing SSE because flexible rate not supported.\n");

				go = 0;
				#else
				error("You will hear some awful sound because of flexible rate being chosen with SSE decoder!");
				#endif
			}
			if(go){ /* temporary hack for flexible rate bug, not going indent this - fix it instead! */
			chosen = "SSE";
			cpu_opts.synth_1to1 = synth_1to1_sse;
			cpu_opts.dct64 = dct64_mmx; /* only use the sse version in the synth_1to1_sse */
			cpu_opts.decwin = decwin_mmx;
			cpu_opts.make_decode_tables   = make_decode_tables_mmx;
			cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2_mmx;
			cpu_opts.init_layer2_table    = init_layer2_table_mmx;
			cpu_opts.mpl_dct64 = dct64_sse;
			done = 1;
			}
		}
		#endif
		#ifdef OPT_3DNOW
		cpu_opts.dct36 = dct36;
		/* TODO: make autodetection for _all_ x86 optimizations (maybe just for i586+ and keep separate 486 build?) */
		/* check cpuflags bit 31 (3DNow!) and 23 (MMX) */
		if(   !done && (auto_choose || !strcasecmp(param.cpu, "3dnow"))
			 && (param.stat_3dnow < 2)
			 && ((param.stat_3dnow == 1) || (cpu_3dnow(cf) && cpu_mmx(cf))))
		{
			chosen = "3DNow";
			cpu_opts.dct36 = dct36_3dnow; /* 3DNow! optimized dct36() */
			cpu_opts.synth_1to1 = synth_1to1_3dnow;
			cpu_opts.dct64 = dct64_i386; /* use the 3dnow one? */
			done = 1;
		}
		#endif
		#ifdef OPT_MMX
		if(   !done && (auto_choose || !strcasecmp(param.cpu, "mmx"))
		   && cpu_mmx(cf) )
		{
			int go = 1;
			if(param.force_rate)
			{
				#ifdef PENTIUM_FALLBACK
				if(!auto_choose) error("I refuse to choose MMX as this will screw up with forced rate!");
				else if(param.verbose) fprintf(stderr, "Note: Not choosing MMX because flexible rate not supported.\n");

				go = 0;
				#else
				error("You will hear some awful sound because of flexible rate being chosen with MMX decoder!");
				#endif
			}
			if(go){ /* temporary hack for flexible rate bug, not going indent this - fix it instead! */
			chosen = "MMX";
			cpu_opts.synth_1to1 = synth_1to1_mmx;
			cpu_opts.dct64 = dct64_mmx;
			cpu_opts.decwin = decwin_mmx;
			cpu_opts.make_decode_tables   = make_decode_tables_mmx;
			cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2_mmx;
			cpu_opts.init_layer2_table    = init_layer2_table_mmx;
			done = 1;
			}
		}
		#endif
		#ifdef OPT_I586
		if(!done && (auto_choose || !strcasecmp(param.cpu, "i586")))
		{
			chosen = "i586/pentium";
			cpu_opts.synth_1to1 = synth_1to1_i586;
			cpu_opts.synth_1to1_i586_asm = synth_1to1_i586_asm;
			cpu_opts.dct64 = dct64_i386;
			done = 1;
		}
		#endif
		#ifdef OPT_I586_DITHER
		if(!done && (auto_choose || !strcasecmp(param.cpu, "i586_dither")))
		{
			chosen = "dithered i586/pentium";
			cpu_opts.synth_1to1 = synth_1to1_i586;
			cpu_opts.dct64 = dct64_i386;
			cpu_opts.synth_1to1_i586_asm = synth_1to1_i586_asm_dither;
			done = 1;
		}
		#endif
	}
	#ifdef OPT_I486 /* that won't cooperate nicely in multi opt mode - forcing i486 in layer3.c */
	if(!done && (auto_choose || !strcasecmp(param.cpu, "i486")))
	{
		chosen = "i486";
		cpu_opts.synth_1to1 = synth_1to1_i386; /* i486 function is special */
		cpu_opts.dct64 = dct64_i386;
		done = 1;
	}
	#endif
	#ifdef OPT_I386
	if(!done && (auto_choose || !strcasecmp(param.cpu, "i386")))
	{
		chosen = "i386";
		cpu_opts.synth_1to1 = synth_1to1_i386;
		cpu_opts.dct64 = dct64_i386;
		done = 1;
	}
	#endif

	if(done) /* set common x86 functions */
	{
		cpu_opts.synth_1to1_mono = synth_1to1_mono_i386;
		cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo_i386;
		cpu_opts.synth_1to1_8bit = synth_1to1_8bit_i386;
		cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono_i386;
		cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo_i386;
	}
	#endif /* OPT_X86 */

	#ifdef OPT_ALTIVEC
	if(!done && (auto_choose || !strcasecmp(param.cpu, "altivec")))
	{
		chosen = "AltiVec";
		cpu_opts.dct64 = dct64_altivec;
		cpu_opts.synth_1to1 = synth_1to1_altivec;
		cpu_opts.synth_1to1_mono = synth_1to1_mono_altivec;
		cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo_altivec;
		cpu_opts.synth_1to1_8bit = synth_1to1_8bit_altivec;
		cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono_altivec;
		cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo_altivec;
		done = 1;
	}
	#endif

	#ifdef OPT_GENERIC
	if(!done && (auto_choose || !strcasecmp(param.cpu, "generic")))
	{
		chosen = "generic";
		cpu_opts.dct64 = dct64;
		cpu_opts.synth_1to1 = synth_1to1;
		cpu_opts.synth_1to1_mono = synth_1to1_mono;
		cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo;
		cpu_opts.synth_1to1_8bit = synth_1to1_8bit;
		cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono;
		cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo;
		done = 1;
	}
	#endif

	if(done)
	{
		if(!param.remote && !param.quiet) fprintf(stderr, "decoder: %s\n", chosen);
		return 1;
	}
	else
	{
		error("Could not set optimization!");
		return 0;
	}
}
#endif
