/*
	optimize: get a grip on the different optimizations

	copyright 2006-9 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, inspired by 3DNow stuff in mpg123.[hc]

	Currently, this file contains the struct and function to choose an optimization variant and works only when OPT_MULTI is in effect.
*/

#include "mpg123lib_intern.h" /* includes optimize.h */
#include "debug.h"

/* Must match the enum dectype! */

/*
	It SUCKS having to define these names that way, but compile-time intialization of string arrays is a bitch.
	GCC doesn't see constant stuff when it's wiggling in front of it!
	Anyhow: Have a script for that:
names="generic generic_dither i386 i486 i586 i586_dither MMX 3DNow 3DNowExt AltiVec SSE"
for i in $names; do echo "##define dn_$i \"$i\""; done
echo -n "static const char* decname[] =
{
	\"auto\"
	"
for i in $names; do echo -n ", dn_$i"; done
echo "
	, \"nodec\"
};"
*/
#define dn_generic "generic"
#define dn_generic_dither "generic_dither"
#define dn_i386 "i386"
#define dn_i486 "i486"
#define dn_i586 "i586"
#define dn_i586_dither "i586_dither"
#define dn_MMX "MMX"
#define dn_3DNow "3DNow"
#define dn_3DNowExt "3DNowExt"
#define dn_AltiVec "AltiVec"
#define dn_SSE "SSE"
static const char* decname[] =
{
	"auto"
	, dn_generic, dn_generic_dither, dn_i386, dn_i486, dn_i586, dn_i586_dither, dn_MMX, dn_3DNow, dn_3DNowExt, dn_AltiVec, dn_SSE
	, "nodec"
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
#ifndef NO_8BIT
#ifndef NO_16BIT
	if(basic_synth == synth_1to1_8bit_wrap)
	basic_synth = opt_synth_1to1(fr); /* That is what's really below the surface. */
#endif
#endif

	if(FALSE) ; /* Just to initialize the else if ladder. */
#ifndef NO_16BIT
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
#endif /* 16bit */
#ifdef OPT_I386
	else if
	( FALSE /* just as a first value for the || chain */
#ifndef NO_16BIT
		|| basic_synth == synth_1to1_i386
#endif
#ifndef NO_8BIT
		|| basic_synth == synth_1to1_8bit_i386
#endif
#ifndef NO_DOWNSAMPLE
#ifndef NO_16BIT
		|| basic_synth == synth_2to1_i386
		|| basic_synth == synth_4to1_i386
#endif
#ifndef NO_8BIT
		|| basic_synth == synth_2to1_8bit_i386
		|| basic_synth == synth_4to1_8bit_i386
#endif
#endif
#ifndef REAL_IS_FIXED
#ifndef NO_REAL
		|| basic_synth == synth_1to1_real_i386
#endif
#ifndef NO_32BIT
		|| basic_synth == synth_1to1_s32_i386
#endif
#ifndef NO_DOWNSAMPLE
#ifndef NO_REAL
		|| basic_synth == synth_2to1_real_i386
		|| basic_synth == synth_4to1_real_i386
#endif
#ifndef NO_32BIT
		|| basic_synth == synth_2to1_s32_i386
		|| basic_synth == synth_4to1_s32_i386
#endif
#endif
#endif
	) type = idrei;
#endif
	else if
	( FALSE
#ifndef NO_16BIT
		|| basic_synth == synth_1to1
#ifndef NO_DOWNSAMPLE
		|| basic_synth == synth_2to1
		|| basic_synth == synth_4to1
#endif
#ifndef NO_NTOM
		|| basic_synth == synth_ntom
#endif
#endif
#ifndef NO_8BIT
		|| basic_synth == synth_1to1_8bit
#ifndef NO_DOWNSAMPLE
		|| basic_synth == synth_2to1_8bit
		|| basic_synth == synth_4to1_8bit
#endif
#ifndef NO_NTOM
		|| basic_synth == synth_ntom_8bit
#endif
#endif
#ifndef NO_REAL
		|| basic_synth == synth_1to1_real
#ifndef NO_DOWNSAMPLE
		|| basic_synth == synth_2to1_real
		|| basic_synth == synth_4to1_real
#endif
#ifndef NO_NTOM
		|| basic_synth == synth_ntom_real
#endif
#endif
#ifndef NO_32BIT
		|| basic_synth == synth_1to1_s32
#ifndef NO_DOWNSAMPLE
		|| basic_synth == synth_2to1_s32
		|| basic_synth == synth_4to1_s32
#endif
#ifndef NO_NTOM
		|| basic_synth == synth_ntom_s32
#endif
#endif
	) type = generic;
#ifndef NO_16BIT
#ifdef OPT_GENERIC_DITHER
	else if(basic_synth == synth_1to1_dither) type = generic_dither;
#endif
#ifdef OPT_DITHER /* either i586 or generic! */
#ifndef NO_DOWNSAMPLE
	else if
	(
		   basic_synth == synth_2to1_dither
		|| basic_synth == synth_4to1_dither
	) type = generic_dither;
#endif
#endif
#ifdef OPT_ALTIVEC
	else if(basic_synth == synth_1to1_altivec) type = altivec;
#endif
#endif /* 16bit */
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
#ifndef NO_16BIT
	funcs[OUT_16][0] = (func_synth) opt_synth_1to1(fr);
#ifndef NO_DOWNSAMPLE
	funcs[OUT_16][1] = (func_synth) opt_synth_2to1(fr);
	funcs[OUT_16][2] = (func_synth) opt_synth_4to1(fr);
#endif
#ifndef NO_NTOM
	funcs[OUT_16][3] = (func_synth) opt_synth_ntom(fr);
#endif
#endif
#ifndef NO_8BIT
	funcs[OUT_8][0] = (func_synth) opt_synth_1to1_8bit(fr);
#ifndef NO_DOWNSAMPLE
	funcs[OUT_8][1] = (func_synth) opt_synth_2to1_8bit(fr);
	funcs[OUT_8][2] = (func_synth) opt_synth_4to1_8bit(fr);
#endif
#ifndef NO_NTOM
	funcs[OUT_8][3] = (func_synth) opt_synth_ntom_8bit(fr);
#endif
#endif
#ifndef NO_REAL
	funcs[OUT_REAL][0] = (func_synth) opt_synth_1to1_real(fr);
#ifndef NO_DOWNSAMPLE
	funcs[OUT_REAL][1] = (func_synth) opt_synth_2to1_real(fr);
	funcs[OUT_REAL][2] = (func_synth) opt_synth_4to1_real(fr);
#endif
#ifndef NO_NTOM
	funcs[OUT_REAL][3] = (func_synth) opt_synth_ntom_real(fr);
#endif
#endif
#ifndef NO_32BIT
	funcs[OUT_S32][0] = (func_synth) opt_synth_1to1_s32(fr);
#ifndef NO_DOWNSAMPLE
	funcs[OUT_S32][1] = (func_synth) opt_synth_2to1_s32(fr);
	funcs[OUT_S32][2] = (func_synth) opt_synth_4to1_s32(fr);
#endif
#ifndef NO_NTOM
	funcs[OUT_S32][3] = (func_synth) opt_synth_ntom_s32(fr);
#endif
#endif

#ifndef NO_16BIT
	funcs_mono[OUT_16][0] = (func_synth_mono) opt_synth_1to1_mono(fr);
#ifndef NO_DOWNSAMPLE
	funcs_mono[OUT_16][1] = (func_synth_mono) opt_synth_2to1_mono(fr);
	funcs_mono[OUT_16][2] = (func_synth_mono) opt_synth_4to1_mono(fr);
#endif
#ifndef NO_NTOM
	funcs_mono[OUT_16][3] = (func_synth_mono) opt_synth_ntom_mono(fr);
#endif
#endif
#ifndef NO_8BIT
	funcs_mono[OUT_8][0] = (func_synth_mono) opt_synth_1to1_8bit_mono(fr);
#ifndef NO_DOWNSAMPLE
	funcs_mono[OUT_8][1] = (func_synth_mono) opt_synth_2to1_8bit_mono(fr);
	funcs_mono[OUT_8][2] = (func_synth_mono) opt_synth_4to1_8bit_mono(fr);
#endif
#ifndef NO_NTOM
	funcs_mono[OUT_8][3] = (func_synth_mono) opt_synth_ntom_8bit_mono(fr);
#endif
#endif
#ifndef NO_REAL
	funcs_mono[OUT_REAL][0] = (func_synth_mono) opt_synth_1to1_real_mono(fr);
#ifndef NO_DOWNSAMPLE
	funcs_mono[OUT_REAL][1] = (func_synth_mono) opt_synth_2to1_real_mono(fr);
	funcs_mono[OUT_REAL][2] = (func_synth_mono) opt_synth_4to1_real_mono(fr);
#endif
#ifndef NO_NTOM
	funcs_mono[OUT_REAL][3] = (func_synth_mono) opt_synth_ntom_real_mono(fr);
#endif
#endif
#ifndef NO_32BIT
	funcs_mono[OUT_S32][0] = (func_synth_mono) opt_synth_1to1_s32_mono(fr);
#ifndef NO_DOWNSAMPLE
	funcs_mono[OUT_S32][1] = (func_synth_mono) opt_synth_2to1_s32_mono(fr);
	funcs_mono[OUT_S32][2] = (func_synth_mono) opt_synth_4to1_s32_mono(fr);
#endif
#ifndef NO_NTOM
	funcs_mono[OUT_S32][3] = (func_synth_mono) opt_synth_ntom_s32_mono(fr);
#endif
#endif

#ifndef NO_16BIT
	funcs_mono2stereo[OUT_16][0] = (func_synth_mono) opt_synth_1to1_mono2stereo(fr);
#ifndef NO_DOWNSAMPLE
	funcs_mono2stereo[OUT_16][1] = (func_synth_mono) opt_synth_2to1_mono2stereo(fr);
	funcs_mono2stereo[OUT_16][2] = (func_synth_mono) opt_synth_4to1_mono2stereo(fr);
#endif
#ifndef NO_NTOM
	funcs_mono2stereo[OUT_16][3] = (func_synth_mono) opt_synth_ntom_mono2stereo(fr);
#endif
#endif
#ifndef NO_8BIT
	funcs_mono2stereo[OUT_8][0] = (func_synth_mono) opt_synth_1to1_8bit_mono2stereo(fr);
#ifndef NO_DOWNSAMPLE
	funcs_mono2stereo[OUT_8][1] = (func_synth_mono) opt_synth_2to1_8bit_mono2stereo(fr);
	funcs_mono2stereo[OUT_8][2] = (func_synth_mono) opt_synth_4to1_8bit_mono2stereo(fr);
#endif
#ifndef NO_NTOM
	funcs_mono2stereo[OUT_8][3] = (func_synth_mono) opt_synth_ntom_8bit_mono2stereo(fr);
#endif
#endif
#ifndef NO_REAL
	funcs_mono2stereo[OUT_REAL][0] = (func_synth_mono) opt_synth_1to1_real_mono2stereo(fr);
#ifndef NO_DOWNSAMPLE
	funcs_mono2stereo[OUT_REAL][1] = (func_synth_mono) opt_synth_2to1_real_mono2stereo(fr);
	funcs_mono2stereo[OUT_REAL][2] = (func_synth_mono) opt_synth_4to1_real_mono2stereo(fr);
#endif
#ifndef NO_NTOM
	funcs_mono2stereo[OUT_REAL][3] = (func_synth_mono) opt_synth_ntom_real_mono2stereo(fr);
#endif
#endif
#ifndef NO_32BIT
	funcs_mono2stereo[OUT_S32][0] = (func_synth_mono) opt_synth_1to1_s32_mono2stereo(fr);
#ifndef NO_DOWNSAMPLE
	funcs_mono2stereo[OUT_S32][1] = (func_synth_mono) opt_synth_2to1_s32_mono2stereo(fr);
	funcs_mono2stereo[OUT_S32][2] = (func_synth_mono) opt_synth_4to1_s32_mono2stereo(fr);
#endif
#ifndef NO_NTOM
	funcs_mono2stereo[OUT_S32][3] = (func_synth_mono) opt_synth_ntom_s32_mono2stereo(fr);
#endif
#endif

	/* Select the basic output format, different from 16bit: 8bit, real. */
	if(fr->af.encoding & MPG123_ENC_8)
	basic_format = OUT_8;
	else if(fr->af.encoding & MPG123_ENC_FLOAT)
	basic_format = OUT_REAL;
	else if(fr->af.encoding & MPG123_ENC_32)
	basic_format = OUT_S32;

	if /* Make sure the chosen format is compiled into this lib. */
	( FALSE
#ifdef NO_8BIT
		|| basic_format == OUT_8
#endif
#ifdef NO_16BIT
		|| basic_format == OUT_16
#endif
#ifdef NO_32BIT
		|| basic_format == OUT_S32
#endif
#ifdef NO_REAL
		|| basic_format == OUT_REAL
#endif
	)
	{
		if(NOQUIET) error("set_synth_functions: This output format is disabled in this build!");

		return -1;
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
		fr->err = MPG123_NO_BUFFERS;
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
#ifndef NO_LAYER3
		init_layer3_stuff(fr, init_layer3_gainpow2_mmx);
#endif
#ifndef NO_LAYER12
		init_layer12_stuff(fr, init_layer12_table_mmx);
#endif
		fr->make_decode_tables = make_decode_tables_mmx;
	}
	else
#endif
	{
#ifndef NO_LAYER3
		init_layer3_stuff(fr, init_layer3_gainpow2);
#endif
#ifndef NO_LAYER12
		init_layer12_stuff(fr, init_layer12_table);
#endif
		fr->make_decode_tables = make_decode_tables;
	}

	/* We allocated the table buffers just now, so (re)create the tables. */
	fr->make_decode_tables(fr);

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
	for t in "" _8bit _s32 _real
	do
		test "$t" = _real   && echo "##ifndef NO_REAL"
		test "$t" = ""      && echo "##ifndef NO_16BIT"
		test "$t" = "_8bit" && echo "##ifndef NO_8BIT"
		test "$t" = "_s32"  && echo "##ifndef NO_32BIT"

		for i in 1to1 2to1 4to1 ntom;
		do
		if test "$i" = ntom; then
			echo "##ifndef NO_NTOM"
		fi
		if test "$i" = 2to1; then
			echo "##ifndef NO_DOWNSAMPLE"
		fi
		for f in "" _mono _mono2stereo;
		do
			echo "	fr->cpu_opts.synth_${i}${t}${f} = NULL;"
		done
		if test "$i" = ntom; then
			echo "##endif"
		fi
		if test "$i" = 4to1; then
			echo "##endif"
		fi
		done

		echo "##endif"
	done
*/
#ifndef NO_16BIT
	fr->cpu_opts.synth_1to1 = NULL;
	fr->cpu_opts.synth_1to1_mono = NULL;
	fr->cpu_opts.synth_1to1_mono2stereo = NULL;
#ifndef NO_DOWNSAMPLE
	fr->cpu_opts.synth_2to1 = NULL;
	fr->cpu_opts.synth_2to1_mono = NULL;
	fr->cpu_opts.synth_2to1_mono2stereo = NULL;
	fr->cpu_opts.synth_4to1 = NULL;
	fr->cpu_opts.synth_4to1_mono = NULL;
	fr->cpu_opts.synth_4to1_mono2stereo = NULL;
#endif
#ifndef NO_NTOM
	fr->cpu_opts.synth_ntom = NULL;
	fr->cpu_opts.synth_ntom_mono = NULL;
	fr->cpu_opts.synth_ntom_mono2stereo = NULL;
#endif
#endif
#ifndef NO_8BIT
	fr->cpu_opts.synth_1to1_8bit = NULL;
	fr->cpu_opts.synth_1to1_8bit_mono = NULL;
	fr->cpu_opts.synth_1to1_8bit_mono2stereo = NULL;
#ifndef NO_DOWNSAMPLE
	fr->cpu_opts.synth_2to1_8bit = NULL;
	fr->cpu_opts.synth_2to1_8bit_mono = NULL;
	fr->cpu_opts.synth_2to1_8bit_mono2stereo = NULL;
	fr->cpu_opts.synth_4to1_8bit = NULL;
	fr->cpu_opts.synth_4to1_8bit_mono = NULL;
	fr->cpu_opts.synth_4to1_8bit_mono2stereo = NULL;
#endif
#ifndef NO_NTOM
	fr->cpu_opts.synth_ntom_8bit = NULL;
	fr->cpu_opts.synth_ntom_8bit_mono = NULL;
	fr->cpu_opts.synth_ntom_8bit_mono2stereo = NULL;
#endif
#endif
#ifndef NO_32BIT
	fr->cpu_opts.synth_1to1_s32 = NULL;
	fr->cpu_opts.synth_1to1_s32_mono = NULL;
	fr->cpu_opts.synth_1to1_s32_mono2stereo = NULL;
#ifndef NO_DOWNSAMPLE
	fr->cpu_opts.synth_2to1_s32 = NULL;
	fr->cpu_opts.synth_2to1_s32_mono = NULL;
	fr->cpu_opts.synth_2to1_s32_mono2stereo = NULL;
	fr->cpu_opts.synth_4to1_s32 = NULL;
	fr->cpu_opts.synth_4to1_s32_mono = NULL;
	fr->cpu_opts.synth_4to1_s32_mono2stereo = NULL;
#endif
#ifndef NO_NTOM
	fr->cpu_opts.synth_ntom_s32 = NULL;
	fr->cpu_opts.synth_ntom_s32_mono = NULL;
	fr->cpu_opts.synth_ntom_s32_mono2stereo = NULL;
#endif
#endif
#ifndef NO_REAL
	fr->cpu_opts.synth_1to1_real = NULL;
	fr->cpu_opts.synth_1to1_real_mono = NULL;
	fr->cpu_opts.synth_1to1_real_mono2stereo = NULL;
#ifndef NO_DOWNSAMPLE
	fr->cpu_opts.synth_2to1_real = NULL;
	fr->cpu_opts.synth_2to1_real_mono = NULL;
	fr->cpu_opts.synth_2to1_real_mono2stereo = NULL;
	fr->cpu_opts.synth_4to1_real = NULL;
	fr->cpu_opts.synth_4to1_real_mono = NULL;
	fr->cpu_opts.synth_4to1_real_mono2stereo = NULL;
#endif
#ifndef NO_NTOM
	fr->cpu_opts.synth_ntom_real = NULL;
	fr->cpu_opts.synth_ntom_real_mono = NULL;
	fr->cpu_opts.synth_ntom_real_mono2stereo = NULL;
#endif
#endif

	fr->cpu_opts.type = nodec;
	/* covers any i386+ cpu; they actually differ only in the synth_1to1 function... */
#ifdef OPT_X86

#ifndef NO_LAYER3
#if (defined OPT_3DNOW || defined OPT_3DNOWEXT)
	fr->cpu_opts.dct36 = dct36;
#endif
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
#			ifndef NO_LAYER3
			fr->cpu_opts.dct36 = dct36_3dnowext;
#			endif
#			ifndef NO_16BIT
			fr->cpu_opts.synth_1to1 = synth_1to1_3dnowext;
#			endif
			done = 1;
		}
		#endif
		#ifdef OPT_SSE
		if(   !done && (auto_choose || want_dec == sse)
		   && cpu_sse(cpu_flags) && cpu_mmx(cpu_flags) )
		{
			chosen = "SSE";
			fr->cpu_opts.type = sse;
#			ifndef NO_16BIT
			fr->cpu_opts.synth_1to1 = synth_1to1_sse;
#			endif
			done = 1;
		}
		#endif
		#ifdef OPT_3DNOW
#		ifndef NO_LAYER3
		fr->cpu_opts.dct36 = dct36;
#		endif
		if(    !done && (auto_choose || want_dec == dreidnow)
		    && cpu_3dnow(cpu_flags) && cpu_mmx(cpu_flags) )
		{
			chosen = "3DNow";
			fr->cpu_opts.type = dreidnow;
#			ifndef NO_LAYER3
			fr->cpu_opts.dct36 = dct36_3dnow;
#			endif
#			ifndef NO_16BIT
			fr->cpu_opts.synth_1to1 = synth_1to1_3dnow;
#			endif
			done = 1;
		}
		#endif
		#ifdef OPT_MMX
		if(   !done && (auto_choose || want_dec == mmx)
		   && cpu_mmx(cpu_flags) )
		{
			chosen = "MMX";
			fr->cpu_opts.type = mmx;
#			ifndef NO_16BIT
			fr->cpu_opts.synth_1to1 = synth_1to1_mmx;
#			endif
			done = 1;
		}
		#endif
		#ifdef OPT_I586
		if(!done && (auto_choose || want_dec == ifuenf))
		{
			chosen = "i586/pentium";
			fr->cpu_opts.type = ifuenf;
#			ifndef NO_16BIT
			fr->cpu_opts.synth_1to1 = synth_1to1_i586;
#			endif
			done = 1;
		}
		#endif
		#ifdef OPT_I586_DITHER
		if(!done && (auto_choose || want_dec == ifuenf_dither))
		{
			chosen = "dithered i586/pentium";
			fr->cpu_opts.type = ifuenf_dither;
#			ifndef NO_16BIT
			fr->cpu_opts.synth_1to1 = synth_1to1_i586_dither;
#			ifndef NO_DOWNSAMPLE
			fr->cpu_opts.synth_2to1 = synth_2to1_dither;
			fr->cpu_opts.synth_4to1 = synth_4to1_dither;
#			endif
#			endif
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
#ifndef NO_8BIT
#ifndef NO_16BIT /* possibility to use a 16->8 wrapper... */
		if(fr->cpu_opts.synth_1to1 != NULL)
		{
			fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit_wrap;
			fr->cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_wrap_mono;
			fr->cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_wrap_mono2stereo;
		}
		else
		{
			fr->cpu_opts.synth_1to1 = synth_1to1_i386;
#endif /* straight 8bit */
			fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit_i386;
#ifndef NO_16BIT
		}
#endif
#endif
#ifndef NO_16BIT
		if(fr->cpu_opts.synth_1to1 == NULL)
		fr->cpu_opts.synth_1to1 = synth_1to1_i386;
#endif
		/*
			Now fill in the non-mono synths that are still missing from the i386 variants.
			

			## This is an inline bourne shell script for execution in nedit to generate the lines below.
			## The ## is a quote for just #
			for t in "" _8bit _s32 _real
			do
				test "$t" = _real   && echo "##		ifndef NO_REAL"
				test "$t" = ""      && echo "##		ifndef NO_16BIT"
				test "$t" = "_8bit" && echo "##		ifndef NO_8BIT"
				test "$t" = "_s32"  && echo "##		ifndef NO_32BIT"

				for i in 1to1 2to1 4to1
				do
					if test "$i" = 2to1; then
						echo "##		ifndef NO_DOWNSAMPLE"
					fi
					echo "		if(fr->cpu_opts.synth_${i}${t} == NULL) fr->cpu_opts.synth_${i}${t} = synth_${i}${t}_i386;"
					if test "$i" = 4to1; then
						echo "##		endif"
					fi
				done

				echo "##		endif"
			done
		*/
#		ifndef NO_16BIT
		if(fr->cpu_opts.synth_1to1 == NULL) fr->cpu_opts.synth_1to1 = synth_1to1_i386;
#		ifndef NO_DOWNSAMPLE
		if(fr->cpu_opts.synth_2to1 == NULL) fr->cpu_opts.synth_2to1 = synth_2to1_i386;
		if(fr->cpu_opts.synth_4to1 == NULL) fr->cpu_opts.synth_4to1 = synth_4to1_i386;
#		endif
#		endif
#		ifndef NO_8BIT
		if(fr->cpu_opts.synth_1to1_8bit == NULL) fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit_i386;
#		ifndef NO_DOWNSAMPLE
		if(fr->cpu_opts.synth_2to1_8bit == NULL) fr->cpu_opts.synth_2to1_8bit = synth_2to1_8bit_i386;
		if(fr->cpu_opts.synth_4to1_8bit == NULL) fr->cpu_opts.synth_4to1_8bit = synth_4to1_8bit_i386;
#		endif
#		endif
#		ifndef NO_32BIT
		if(fr->cpu_opts.synth_1to1_s32 == NULL) fr->cpu_opts.synth_1to1_s32 = synth_1to1_s32_i386;
#		ifndef NO_DOWNSAMPLE
		if(fr->cpu_opts.synth_2to1_s32 == NULL) fr->cpu_opts.synth_2to1_s32 = synth_2to1_s32_i386;
		if(fr->cpu_opts.synth_4to1_s32 == NULL) fr->cpu_opts.synth_4to1_s32 = synth_4to1_s32_i386;
#		endif
#		endif
#		ifndef NO_REAL
		if(fr->cpu_opts.synth_1to1_real == NULL) fr->cpu_opts.synth_1to1_real = synth_1to1_real_i386;
#		ifndef NO_DOWNSAMPLE
		if(fr->cpu_opts.synth_2to1_real == NULL) fr->cpu_opts.synth_2to1_real = synth_2to1_real_i386;
		if(fr->cpu_opts.synth_4to1_real == NULL) fr->cpu_opts.synth_4to1_real = synth_4to1_real_i386;
#		endif
#		endif
	}

#endif /* OPT_X86 */

#ifdef OPT_GENERIC_DITHER
	if(!done && (auto_choose || want_dec == generic_dither))
	{
		chosen = "dithered generic";
		fr->cpu_opts.type = generic_dither;
#		ifndef NO_16BIT
		fr->cpu_opts.synth_1to1 = synth_1to1_dither;
#		ifndef NO_DOWNSAMPLE
		fr->cpu_opts.synth_2to1 = synth_2to1_dither;
		fr->cpu_opts.synth_4to1 = synth_4to1_dither;
#		endif
#		endif
		/* Wrapping 8bit functions don't make sense for dithering. */
		done = 1;
	}
#endif

	#ifdef OPT_ALTIVEC
	if(!done && (auto_choose || want_dec == altivec))
	{
		chosen = "AltiVec";
		fr->cpu_opts.type = altivec;
#		ifndef NO_16BIT
		fr->cpu_opts.synth_1to1 = synth_1to1_altivec;
		fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit_wrap;
		fr->cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_wrap_mono;
		fr->cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_wrap_mono2stereo;
#		endif
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
		for t in "" _8bit _s32 _real
		do
			test "$t" = _real   && echo "##	ifndef NO_REAL"
			test "$t" = ""      && echo "##	ifndef NO_16BIT"
			test "$t" = "_8bit" && echo "##	ifndef NO_8BIT"
			test "$t" = "_s32"  && echo "##	ifndef NO_32BIT"

			for i in 1to1 2to1 4to1 ntom
			do
			if test "$i" = ntom; then
				echo "##	ifndef NO_NTOM"
			fi
			if test "$i" = 2to1; then
				echo "##	ifndef NO_DOWNSAMPLE"
			fi
			for m in "" _mono _mono2stereo
			do
				echo "	if(fr->cpu_opts.synth_${i}${t}${m} == NULL) fr->cpu_opts.synth_${i}${t}${m} = synth_${i}${t}${m};"
			done
			if test "$i" = ntom; then
				echo "##	endif"
			fi
			if test "$i" = 4to1; then
				echo "##	endif"
			fi
			done

			echo "##	endif"
		done
	*/
#	ifndef NO_16BIT
	if(fr->cpu_opts.synth_1to1 == NULL) fr->cpu_opts.synth_1to1 = synth_1to1;
	if(fr->cpu_opts.synth_1to1_mono == NULL) fr->cpu_opts.synth_1to1_mono = synth_1to1_mono;
	if(fr->cpu_opts.synth_1to1_mono2stereo == NULL) fr->cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo;
#	ifndef NO_DOWNSAMPLE
	if(fr->cpu_opts.synth_2to1 == NULL) fr->cpu_opts.synth_2to1 = synth_2to1;
	if(fr->cpu_opts.synth_2to1_mono == NULL) fr->cpu_opts.synth_2to1_mono = synth_2to1_mono;
	if(fr->cpu_opts.synth_2to1_mono2stereo == NULL) fr->cpu_opts.synth_2to1_mono2stereo = synth_2to1_mono2stereo;
	if(fr->cpu_opts.synth_4to1 == NULL) fr->cpu_opts.synth_4to1 = synth_4to1;
	if(fr->cpu_opts.synth_4to1_mono == NULL) fr->cpu_opts.synth_4to1_mono = synth_4to1_mono;
	if(fr->cpu_opts.synth_4to1_mono2stereo == NULL) fr->cpu_opts.synth_4to1_mono2stereo = synth_4to1_mono2stereo;
#	endif
#	ifndef NO_NTOM
	if(fr->cpu_opts.synth_ntom == NULL) fr->cpu_opts.synth_ntom = synth_ntom;
	if(fr->cpu_opts.synth_ntom_mono == NULL) fr->cpu_opts.synth_ntom_mono = synth_ntom_mono;
	if(fr->cpu_opts.synth_ntom_mono2stereo == NULL) fr->cpu_opts.synth_ntom_mono2stereo = synth_ntom_mono2stereo;
#	endif
#	endif
#	ifndef NO_8BIT
	if(fr->cpu_opts.synth_1to1_8bit == NULL) fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit;
	if(fr->cpu_opts.synth_1to1_8bit_mono == NULL) fr->cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono;
	if(fr->cpu_opts.synth_1to1_8bit_mono2stereo == NULL) fr->cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo;
#	ifndef NO_DOWNSAMPLE
	if(fr->cpu_opts.synth_2to1_8bit == NULL) fr->cpu_opts.synth_2to1_8bit = synth_2to1_8bit;
	if(fr->cpu_opts.synth_2to1_8bit_mono == NULL) fr->cpu_opts.synth_2to1_8bit_mono = synth_2to1_8bit_mono;
	if(fr->cpu_opts.synth_2to1_8bit_mono2stereo == NULL) fr->cpu_opts.synth_2to1_8bit_mono2stereo = synth_2to1_8bit_mono2stereo;
	if(fr->cpu_opts.synth_4to1_8bit == NULL) fr->cpu_opts.synth_4to1_8bit = synth_4to1_8bit;
	if(fr->cpu_opts.synth_4to1_8bit_mono == NULL) fr->cpu_opts.synth_4to1_8bit_mono = synth_4to1_8bit_mono;
	if(fr->cpu_opts.synth_4to1_8bit_mono2stereo == NULL) fr->cpu_opts.synth_4to1_8bit_mono2stereo = synth_4to1_8bit_mono2stereo;
#	endif
#	ifndef NO_NTOM
	if(fr->cpu_opts.synth_ntom_8bit == NULL) fr->cpu_opts.synth_ntom_8bit = synth_ntom_8bit;
	if(fr->cpu_opts.synth_ntom_8bit_mono == NULL) fr->cpu_opts.synth_ntom_8bit_mono = synth_ntom_8bit_mono;
	if(fr->cpu_opts.synth_ntom_8bit_mono2stereo == NULL) fr->cpu_opts.synth_ntom_8bit_mono2stereo = synth_ntom_8bit_mono2stereo;
#	endif
#	endif
#	ifndef NO_32BIT
	if(fr->cpu_opts.synth_1to1_s32 == NULL) fr->cpu_opts.synth_1to1_s32 = synth_1to1_s32;
	if(fr->cpu_opts.synth_1to1_s32_mono == NULL) fr->cpu_opts.synth_1to1_s32_mono = synth_1to1_s32_mono;
	if(fr->cpu_opts.synth_1to1_s32_mono2stereo == NULL) fr->cpu_opts.synth_1to1_s32_mono2stereo = synth_1to1_s32_mono2stereo;
#	ifndef NO_DOWNSAMPLE
	if(fr->cpu_opts.synth_2to1_s32 == NULL) fr->cpu_opts.synth_2to1_s32 = synth_2to1_s32;
	if(fr->cpu_opts.synth_2to1_s32_mono == NULL) fr->cpu_opts.synth_2to1_s32_mono = synth_2to1_s32_mono;
	if(fr->cpu_opts.synth_2to1_s32_mono2stereo == NULL) fr->cpu_opts.synth_2to1_s32_mono2stereo = synth_2to1_s32_mono2stereo;
	if(fr->cpu_opts.synth_4to1_s32 == NULL) fr->cpu_opts.synth_4to1_s32 = synth_4to1_s32;
	if(fr->cpu_opts.synth_4to1_s32_mono == NULL) fr->cpu_opts.synth_4to1_s32_mono = synth_4to1_s32_mono;
	if(fr->cpu_opts.synth_4to1_s32_mono2stereo == NULL) fr->cpu_opts.synth_4to1_s32_mono2stereo = synth_4to1_s32_mono2stereo;
#	endif
#	ifndef NO_NTOM
	if(fr->cpu_opts.synth_ntom_s32 == NULL) fr->cpu_opts.synth_ntom_s32 = synth_ntom_s32;
	if(fr->cpu_opts.synth_ntom_s32_mono == NULL) fr->cpu_opts.synth_ntom_s32_mono = synth_ntom_s32_mono;
	if(fr->cpu_opts.synth_ntom_s32_mono2stereo == NULL) fr->cpu_opts.synth_ntom_s32_mono2stereo = synth_ntom_s32_mono2stereo;
#	endif
#	endif
#	ifndef NO_REAL
	if(fr->cpu_opts.synth_1to1_real == NULL) fr->cpu_opts.synth_1to1_real = synth_1to1_real;
	if(fr->cpu_opts.synth_1to1_real_mono == NULL) fr->cpu_opts.synth_1to1_real_mono = synth_1to1_real_mono;
	if(fr->cpu_opts.synth_1to1_real_mono2stereo == NULL) fr->cpu_opts.synth_1to1_real_mono2stereo = synth_1to1_real_mono2stereo;
#	ifndef NO_DOWNSAMPLE
	if(fr->cpu_opts.synth_2to1_real == NULL) fr->cpu_opts.synth_2to1_real = synth_2to1_real;
	if(fr->cpu_opts.synth_2to1_real_mono == NULL) fr->cpu_opts.synth_2to1_real_mono = synth_2to1_real_mono;
	if(fr->cpu_opts.synth_2to1_real_mono2stereo == NULL) fr->cpu_opts.synth_2to1_real_mono2stereo = synth_2to1_real_mono2stereo;
	if(fr->cpu_opts.synth_4to1_real == NULL) fr->cpu_opts.synth_4to1_real = synth_4to1_real;
	if(fr->cpu_opts.synth_4to1_real_mono == NULL) fr->cpu_opts.synth_4to1_real_mono = synth_4to1_real_mono;
	if(fr->cpu_opts.synth_4to1_real_mono2stereo == NULL) fr->cpu_opts.synth_4to1_real_mono2stereo = synth_4to1_real_mono2stereo;
#	endif
#	ifndef NO_NTOM
	if(fr->cpu_opts.synth_ntom_real == NULL) fr->cpu_opts.synth_ntom_real = synth_ntom_real;
	if(fr->cpu_opts.synth_ntom_real_mono == NULL) fr->cpu_opts.synth_ntom_real_mono = synth_ntom_real_mono;
	if(fr->cpu_opts.synth_ntom_real_mono2stereo == NULL) fr->cpu_opts.synth_ntom_real_mono2stereo = synth_ntom_real_mono2stereo;
#	endif
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
	enum optdec dt;
	if(   (decoder == NULL)
	   || (decoder[0] == 0) )
	return autodec;

	for(dt=autodec; dt<nodec; ++dt)
	if(!strcasecmp(decoder, decname[dt])) return dt;

	return nodec; /* If we found nothing... */
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
	#ifdef OPT_GENERIC_FLOAT
	NULL,
	#endif
#	ifdef OPT_GENERIC
	NULL,
#	endif
#	ifdef OPT_GENERIC_DITHER
	NULL,
#	endif
	NULL
};
#endif

static const char *mpg123_decoder_list[] =
{
	#ifdef OPT_3DNOWEXT
	dn_3DNowExt
	#endif
	#ifdef OPT_SSE
	dn_SSE,
	#endif
	#ifdef OPT_3DNOW
	dn_3DNow,
	#endif
	#ifdef OPT_MMX
	dn_MMX,
	#endif
	#ifdef OPT_I586
	dn_i586,
	#endif
	#ifdef OPT_I586_DITHER
	dn_i586_dither,
	#endif
	#ifdef OPT_I486
	dn_i486,
	#endif
	#ifdef OPT_I386
	dn_i386,
	#endif
	#ifdef OPT_ALTIVEC
	dn_altivec,
	#endif
	#ifdef OPT_GENERIC
	dn_generic,
	#endif
	#ifdef OPT_GENERIC_DITHER
	dn_generic_dither,
	#endif
	NULL
};

void check_decoders(void )
{
#ifndef OPT_MULTI
	/* In non-multi mode, only the full list (one entry) is used. */
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
		if(cpu_3dnowext(cpu_flags)) *(d++) = decname[dreidnowext];
#endif
#ifdef OPT_SSE
		if(cpu_sse(cpu_flags)) *(d++) = decname[sse];
#endif
#ifdef OPT_3DNOW
		if(cpu_3dnow(cpu_flags)) *(d++) = decname[dreidnow];
#endif
#ifdef OPT_MMX
		if(cpu_mmx(cpu_flags)) *(d++) = decname[mmx];
#endif
#ifdef OPT_I586
		*(d++) = decname[ifuenf];
#endif
#ifdef OPT_I586_DITHER
		*(d++) = decname[ifuenf_dither];
#endif
	}
#endif
/* just assume that the i486 built is run on a i486 cpu... */
#ifdef OPT_I486
	*(d++) = decname[ivier];
#endif
#ifdef OPT_ALTIVEC
	*(d++) = decname[altivec];
#endif
/* every supported x86 can do i386, any cpu can do generic */
#ifdef OPT_I386
	*(d++) = decname[idrei];
#endif
#ifdef OPT_GENERIC
	*(d++) = decname[generic];
#endif
#ifdef OPT_GENERIC_DITHER
	*(d++) = decname[generic_dither];
#endif
#endif /* ndef OPT_MULTI */
}

const char* attribute_align_arg mpg123_current_decoder(mpg123_handle *mh)
{
	if(mh == NULL) return NULL;

	return decname[mh->cpu_opts.type];
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
