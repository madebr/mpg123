## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://www.mpg123.org
## initially written by Nicholas J. Humfrey

AM_CPPFLAGS = -I$(top_srcdir)/src/libmpg123 -I$(top_srcdir)/src

pkglib_LTLIBRARIES = output_dummy.la
output_dummy_la_SOURCES = dummy.c
output_dummy_la_LDFLAGS = -module -avoid-version

#if HAVE_AIX
#pkglib_LTLIBRARIES += output_aix.la
#output_aix_la_SOURCES = aix.c
#output_aix_la_LDFLAGS = -module -avoid-version
#endif

#if HAVE_ALIB
#pkglib_LTLIBRARIES += output_alib.la
#output_alib_la_SOURCES = alib.c
#output_alib_la_LDFLAGS = -module -avoid-version
#endif

if HAVE_ALSA
pkglib_LTLIBRARIES += output_alsa.la
output_alsa_la_SOURCES = alsa.c
output_alsa_la_LDFLAGS = -module -avoid-version
output_alsa_la_LIBADD  = -lasound
endif

if HAVE_COREAUDIO
pkglib_LTLIBRARIES += output_coreaudio.la
output_coreaudio_la_SOURCES = coreaudio.c
output_coreaudio_la_LDFLAGS = -module -avoid-version \
	-framework AudioToolbox \
	-framework AudioUnit \
	-framework CoreServices
endif

if HAVE_ESOUND
pkglib_LTLIBRARIES += output_esound.la
output_esound_la_SOURCES = esd.c
output_esound_la_CFLAGS  = @ESOUND_CFLAGS@
output_esound_la_LDFLAGS = -module -avoid-version @ESOUND_LDFLAGS@
output_esound_la_LIBADD  = @ESOUND_LIBS@
endif

#if HAVE_HP
#pkglib_LTLIBRARIES += output_hp.la
#output_hp_la_SOURCES = hp.c
#output_hp_la_LDFLAGS = -module -avoid-version
#endif

if HAVE_JACK
pkglib_LTLIBRARIES += output_jack.la
output_jack_la_SOURCES = jack.c
output_jack_la_CFLAGS  = @JACK_CFLAGS@
output_jack_la_LDFLAGS = -module -avoid-version @JACK_LDFLAGS@
output_jack_la_LIBADD  = @JACK_LIBS@
endif

#if HAVE_MINT
#pkglib_LTLIBRARIES += output_mint.la
#output_mint_la_SOURCES = mint.c
#output_mint_la_LDFLAGS = -module -avoid-version
#endif

if HAVE_NAS
pkglib_LTLIBRARIES += output_nas.la
output_nas_la_SOURCES = nas.c
output_nas_la_LDFLAGS = -module -avoid-version
output_nas_la_LIBADD = -laudio
endif

#if HAVE_OS2
#pkglib_LTLIBRARIES += output_os2.la
#output_os2_la_SOURCES = os2.c
#output_os2_la_LDFLAGS = -module -avoid-version
#endif

if HAVE_OSS
pkglib_LTLIBRARIES += output_oss.la
output_oss_la_SOURCES = oss.c
output_oss_la_LDFLAGS = -module -avoid-version
endif

if HAVE_PORTAUDIO
pkglib_LTLIBRARIES += output_portaudio.la
output_portaudio_la_SOURCES = portaudio.c
output_portaudio_la_LDFLAGS = -module -avoid-version
output_portaudio_la_LIBADD  = -lportaudio
endif

if HAVE_PULSE
pkglib_LTLIBRARIES += output_pulse.la
output_pulse_la_SOURCES = pulse.c
output_pulse_la_CFLAGS  = @PULSE_CFLAGS@
output_pulse_la_LDFLAGS = -module -avoid-version @PULSE_LDFLAGS@
output_pulse_la_LIBADD  = @PULSE_LIBS@
endif

if HAVE_SDL
pkglib_LTLIBRARIES += output_sdl.la
output_sdl_la_SOURCES = sdl.c
output_sdl_la_CFLAGS = @SDL_CFLAGS@
output_sdl_la_LDFLAGS = -module -avoid-version @SDL_LDFLAGS@
output_sdl_la_LIBADD = @SDL_LIBS@
endif

#if HAVE_SGI
#pkglib_LTLIBRARIES += output_sgi.la
#output_sgi_la_SOURCES = sgi.c
#output_sgi_la_LDFLAGS = -module -avoid-version
#endif

if HAVE_SUN
pkglib_LTLIBRARIES += output_sun.la
output_sun_la_SOURCES = sun.c
output_sun_la_LDFLAGS = -module -avoid-version
endif

#if HAVE_WIN32
#pkglib_LTLIBRARIES += output_win32.la
#output_win32_la_SOURCES = win32.c
#output_win32_la_LDFLAGS = -module -avoid-version
#endif
