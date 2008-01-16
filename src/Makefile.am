## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://mpg123.org
## initially written by Nicholas J. Humfrey

AM_CPPFLAGS = -DPKGLIBDIR="\"$(pkglibdir)\""
mpg123_LDADD = $(LIBLTDL) libmpg123/libmpg123.la @MODULE_OBJ@ @OUTPUT_OBJ@ @OUTPUT_LIBS@
mpg123_LDFLAGS = @LT_LDFLAGS@ @OUTPUT_LDFLAGS@
# Just mpg123_INCLUDES has no effect on build! Trying that before reverting to AM_CPPFLAGS.
INCLUDES = $(LTDLINCL) -I$(top_builddir)/src/libmpg123 -I$(top_srcdir)/src/libmpg123
# libltdl is not mentioned here... it's not that trivial
mpg123_DEPENDENCIES = @OUTPUT_OBJ@ @MODULE_OBJ@ libmpg123/libmpg123.la

SUBDIRS = output libmpg123
EXTRA_DIST = Makefile.legacy config.h.legacy legacy_module.c module.c

CLEANFILES = *.a

bin_PROGRAMS = mpg123

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

# Would have to mention _all_ source files... Dammit, that's what the libmpg123/Makefile.am does!
# But again, the a make $something here needs that stupid rule... WHY???
libmpg123/libmpg123.la: config.h libmpg123/mpg123.h
	cd libmpg123 && $(MAKE)

