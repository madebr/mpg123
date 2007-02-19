## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://mpg123.de
## initially written by Nicholas J. Humfrey

AM_CFLAGS = @AUDIO_CFLAGS@
AM_LDFLAGS = @AUDIO_LIBS@
mpg123_LDADD = @AUDIO_OBJ@ @CPU_TYPE_LIB@
mpg123_DEPENDENCIES = @AUDIO_OBJ@ @CPU_TYPE_LIB@

EXTRA_DIST = Makefile.legacy config.h.legacy


bin_PROGRAMS = mpg123
mpg123_SOURCES = \
	audio.c \
	audio.h \
	buffer.c \
	buffer.h \
	common.c \
	common.h \
	control_generic.c \
	debug.h \
	decode_2to1.c \
	decode_4to1.c \
	decode_ntom.c \
	equalizer.c \
	getbits.c \
	getbits.h \
	getlopt.c \
	getlopt.h \
	httpget.c \
	huffman.h \
	icy.c \
	icy.h \
	id3.c \
	id3.h \
	genre.h \
	l2tables.h \
	layer1.c \
	layer2.c \
	layer3.c \
	layer3.h \
	mpg123.c \
	mpg123.h \
	optimize.h \
	optimize.c \
	playlist.c \
	playlist.h \
	readers.c \
	tabinit.c \
	sfifo.c \
	sfifo.h \
	stringbuf.c \
	stringbuf.h \
	term.c \
	term.h \
	wav.c \
	xfermem.c \
	xfermem.h

EXTRA_mpg123_SOURCES = \
	audio_aix.c \
	audio_alib.c \
	audio_alsa.c \
	audio_dummy.c \
	audio_esd.c \
	audio_hp.c \
	audio_jack.c \
	audio_macosx.c \
	audio_mint.c \
	audio_nas.c \
	audio_os2.c \
	audio_oss.c \
	audio_portaudio.c \
	audio_sdl.c \
	audio_sgi.c \
	audio_sun.c \
	audio_win32.c



# Processor dependent decoder code
EXTRA_LIBRARIES = \
	libcpu_generic.a \
	libcpu_generic_fpu.a \
	libcpu_generic_nofpu.a \
	libcpu_altivec.a \
	libcpu_i386.a \
	libcpu_i386_fpu.a \
	libcpu_i386_nofpu.a \
	libcpu_i486.a \
	libcpu_i586.a \
	libcpu_i586_dither.a \
	libcpu_3dnow.a \
	libcpu_mmx.a

libcpu_generic_a_SOURCES = \
	decode.c \
	dct64.c

libcpu_generic_fpu_a_SOURCES = \
	decode.c \
	dct64.c

libcpu_generic_nofpu_a_SOURCES = \
	decode.c \
	dct64.c

libcpu_altivec_a_SOURCES = \
	decode_altivec.c \
	dct64_altivec.c

libcpu_i386_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c

libcpu_i386_fpu_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c

libcpu_i386_nofpu_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c

libcpu_i486_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c \
	decode_i486.c \
	dct64_i486.c

libcpu_i586_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c \
	decode_i586.s

libcpu_i586_dither_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c \
	decode_i586_dither.s

libcpu_3dnow_a_SOURCES = \
	decode_i386.c \
	decode_3dnow.s \
	decode_i586.s \
	dct64_i386.c \
	dct64_3dnow.s \
	dct36_3dnow.s \
	equalizer_3dnow.s \
	getcpuflags.s

libcpu_mmx_a_SOURCES = \
	decode_i386.c \
	dct64_mmx.s \
	tabinit_mmx.s \
	decode_mmx.s

