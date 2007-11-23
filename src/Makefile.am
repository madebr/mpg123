## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://mpg123.org
## initially written by Nicholas J. Humfrey

# AM_CPPFLAGS =
mpg123_LDADD = $(LIBLTDL) libmpg123/libmpg123.la @MODULE_OBJ@ @OUTPUT_OBJ@ @OUTPUT_LIBS@
mpg123_LDFLAGS = @LT_LDFLAGS@ @OUTPUT_LDFLAGS@
# Just mpg123_INCLUDES has no effect on build! Trying that before reverting to AM_CPPFLAGS.
INCLUDES = $(LTDLINCL) -I$(top_srcdir)/src/libmpg123
# libltdl is not mentioned here... it's not that trivial
mpg123_DEPENDENCIES = @OUTPUT_OBJ@ @MODULE_OBJ@ libmpg123/libmpg123.la

SUBDIRS = output libmpg123
EXTRA_DIST = Makefile.legacy config.h.legacy legacy_module.c module.c

CLEANFILES = *.a

bin_PROGRAMS = mpg123
EXTRA_PROGRAMS = mpglib scan

# Dammit, how do I cross-directory dependencies so that they actually _work_?
# Putting all rules in one makefile doesn't work with libtool as long as I got the sources organized in subdirs.
# That SUCKS.

mpglib_LDADD = libmpg123/libmpg123.la
mpglib_DEPENDENCIES = libmpg123/libmpg123.la

scan_LDADD = libmpg123/libmpg123.la
scan_DEPENDENCIES = libmpg123/libmpg123.la
scan_SOURCES = scan.c


mpglib_SOURCES = \
	mpglib.c

mpg123_SOURCES = \
	audio.c \
	audio.h \
	buffer.c \
	buffer.h \
	common.c \
	common.h \
	libmpg123/compat.c \
	libmpg123/compat.h \
	control_generic.c \
	getlopt.c \
	getlopt.h \
	httpget.c \
	httpget.h \
	genre.h \
	genre.c \
	module.h \
	mpg123.c \
	mpg123app.h \
	id3print.c \
	id3print.h \
	playlist.c \
	playlist.h \
	sfifo.c \
	sfifo.h \
	term.c \
	term.h \
	wav.c \
	xfermem.c \
	xfermem.h

# That is not nice... but it is how I manage to get the dependency on output/alsa.o without error about .deps/output/alsa.Tpo .
# Did I mention that recursive make sucks?
output/%.$(OBJEXT): output/%.c audio.h module.h
	cd output && $(MAKE)
