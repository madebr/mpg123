/*
	optimize: get a grip on the different optimizations

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, inspired by 3DNow stuff in mpg123.[hc]

	Currently, this file contains the struct and function to choose an optimization variant and works only when OPT_MULTI is in effect.
*/

#include "mpg123lib_intern.h" /* includes optimize.h */
#include "debug.h"

/* Must match the enum dectype! */
const char* decname[] =
{
	"auto", "nodec", "generic", "i386",
	"i486", "i586", "i586_dither", "MMX",
	"3DNow", "3DNowExt", "AltiVec", "SSE"
};

#if (defined OPT_X86) && (defined OPT_MULTI)
#include "getcpuflags.h"
struct cpuflags cpu_flags;
#endif

enum optdec defdec(void){ return defopt; }

enum optcla decclass(const enum optdec type)
{
	return (type == mmx || type == sse || type == dreidnowext) ? mmxsse : normal;
}

/* Determine what kind of decoder is actually active
   This depends on runtime choices which may cause fallback to i386 or generic code. */
static int find_dectype(mpg123_handle *fr)
{
	enum optdec type = nodec;
	/* Direct and indirect usage, 1to1 stereo decoding.
	   Concentrating on the plain stereo synth should be fine, mono stuff is derived. */
	func_synth basic_synth = fr->synth;
	if(basic_synth == synth_1to1_8bit_wrap)
	basic_synth = opt_synth_1to1(fr); /* That is what's really below the surface. */

	if(FALSE) ; /* Just to initialize the else if ladder. */
#ifdef OPT_3DNOWEXT
	else if(basic_synth == synth_1to1_3dnowext) type = dreidnowext;
#endif
#ifdef OPT_SSE
	else if(basic_synth == synth_1to1_sse) type = sse;
#endif
#ifdef OPT_3DNOW
	else if(basic_synth == synth_1to1_3dnow) type = dreidnow;
#endif
#ifdef OPT_MMX
	else if(basic_synth == synth_1to1_mmx) type = mmx;
#endif
#ifdef OPT_I586_DITHER
	else if(basic_synth == synth_1to1_i586_dither) type = ifuenf_dither;
#endif
#ifdef OPT_I586
	else if(basic_synth == synth_1to1_i586) type = ifuenf;
#endif
#ifdef OPT_I386
	else if
	(
		   basic_synth == synth_1to1_i386
		|| basic_synth == synth_2to1_i386
		|| basic_synth == synth_4to1_i386
		|| basic_synth == synth_1to1_8bit_i386
		|| basic_synth == synth_2to1_8bit_i386
		|| basic_synth == synth_4to1_8bit_i386
#ifndef REAL_IS_FIXED
		|| basic_synth == synth_1to1_real_i386
		|| basic_synth == synth_2to1_real_i386
		|| basic_synth == synth_4to1_real_i386
#endif
	) type = idrei;
#endif
	else if
	(
		   basic_synth == synth_1to1
		|| basic_synth == synth_2to1
		|| basic_synth == synth_4to1
		|| basic_synth == synth_ntom
		|| basic_synth == synth_1to1_8bit
		|| basic_synth == synth_2to1_8bit
		|| basic_synth == synth_4to1_8bit
		|| basic_synth == synth_ntom_8bit
#ifndef REAL_IS_FIXED
		|| basic_synth == synth_1to1_real
		|| basic_synth == synth_2to1_real
		|| basic_synth == synth_4to1_real
		|| basic_synth == synth_ntom_real
#endif
	) type = generic;
#ifdef OPT_ALTIVEC
	else if(basic_synth == synth_1to1_altivec) type = altivec;
#endif
#ifdef OPT_I486
	/* i486 is special ... the specific code is in use for 16bit 1to1 stereo
	   otherwise we have i386 active... but still, the distinction doesn't matter*/
	type = ivier;
#endif

	if(type != nodec)
	{
		fr->cpu_opts.type = type;
		fr->cpu_opts.class = decclass(type);

		debug3("determined active decoder type %i (%s) of class %i", type, decname[type], fr->cpu_opts.class);
		return MPG123_OK;
	}
	else
	{
		if(NOQUIET) error("Unable to determine active decoder type -- this is SERIOUS b0rkage!");

		fr->err = MPG123_BAD_DECODER_SETUP;
		return MPG123_ERR;
	}
}

/* set synth functions for current frame, optimizations handled by opt_* macros */
int set_synth_functions(mpg123_handle *fr)
{
	int ds = fr->down_sample;
	int basic_format = OUT_16; /* Default is always 16bit. */
	/* The tables to select the synth functions from...
	   First we have stereo synths for different outputs and resampling modes,
	   then functions for mono2stereo and mono, again for different outputs and resampling modes. */
	func_synth      funcs[OUT_FORMATS][4];
	func_synth_mono funcs_mono[OUT_FORMATS][4];
	func_synth_mono funcs_mono2stereo[OUT_FORMATS][4];

	/* What a pyramid... but that's the host of synth function interfaces we cater.
	   TODO: In future, the synth slots in the frame struct should have the same array structure.
	   Actually... they shall _be_ _this_ struct. Will reduce quite some code. */
	funcs[OUT_16][0] = (func_synth) opt_synth_1to1(fr);
	funcs[OUT_16][1] = (func_synth) opt_synth_2to1(fr);
	funcs[OUT_16][2] = (func_synth) opt_synth_4to1(fr);
	funcs[OUT_16][3] = (func_synth) opt_synth_ntom(fr);
	funcs[OUT_8][0] = (func_synth) opt_synth_1to1_8bit(fr);
	funcs[OUT_8][1] = (func_synth) opt_synth_2to1_8bit(fr);
	funcs[OUT_8][2] = (func_synth) opt_synth_4to1_8bit(fr);
	funcs[OUT_8][3] = (func_synth) opt_synth_ntom_8bit(fr);
#ifndef REAL_IS_FIXED
	funcs[OUT_REAL][0] = (func_synth) opt_synth_1to1_real(fr);
	funcs[OUT_REAL][1] = (func_synth) opt_synth_2to1_real(fr);
	funcs[OUT_REAL][2] = (func_synth) opt_synth_4to1_real(fr);
	funcs[OUT_REAL][3] = (func_synth) opt_synth_ntom_real(fr);
#endif

	funcs_mono[OUT_16][0] = (func_synth_mono) opt_synth_1to1_mono(fr);
	funcs_mono[OUT_16][1] = (func_synth_mono) opt_synth_2to1_mono(fr);
	funcs_mono[OUT_16][2] = (func_synth_mono) opt_synth_4to1_mono(fr);
	funcs_mono[OUT_16][3] = (func_synth_mono) opt_synth_ntom_mono(fr);
	funcs_mono[OUT_8][0] = (func_synth_mono) opt_synth_1to1_8bit_mono(fr);
	funcs_mono[OUT_8][1] = (func_synth_mono) opt_synth_2to1_8bit_mono(fr);
	funcs_mono[OUT_8][2] = (func_synth_mono) opt_synth_4to1_8bit_mono(fr);
	funcs_mono[OUT_8][3] = (func_synth_mono) opt_synth_ntom_8bit_mono(fr);
#ifndef REAL_IS_FIXED
	funcs_mono[OUT_REAL][0] = (func_synth_mono) opt_synth_1to1_real_mono(fr);
	funcs_mono[OUT_REAL][1] = (func_synth_mono) opt_synth_2to1_real_mono(fr);
	funcs_mono[OUT_REAL][2] = (func_synth_mono) opt_synth_4to1_real_mono(fr);
	funcs_mono[OUT_REAL][3] = (func_synth_mono) opt_synth_ntom_real_mono(fr);
#endif

	funcs_mono2stereo[OUT_16][0] = (func_synth_mono) opt_synth_1to1_mono2stereo(fr);
	funcs_mono2stereo[OUT_16][1] = (func_synth_mono) opt_synth_2to1_mono2stereo(fr);
	funcs_mono2stereo[OUT_16][2] = (func_synth_mono) opt_synth_4to1_mono2stereo(fr);
	funcs_mono2stereo[OUT_16][3] = (func_synth_mono) opt_synth_ntom_mono2stereo(fr);
	funcs_mono2stereo[OUT_8][0] = (func_synth_mono) opt_synth_1to1_8bit_mono2stereo(fr);
	funcs_mono2stereo[OUT_8][1] = (func_synth_mono) opt_synth_2to1_8bit_mono2stereo(fr);
	funcs_mono2stereo[OUT_8][2] = (func_synth_mono) opt_synth_4to1_8bit_mono2stereo(fr);
	funcs_mono2stereo[OUT_8][3] = (func_synth_mono) opt_synth_ntom_8bit_mono2stereo(fr);
#ifndef REAL_IS_FIXED
	funcs_mono2stereo[OUT_REAL][0] = (func_synth_mono) opt_synth_1to1_real_mono2stereo(fr);
	funcs_mono2stereo[OUT_REAL][1] = (func_synth_mono) opt_synth_2to1_real_mono2stereo(fr);
	funcs_mono2stereo[OUT_REAL][2] = (func_synth_mono) opt_synth_4to1_real_mono2stereo(fr);
	funcs_mono2stereo[OUT_REAL][3] = (func_synth_mono) opt_synth_ntom_real_mono2stereo(fr);
#endif


	/* Select the basic output format, different from 16bit: 8bit, real. */
	if(fr->af.encoding & MPG123_ENC_8) basic_format = OUT_8;
	else if(fr->af.encoding & MPG123_ENC_FLOAT)
	{
#ifndef REAL_IS_FIXED
		basic_format = OUT_REAL;
#else
		if(NOQUIET) error("set_synth_functions: Invalid output format, shouldn't have reached me!");
		return -1;
#endif
	}

	/* Finally selecting the synth functions for stereo / mono. */
	fr->synth = funcs[basic_format][ds];
	fr->synth_mono = fr->af.channels==2
		? funcs_mono2stereo[basic_format][ds] /* Mono MPEG file decoded to stereo. */
		: funcs_mono[basic_format][ds];       /* Mono MPEG file decoded to mono. */

	if(find_dectype(fr) != MPG123_OK) /* Actually determine the currently active decoder breed. */
	{
		fr->err = MPG123_BAD_DECODER_SETUP;
		return MPG123_ERR;
	}

	if(frame_buffers(fr) != 0)
	{
		fr->err = MPG123_BAD_DECODER_SETUP;
		if(NOQUIET) error("Failed to set up decoder buffers!");

		return MPG123_ERR;
	}

	if(basic_format == OUT_8)
	{
		if(make_conv16to8_table(fr) != 0)
		{
			if(NOQUIET) error("Failed to set up conv16to8 table!");
			/* it's a bit more work to get proper error propagation up */
			return -1;
		}
	}

#ifdef OPT_MMXORSSE
	/* Special treatment for MMX, SSE and 3DNowExt stuff. */
	if(fr->cpu_opts.class == mmxsse)
	{
		init_layer3_stuff(fr, init_layer3_gainpow2_mmx);
		init_layer2_stuff(fr, init_layer2_table_mmx);
		fr->make_decode_tables = make_decode_tables_mmx;
	}
	else
#endif
	{
		init_layer3_stuff(fr, init_layer3_gainpow2);
		init_layer2_stuff(fr, init_layer2_table);
		fr->make_decode_tables = make_decode_tables;
	}

	return 0;
}

int frame_cpu_opt(mpg123_handle *fr, const char* cpu)
{
	const char* chosen = ""; /* the chosen decoder opt as string */
	enum optdec want_dec = nodec;
	int done = 0;
	int auto_choose = 0;
	want_dec = dectype(cpu);
	auto_choose = want_dec == autodec ? 1 : 0;
#ifndef OPT_MULTI
	{
		if(!auto_choose && want_dec != defopt)
		{
			if(NOQUIET) error2("you wanted decoder type %i, I only have %i", want_dec, defopt);

			done = 0;
		}
		else
		{
			const char **sd = mpg123_decoders(); /* this contains _one_ decoder */
			chosen = sd[0];
			done = 1;
		}
	}
#else
/*
	First the set of synth functions is nulled, so that we know what to fill in at the end.

	## This is an inline bourne shell script for execution in nedit to generate the lines below.
	## The ## is a quote for just #
	for t in "" _8bit _real
	do
		if test "$t" = _real; then
			echo "##ifndef REAL_IS_FIXED"
		fi
		for i in 1to1 2to1 4to1 ntom; do for f in "" _mono _mono2stereo;
		do
			echo "	fr->cpu_opts.synth_${i}${t}${f} = NULL;"
		done;done
		if test "$t" = _real; then
			echo "##endif"
		fi
	done
*/
	fr->cpu_opts.synth_1to1 = NULL;
	fr->cpu_opts.synth_1to1_mono = NULL;
	fr->cpu_opts.synth_1to1_mono2stereo = NULL;
	fr->cpu_opts.synth_2to1 = NULL;
	fr->cpu_opts.synth_2to1_mono = NULL;
	fr->cpu_opts.synth_2to1_mono2stereo = NULL;
	fr->cpu_opts.synth_4to1 = NULL;
	fr->cpu_opts.synth_4to1_mono = NULL;
	fr->cpu_opts.synth_4to1_mono2stereo = NULL;
	fr->cpu_opts.synth_ntom = NULL;
	fr->cpu_opts.synth_ntom_mono = NULL;
	fr->cpu_opts.synth_ntom_mono2stereo = NULL;
	fr->cpu_opts.synth_1to1_8bit = NULL;
	fr->cpu_opts.synth_1to1_8bit_mono = NULL;
	fr->cpu_opts.synth_1to1_8bit_mono2stereo = NULL;
	fr->cpu_opts.synth_2to1_8bit = NULL;
	fr->cpu_opts.synth_2to1_8bit_mono = NULL;
	fr->cpu_opts.synth_2to1_8bit_mono2stereo = NULL;
	fr->cpu_opts.synth_4to1_8bit = NULL;
	fr->cpu_opts.synth_4to1_8bit_mono = NULL;
	fr->cpu_opts.synth_4to1_8bit_mono2stereo = NULL;
	fr->cpu_opts.synth_ntom_8bit = NULL;
	fr->cpu_opts.synth_ntom_8bit_mono = NULL;
	fr->cpu_opts.synth_ntom_8bit_mono2stereo = NULL;
#ifndef REAL_IS_FIXED
	fr->cpu_opts.synth_1to1_real = NULL;
	fr->cpu_opts.synth_1to1_real_mono = NULL;
	fr->cpu_opts.synth_1to1_real_mono2stereo = NULL;
	fr->cpu_opts.synth_2to1_real = NULL;
	fr->cpu_opts.synth_2to1_real_mono = NULL;
	fr->cpu_opts.synth_2to1_real_mono2stereo = NULL;
	fr->cpu_opts.synth_4to1_real = NULL;
	fr->cpu_opts.synth_4to1_real_mono = NULL;
	fr->cpu_opts.synth_4to1_real_mono2stereo = NULL;
	fr->cpu_opts.synth_ntom_real = NULL;
	fr->cpu_opts.synth_ntom_real_mono = NULL;
	fr->cpu_opts.synth_ntom_real_mono2stereo = NULL;
#endif

	fr->cpu_opts.type = nodec;
	/* covers any i386+ cpu; they actually differ only in the synth_1to1 function... */
#ifdef OPT_X86

	#ifdef OPT_3DNOW
	fr->cpu_opts.dct36 = dct36;
	#endif
	#ifdef OPT_3DNOWEXT
	fr->cpu_opts.dct36 = dct36;
	#endif

	if(cpu_i586(cpu_flags))
	{
		debug2("standard flags: 0x%08x\textended flags: 0x%08x", cpu_flags.std, cpu_flags.ext);
		#ifdef OPT_3DNOWEXT
		if(   !done && (auto_choose || want_dec == dreidnowext )
		   && cpu_3dnow(cpu_flags)
		   && cpu_3dnowext(cpu_flags)
		   && cpu_mmx(cpu_flags) )
		{
			chosen = "3DNowExt";
			fr->cpu_opts.type = dreidnowext;
			fr->cpu_opts.dct36 = dct36_3dnowext;
			fr->cpu_opts.synth_1to1 = synth_1to1_3dnowext;
			done = 1;
		}
		#endif
		#ifdef OPT_SSE
		if(   !done && (auto_choose || want_dec == sse)
		   && cpu_sse(cpu_flags) && cpu_mmx(cpu_flags) )
		{
			chosen = "SSE";
			fr->cpu_opts.type = sse;
			fr->cpu_opts.synth_1to1 = synth_1to1_sse;
			done = 1;
		}
		#endif
		#ifdef OPT_3DNOW
		fr->cpu_opts.dct36 = dct36;
		if(    !done && (auto_choose || want_dec == dreidnow)
		    && cpu_3dnow(cpu_flags) && cpu_mmx(cpu_flags) )
		{
			chosen = "3DNow";
			fr->cpu_opts.type = dreidnow;
			fr->cpu_opts.dct36 = dct36_3dnow;
			fr->cpu_opts.synth_1to1 = synth_1to1_3dnow;
			done = 1;
		}
		#endif
		#ifdef OPT_MMX
		if(   !done && (auto_choose || want_dec == mmx)
		   && cpu_mmx(cpu_flags) )
		{
			chosen = "MMX";
			fr->cpu_opts.type = mmx;
			fr->cpu_opts.synth_1to1 = synth_1to1_mmx;
			done = 1;
		}
		#endif
		#ifdef OPT_I586
		if(!done && (auto_choose || want_dec == ifuenf))
		{
			chosen = "i586/pentium";
			fr->cpu_opts.type = ifuenf;
			fr->cpu_opts.synth_1to1 = synth_1to1_i586;
			done = 1;
		}
		#endif
		#ifdef OPT_I586_DITHER
		if(!done && (auto_choose || want_dec == ifuenf_dither))
		{
			chosen = "dithered i586/pentium";
			fr->cpu_opts.type = ifuenf_dither;
			fr->cpu_opts.synth_1to1 = synth_1to1_i586;
			done = 1;
		}
		#endif
	}
	#ifdef OPT_I486
	/* That won't cooperate in multi opt mode - forcing i486 in layer3.c
	   But still... here it is... maybe for real use in future. */
	if(!done && (auto_choose || want_dec == ivier))
	{
		chosen = "i486";
		fr->cpu_opts.type = ivier;
		done = 1;
	}
	#endif
	#ifdef OPT_I386
	if(!done && (auto_choose || want_dec == idrei))
	{
		chosen = "i386";
		fr->cpu_opts.type = idrei;
		done = 1;
	}
	#endif

	if(done)
	{ /* We have chosen some x86 decoder... */
		/* First, we see if there is indeed some special (non-i386) synth_1to1 and use the 8bit wrappers over it.
		   If not, we use the direct i386 8bit synth and the normal mono functions. */
		if(fr->cpu_opts.synth_1to1 != NULL)
		{
			fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit_wrap;
			fr->cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_wrap_mono;
			fr->cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_wrap_mono2stereo;
		}
		else
		{
			fr->cpu_opts.synth_1to1 = synth_1to1_i386;
			fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit_i386;
		}
		/*
			Now fill in the non-mono synths that are still missing from the i386 variants.
			

			## This is an inline bourne shell script for execution in nedit to generate the lines below.
			## The ## is a quote for just #
			for t in "" _8bit _real
			do
				if test "$t" = _real; then
					echo "##		ifndef REAL_IS_FIXED"
				fi
				for i in 1to1 2to1 4to1
				do
					echo "		if(fr->cpu_opts.synth_${i}${t} == NULL) fr->cpu_opts.synth_${i}${t} = synth_${i}${t}_i386;"
				done
				if test "$t" = _real; then
					echo "##		endif"
				fi
			done
		*/
		if(fr->cpu_opts.synth_1to1 == NULL) fr->cpu_opts.synth_1to1 = synth_1to1_i386;
		if(fr->cpu_opts.synth_2to1 == NULL) fr->cpu_opts.synth_2to1 = synth_2to1_i386;
		if(fr->cpu_opts.synth_4to1 == NULL) fr->cpu_opts.synth_4to1 = synth_4to1_i386;
		if(fr->cpu_opts.synth_1to1_8bit == NULL) fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit_i386;
		if(fr->cpu_opts.synth_2to1_8bit == NULL) fr->cpu_opts.synth_2to1_8bit = synth_2to1_8bit_i386;
		if(fr->cpu_opts.synth_4to1_8bit == NULL) fr->cpu_opts.synth_4to1_8bit = synth_4to1_8bit_i386;
#		ifndef REAL_IS_FIXED
		if(fr->cpu_opts.synth_1to1_real == NULL) fr->cpu_opts.synth_1to1_real = synth_1to1_real_i386;
		if(fr->cpu_opts.synth_2to1_real == NULL) fr->cpu_opts.synth_2to1_real = synth_2to1_real_i386;
		if(fr->cpu_opts.synth_4to1_real == NULL) fr->cpu_opts.synth_4to1_real = synth_4to1_real_i386;
#		endif
	}

#endif /* OPT_X86 */

	#ifdef OPT_ALTIVEC
	if(!done && (auto_choose || want_dec = altivec))
	{
		chosen = "AltiVec";
		fr->cpu_opts.type = altivec;
		fr->cpu_opts.synth_1to1 = synth_1to1_altivec;
		fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit_wrap;
		fr->cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_wrap_mono;
		fr->cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_wrap_mono2stereo;
		done = 1;
	}
	#endif

	#ifdef OPT_GENERIC
	if(!done && (auto_choose || want_dec == generic))
	{
		chosen = "generic";
		fr->cpu_opts.type = generic;
		done = 1;
	}
	#endif

	fr->cpu_opts.class = decclass(fr->cpu_opts.type);

	/*
		Filling in the last bits.
		No need to care about 8bit wrappers here, that's all set.
		Just set everything still missing to a generic function.

		## This is an inline bourne shell script for execution in nedit to generate the lines below.
		## The ## is a quote for just #
		for t in "" _8bit _real
		do
			if test "$t" = _real; then
				echo "##	ifndef REAL_IS_FIXED"
			fi
			for i in 1to1 2to1 4to1 ntom
			do
			for m in "" _mono _mono2stereo
			do
				echo "	if(fr->cpu_opts.synth_${i}${t}${m} == NULL) fr->cpu_opts.synth_${i}${t}${m} = synth_${i}${t}${m};"
			done
			done
			if test "$t" = _real; then
				echo "##	endif"
			fi
		done
	*/
	if(fr->cpu_opts.synth_1to1 == NULL) fr->cpu_opts.synth_1to1 = synth_1to1;
	if(fr->cpu_opts.synth_1to1_mono == NULL) fr->cpu_opts.synth_1to1_mono = synth_1to1_mono;
	if(fr->cpu_opts.synth_1to1_mono2stereo == NULL) fr->cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo;
	if(fr->cpu_opts.synth_2to1 == NULL) fr->cpu_opts.synth_2to1 = synth_2to1;
	if(fr->cpu_opts.synth_2to1_mono == NULL) fr->cpu_opts.synth_2to1_mono = synth_2to1_mono;
	if(fr->cpu_opts.synth_2to1_mono2stereo == NULL) fr->cpu_opts.synth_2to1_mono2stereo = synth_2to1_mono2stereo;
	if(fr->cpu_opts.synth_4to1 == NULL) fr->cpu_opts.synth_4to1 = synth_4to1;
	if(fr->cpu_opts.synth_4to1_mono == NULL) fr->cpu_opts.synth_4to1_mono = synth_4to1_mono;
	if(fr->cpu_opts.synth_4to1_mono2stereo == NULL) fr->cpu_opts.synth_4to1_mono2stereo = synth_4to1_mono2stereo;
	if(fr->cpu_opts.synth_ntom == NULL) fr->cpu_opts.synth_ntom = synth_ntom;
	if(fr->cpu_opts.synth_ntom_mono == NULL) fr->cpu_opts.synth_ntom_mono = synth_ntom_mono;
	if(fr->cpu_opts.synth_ntom_mono2stereo == NULL) fr->cpu_opts.synth_ntom_mono2stereo = synth_ntom_mono2stereo;
	if(fr->cpu_opts.synth_1to1_8bit == NULL) fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit;
	if(fr->cpu_opts.synth_1to1_8bit_mono == NULL) fr->cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono;
	if(fr->cpu_opts.synth_1to1_8bit_mono2stereo == NULL) fr->cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo;
	if(fr->cpu_opts.synth_2to1_8bit == NULL) fr->cpu_opts.synth_2to1_8bit = synth_2to1_8bit;
	if(fr->cpu_opts.synth_2to1_8bit_mono == NULL) fr->cpu_opts.synth_2to1_8bit_mono = synth_2to1_8bit_mono;
	if(fr->cpu_opts.synth_2to1_8bit_mono2stereo == NULL) fr->cpu_opts.synth_2to1_8bit_mono2stereo = synth_2to1_8bit_mono2stereo;
	if(fr->cpu_opts.synth_4to1_8bit == NULL) fr->cpu_opts.synth_4to1_8bit = synth_4to1_8bit;
	if(fr->cpu_opts.synth_4to1_8bit_mono == NULL) fr->cpu_opts.synth_4to1_8bit_mono = synth_4to1_8bit_mono;
	if(fr->cpu_opts.synth_4to1_8bit_mono2stereo == NULL) fr->cpu_opts.synth_4to1_8bit_mono2stereo = synth_4to1_8bit_mono2stereo;
	if(fr->cpu_opts.synth_ntom_8bit == NULL) fr->cpu_opts.synth_ntom_8bit = synth_ntom_8bit;
	if(fr->cpu_opts.synth_ntom_8bit_mono == NULL) fr->cpu_opts.synth_ntom_8bit_mono = synth_ntom_8bit_mono;
	if(fr->cpu_opts.synth_ntom_8bit_mono2stereo == NULL) fr->cpu_opts.synth_ntom_8bit_mono2stereo = synth_ntom_8bit_mono2stereo;
#	ifndef REAL_IS_FIXED
	if(fr->cpu_opts.synth_1to1_real == NULL) fr->cpu_opts.synth_1to1_real = synth_1to1_real;
	if(fr->cpu_opts.synth_1to1_real_mono == NULL) fr->cpu_opts.synth_1to1_real_mono = synth_1to1_real_mono;
	if(fr->cpu_opts.synth_1to1_real_mono2stereo == NULL) fr->cpu_opts.synth_1to1_real_mono2stereo = synth_1to1_real_mono2stereo;
	if(fr->cpu_opts.synth_2to1_real == NULL) fr->cpu_opts.synth_2to1_real = synth_2to1_real;
	if(fr->cpu_opts.synth_2to1_real_mono == NULL) fr->cpu_opts.synth_2to1_real_mono = synth_2to1_real_mono;
	if(fr->cpu_opts.synth_2to1_real_mono2stereo == NULL) fr->cpu_opts.synth_2to1_real_mono2stereo = synth_2to1_real_mono2stereo;
	if(fr->cpu_opts.synth_4to1_real == NULL) fr->cpu_opts.synth_4to1_real = synth_4to1_real;
	if(fr->cpu_opts.synth_4to1_real_mono == NULL) fr->cpu_opts.synth_4to1_real_mono = synth_4to1_real_mono;
	if(fr->cpu_opts.synth_4to1_real_mono2stereo == NULL) fr->cpu_opts.synth_4to1_real_mono2stereo = synth_4to1_real_mono2stereo;
	if(fr->cpu_opts.synth_ntom_real == NULL) fr->cpu_opts.synth_ntom_real = synth_ntom_real;
	if(fr->cpu_opts.synth_ntom_real_mono == NULL) fr->cpu_opts.synth_ntom_real_mono = synth_ntom_real_mono;
	if(fr->cpu_opts.synth_ntom_real_mono2stereo == NULL) fr->cpu_opts.synth_ntom_real_mono2stereo = synth_ntom_real_mono2stereo;
#	endif

#endif /* OPT_MULTI */
	if(done)
	{
		if(VERBOSE) fprintf(stderr, "Decoder: %s\n", chosen);
		return 1;
	}
	else
	{
		if(NOQUIET) error("Could not set optimization!");
		return 0;
	}
}

enum optdec dectype(const char* decoder)
{
	if(   (decoder == NULL)
	   || (decoder[0] == 0)
	   || !strcasecmp(decoder, "auto") )
	return autodec;

	if(!strcasecmp(decoder, "3dnowext"))    return dreidnowext;
	if(!strcasecmp(decoder, "3dnow"))       return dreidnow;
	if(!strcasecmp(decoder, "sse"))         return sse;
	if(!strcasecmp(decoder, "mmx"))         return mmx;
	if(!strcasecmp(decoder, "generic"))     return generic;
	if(!strcasecmp(decoder, "generic_float"))     return generic;
	if(!strcasecmp(decoder, "altivec"))     return altivec;
	if(!strcasecmp(decoder, "i386"))        return idrei;
	if(!strcasecmp(decoder, "i486"))        return ivier;
	if(!strcasecmp(decoder, "i586"))        return ifuenf;
	if(!strcasecmp(decoder, "i586_dither")) return ifuenf_dither;
	return nodec;
}

#ifdef OPT_MULTI

/* same number of entries as full list, but empty at beginning */
static const char *mpg123_supported_decoder_list[] =
{
	#ifdef OPT_3DNOWEXT
	NULL,
	#endif
	#ifdef OPT_SSE
	NULL,
	#endif
	#ifdef OPT_3DNOW
	NULL,
	#endif
	#ifdef OPT_MMX
	NULL,
	#endif
	#ifdef OPT_I586
	NULL,
	#endif
	#ifdef OPT_I586_DITHER
	NULL,
	#endif
	#ifdef OPT_I486
	NULL,
	#endif
	#ifdef OPT_I386
	NULL,
	#endif
	#ifdef OPT_ALTIVEC
	NULL,
	#endif
	NULL, /* generic */
	NULL
};
#endif

static const char *mpg123_decoder_list[] =
{
	#ifdef OPT_3DNOWEXT
	"3DNowExt",
	#endif
	#ifdef OPT_SSE
	"SSE",
	#endif
	#ifdef OPT_3DNOW
	"3DNow",
	#endif
	#ifdef OPT_MMX
	"MMX",
	#endif
	#ifdef OPT_I586
	"i586",
	#endif
	#ifdef OPT_I586_DITHER
	"i586_dither",
	#endif
	#ifdef OPT_I486
	"i486",
	#endif
	#ifdef OPT_I386
	"i386",
	#endif
	#ifdef OPT_ALTIVEC
	"AltiVec",
	#endif
	#ifdef OPT_GENERIC
	"generic",
	#endif
	NULL
};

void check_decoders(void )
{
#ifndef OPT_MULTI
	return;
#else
	const char **d = mpg123_supported_decoder_list;
#ifdef OPT_X86
	getcpuflags(&cpu_flags);
	if(cpu_i586(cpu_flags))
	{
		/* not yet: if(cpu_sse2(cpu_flags)) printf(" SSE2");
		if(cpu_sse3(cpu_flags)) printf(" SSE3"); */
#ifdef OPT_3DNOWEXT
		if(cpu_3dnowext(cpu_flags)) *(d++) = "3DNowExt";
#endif
#ifdef OPT_SSE
		if(cpu_sse(cpu_flags)) *(d++) = "SSE";
#endif
#ifdef OPT_3DNOW
		if(cpu_3dnow(cpu_flags)) *(d++) = "3DNow";
#endif
#ifdef OPT_MMX
		if(cpu_mmx(cpu_flags)) *(d++) = "MMX";
#endif
#ifdef OPT_I586
		*(d++) = "i586";
#endif
#ifdef OPT_I586_DITHER
		*(d++) = "i586_dither";
#endif
	}
#endif
/* just assume that the i486 built is run on a i486 cpu... */
#ifdef OPT_I486
	*(d++) = "i486";
#endif
#ifdef OPT_ALTIVEC
	*(d++) = "AltiVec";
#endif
/* every supported x86 can do i386, any cpu can do generic */
#ifdef OPT_I386
	*(d++) = "i386";
#endif
#ifdef OPT_GENERIC
	*(d++) = "generic";
#endif
#endif /* ndef OPT_MULTI */
}

int attribute_align_arg mpg123_current_decoder(mpg123_handle *mh)
{
	if(mh == NULL) return MPG123_ERR;

#ifdef OPT_MULTI
	{
		int idx = 0;
		const char* dn = decname[mh->cpu_opts.type];
		const char** cmp = mpg123_decoder_list;
		while(*cmp != NULL && strcasecmp(dn, *cmp))
		{
			++cmp;
			++idx;
		}
		if(*cmp == NULL) return -1;
		else return idx;
	}
#endif

	return 0;
}

const char attribute_align_arg **mpg123_decoders(){ return mpg123_decoder_list; }
const char attribute_align_arg **mpg123_supported_decoders()
{
#ifdef OPT_MULTI
	return mpg123_supported_decoder_list;
#else
	return mpg123_decoder_list;
#endif
}
