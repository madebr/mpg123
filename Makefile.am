## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://mpg123.org
## initially written by Nicholas J. Humfrey
dist_man_MANS = man1/mpg123.1

SUBDIRS = src doc
DIST_SUBDIRS = src doc

# pkg-config file for the mpg123 library
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmpg123.pc

EXTRA_DIST = \
	libmpg123.pc.in \
	mpg123.spec.in \
	mpg123.spec \
	makedll.sh \
	windows-builds.sh \
	equalize.dat \
	NEWS.libmpg123 \
	autogen.sh \
	ports/MSVC++/mpg123.h \
	ports/MSVC++/config.h \
	ports/MSVC++/msvc.c \
	ports/MSVC++/examples/scan.c \
	ports/MSVC++/examples/feedseek.c \
	ports/MSVC++/2005/libmpg123/libmpg123.vcproj \
	ports/MSVC++/2008/mpg123.sln \
	ports/MSVC++/2008/feedseek/feedseek.vcproj \
	ports/MSVC++/2008/mpglib/mpglib.vcproj \
	ports/MSVC++/2008/libmpg123/libmpg123.vcproj \
	ports/MSVC++/2008/scan/scan.vcproj \
	ports/MSVC++/CMP3Stream/libMPG123/libMPG123.vcproj \
	ports/MSVC++/CMP3Stream/libMPG123/PLACE_LIBMPG123_SOURCES_HERE \
	ports/MSVC++/CMP3Stream/README \
	ports/MSVC++/CMP3Stream/SOURCE/CORE_Log.CPP \
	ports/MSVC++/CMP3Stream/SOURCE/CORE_FileIn.CPP \
	ports/MSVC++/CMP3Stream/SOURCE/SourceFilter_MP3Stream.CPP \
	ports/MSVC++/CMP3Stream/SOURCE/CORE_Mutex.CPP \
	ports/MSVC++/CMP3Stream/INCLUDE/CORE/CORE_FileIn.H \
	ports/MSVC++/CMP3Stream/INCLUDE/CORE/SourceFilter_MP3.H \
	ports/MSVC++/CMP3Stream/INCLUDE/IIEP_FileIn.H \
	ports/MSVC++/CMP3Stream/INCLUDE/IIEP_Def.H \
	ports/README \
	ports/Sony_PSP/config.h \
	ports/Sony_PSP/README \
	ports/Sony_PSP/Makefile.psp \
	ports/Sony_PSP/readers.c.patch \
	ports/mpg123_.pas \
	scripts/benchmark-cpu.pl

