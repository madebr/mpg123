dnl configure.ac: source for the configure script

dnl copyright by the mpg123 project - free software under the terms of the LGPL 2.1
dnl see COPYING and AUTHORS files in distribution or http://mpg123.de
dnl initially written by Nicholas J. Humfrey

dnl Require autoconf version >= 2.57
AC_PREREQ(2.57)


dnl ############# Initialisation

AC_INIT([mpg123], [0.60-beta4], [mpg123-devel@lists.sourceforge.net])

AC_CONFIG_SRCDIR(src/mpg123.c)
AC_CONFIG_AUX_DIR(build)
AC_CANONICAL_TARGET

dnl Version 1.7 of automake is recommended
AM_INIT_AUTOMAKE( 1.7 )
AM_CONFIG_HEADER(src/config.h)



dnl ############# Compiler and tools Checks

AM_PROG_AS
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB

AC_C_CONST
AC_C_BIGENDIAN
AC_INLINE

# Check if system supports termios
AC_SYS_POSIX_TERMIOS
if test "x$ac_cv_sys_posix_termios" = "xyes"; then
  AC_DEFINE_UNQUOTED([HAVE_TERMIOS], 1, 
  [Define this if you have the POSIX termios library])
fi



dnl ############## Configurable Options

AC_ARG_ENABLE(debug,
              [  --enable-debug=[no/yes] turn on debugging],
              [ debugging="enabled" 
                AC_DEFINE(DEBUG, 1, [ Define if debugging is enabled. ]) ],
              [ debugging="disabled" ]
)

AC_ARG_ENABLE(gapless,
              [  --enable-gapless=[no/yes] turn on gapless],
              [ gapless="enabled" 
                AC_DEFINE(GAPLESS, 1, [ Define if gapless is enabled. ]) ],
              [ gapless="disabled" ]
)

AC_ARG_WITH([cpu], [
  --with-cpu=generic      Use generic processor code
  --with-cpu=i386         Use code optimized for i386 processors
  --with-cpu=i486         Use code optimized for i486 processors
  --with-cpu=i586         Use code optimized for i586 processors
  --with-cpu=i586_dither  Use code optimized for i586 processors with dithering (noise shaping)
  --with-cpu=3dnow        Use code optimized for 3dnow processors
  --with-cpu=mmx          Use code optimized for MMX processors
  --with-cpu=altivec      Use code optimized for Altivec processors (PowerPC G4 and G5)
]) 

AC_ARG_WITH([audio], [
  --with-audio=aix        Use AIX audio output sub-system
  --with-audio=alib       Use Alib audio output sub-system (for HPUX)
  --with-audio=alsa       Use ALSA audio output sub-system
  --with-audio=dummy      Use dummy audio (when no sound card is available)
  --with-audio=esd        Use ESoundD audio output sub-system
  --with-audio=hp         Use HP audio output sub-system
  --with-audio=jack       Use JACK low-latency audio server
  --with-audio=macosx     Use Mac OS X audio output sub-system (CoreAudio)
  --with-audio=mint       Use MinT audio output sub-system (Atari)
  --with-audio=nas        Use NAS audio output (Network Audio System)
  --with-audio=os2        Use OS2 audio output sub-system
  --with-audio=oss        Use OSS audio output sub-system (/dev/dsp)
  --with-audio=portaudio  Use PortAudio audio output sub-system
  --with-audio=sdl        Use SDL audio output sub-system (Simple DirectMedia Layer)
  --with-audio=sgi        Use SGI audio output sub-system (IRIX)
  --with-audio=sun        Use Sun audio output sub-system (/dev/audio)
])

AC_ARG_WITH([optimization], [
  --with-optimization=0   No Optimization
  --with-optimization=1   Limited Optimization (-O)
  --with-optimization=2   Default Optimization (-O2)
  --with-optimization=3   More Optimize than default (-O3)
  --with-optimization=4   Optimize yet more (-O4)
])


dnl ############## Function Checks

AC_FUNC_MMAP

# Check for sched_setscheduler
AC_CHECK_FUNCS( sched_setscheduler )

# Check for setpriority
AC_CHECK_FUNCS( setpriority )



dnl ############## Library Checks

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([mx], [powf])

# Check for JACK
PKG_CHECK_MODULES(JACK, jack, HAVE_JACK=yes, HAVE_JACK=no)

# Check for EsounD
PKG_CHECK_MODULES(ESOUND, esound, HAVE_ESOUND=yes, HAVE_ESOUND=no)

# Check for SDL
PKG_CHECK_MODULES(SDL, sdl, HAVE_SDL=yes, HAVE_SDL=no)

# Check for NAS
AC_CHECK_LIB( [audio], [AuOpenServer],
	[ AC_CHECK_HEADER( [audio/audiolib.h],
		[ HAVE_NAS="yes" ],
		[ AC_MSG_WARN([Found ALSA library but NOT header files on your system]) ] )
	]
)

# Check for PortAudio
AC_CHECK_LIB( [portaudio], [Pa_Initialize],
	[ AC_CHECK_HEADER( [portaudio.h],
		[ HAVE_PORTAUDIO="yes" ],
		[ AC_MSG_WARN([Found PortAudio library but NOT header files on your system]) ] )
	]
)

# Check for ALSA
AC_CHECK_LIB( [asound], [snd_pcm_open],
	[ AC_CHECK_HEADER( [alsa/asoundlib.h],
		[ HAVE_ALSA="yes" ],
		[ AC_MSG_WARN([Found ALSA library but NOT header files on your system]) ] )
	]
)


dnl ############## Header Checks

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h sched.h sys/ioctl.h])


# OSS Header files
AC_CHECK_HEADERS([sys/soundcard.h linux/soundcard.h machine/soundcard.h])

# Sun Audio headers
AC_CHECK_HEADERS([sun/audioio.h sys/audioio.h asm/audioio.h sys/audio.h])

# Mac OS X
AC_CHECK_HEADERS([CoreAudio/CoreAudio.h], HAVE_COREAUDIO=yes, HAVE_COREAUDIO=no)




dnl ############## Choose compiler flags and CPU

CFLAGS="$CFLAGS -Wall"
LIBS="$LIBS"

# If debugging is enabled then make warnings errors
if test "$debugging" = "enabled"; then
	CFLAGS="$CFLAGS -g -Werror -pedantic"
fi


case $target in
  i386-*-linux*)
    cpu_type="i586"
    CFLAGS="$CFLAGS -DREAL_IS_FLOAT"
  ;;
  i486-*-linux*)
    cpu_type="i586"
    CFLAGS="$CFLAGS -DREAL_IS_FLOAT"
  ;;
  i586-*-linux*)
    cpu_type="i586"
    CFLAGS="$CFLAGS -DREAL_IS_FLOAT"
  ;;
  i686-*-linux*)
    cpu_type="mmx"
    CFLAGS="$CFLAGS -DREAL_IS_FLOAT"
  ;;
  *-*-linux*)
    cpu_type="generic"
    CFLAGS="$CFLAGS -DREAL_IS_FLOAT"
  ;;
  *-apple-darwin*)
    cpu_type="generic"
    CFLAGS="$CFLAGS -DREAL_IS_FLOAT"
  ;;
  *-*-freebsd*)
    cpu_type="generic"
    CFLAGS="$CFLAGS -DREAL_IS_FLOAT"
  ;;
  *-sun-solaris*)
    cpu_type="generic"
    CFLAGS="$CFLAGS -DREAL_IS_FLOAT"
    LIBS="$LIBS -lsocket -lnsl"
  ;;
  *)
  	AC_MSG_WARN([Unknown target operating system])
    cpu_type="generic"
    CFLAGS="$CFLAGS -DGENERIC"
  ;;
esac


dnl Did user choose other CPU type ?
if test "x$with_cpu" != "x"; then
	cpu_type=$with_cpu
fi


dnl CPU specific compiler flags
case $cpu_type in
  generic)
    CFLAGS="$CFLAGS"
  ;;
  altivec)
    CFLAGS="$CFLAGS -maltivec"
  ;;
  i386) 
    CFLAGS="$CFLAGS -march=i386 -DI386_ASSEM "
  ;;
  i486) 
    CFLAGS="$CFLAGS -march=i486 -DI386_ASSEM -DI486_OPT "
  ;;
  i586) 
    CFLAGS="$CFLAGS -DI386_ASSEM -DPENTIUM_OPT "
  ;;
  i586_dither) 
    CFLAGS="$CFLAGS -DI386_ASSEM -DPENTIUM_OPT"
  ;;
  3dnow) 
    CFLAGS="$CFLAGS -DUSE_3DNOW -DI386_ASSEM -DPENTIUM_OPT "
  ;;
  mmx) 
    CFLAGS="$CFLAGS -DUSE_MMX -DI386_ASSEM -DPENTIUM_OPT "
  ;;
  *)
  	AC_MSG_ERROR([Unknown CPU type '$cpu_type'])
  ;;
esac

CPU_TYPE_LIB="libcpu_$cpu_type.a"
AC_SUBST( CPU_TYPE_LIB )




dnl ############## Compiler Optimizations

# None chosen?
if test "x$with_optimization" == "x"; then
	if test "$debugging" = "enabled"; then
		with_optimization="0"
	else
		with_optimization="2"
	fi
fi

case $with_optimization in
  0)
    # No Optimizations
    CFLAGS="$CFLAGS"
  ;;
  1)
    CFLAGS="$CFLAGS -O"
  ;;
  2)
    CFLAGS="$CFLAGS -O2"
    CFLAGS="$CFLAGS -fomit-frame-pointer -funroll-all-loops"
    CFLAGS="$CFLAGS -finline-functions -ffast-math"
  ;;
  3)
    CFLAGS="$CFLAGS -O3"
    CFLAGS="$CFLAGS -fomit-frame-pointer -funroll-all-loops"
    CFLAGS="$CFLAGS -finline-functions -ffast-math"
  ;;
  4)
    CFLAGS="$CFLAGS -O4"
    CFLAGS="$CFLAGS -fomit-frame-pointer -funroll-all-loops"
    CFLAGS="$CFLAGS -finline-functions -ffast-math"
  ;;
  *)
  	AC_MSG_ERROR([Unknown optimizations level '$with_optimization'])
  ;;
esac




dnl ############## Audio Sub-system Settings

# Choose audio subsystem based on Operating System
case $target in
  *-*-linux*)
    if test "x$HAVE_ALSA" = "xyes" ; then
        audio_system="alsa"
    else
        audio_system="oss"
    fi
  ;;
  *-*-freebsd*)
    audio_system="oss"
  ;;
  *-apple-darwin*)
    audio_system="macosx"
  ;;
  *-*-sun*)
    audio_system="sun"
  ;;
  *)
  	audio_system="dummy"
  ;;
esac

# Did user choose other audio type ?
if test "x$with_audio" != "x"; then
	audio_system=$with_audio
fi



AUDIO_OBJ="audio_$audio_system.\$(OBJEXT)"
AUDIO_CFLAGS=""
AUDIO_LIBS=""

case $audio_system in
  aix)	
    # Currently unsupported in the new build system
  ;;
  alib)
    # Currently unsupported in the new build system
    AUDIO_CFLAGS="-I/opt/audio/include"
  ;;
  alsa)
    if test "x$HAVE_ALSA" != "xyes" ; then
  		AC_MSG_ERROR( ALSA is not available for audio output subsystem. )
    fi
    AUDIO_CFLAGS="-lasound"
    AUDIO_LIBS=""
  ;;
  dummy) 
    AC_MSG_WARN( Using dummy audio output. )
  ;;
  esd)
  	if test "x$HAVE_ESD" != "xyes" ; then
  		AC_MSG_ERROR( ESounD is not available for audio output subsystem. )
  	fi
    AUDIO_CFLAGS=$ESOUND_CFLAGS
    AUDIO_LIBS=$ESOUND_LIBS
  ;;
  hp)
    # Currently unsupported in the new build system
  	AUDIO_CFLAGS="-I/opt/audio/include"
  	AUDIO_LIBS="-L/opt/audio/lib -lAlib"
  ;;
  jack)
  	if test "x$HAVE_JACK" != "xyes" ; then
  		AC_MSG_ERROR( JACK is not available for audio output subsystem. )
  	fi
    AUDIO_CFLAGS=$JACK_CFLAGS
    AUDIO_LIBS=$JACK_LIBS
  ;;
  macosx)
  	if test "x$HAVE_COREAUDIO" != "xyes" ; then
  		AC_MSG_ERROR( Can't find Mac OS X's CoreAudio header files )
  	fi
  	AUDIO_LIBS="-framework CoreAudio"
  ;;
  mint)
    # Currently unsupported in the new build system
  ;;
  nas)
  	if test "x$HAVE_NAS" != "xyes" ; then
  		AC_MSG_ERROR( NAS (Network Audio System) is not available. )
  	fi
    AUDIO_CFLAGS=""
    AUDIO_LIBS="-laudio"
  ;;
  os2)
    # Currently unsupported in the new build system
  ;;
  oss)
    if test "x${ac_cv_header_sys_soundcard_h}" = "xyes" \
         -o "x${ac_cv_header_linux_soundcard_h}" = "xyes" \
         -o "x${ac_cv_header_machine_soundcard_h}" = "xyes";
    then
		AUDIO_CFLAGS=""
		AUDIO_LIBS=""
	else 
		AC_MSG_ERROR([Can't find OSS header files])
	fi
  ;;
  portaudio)
  	if test "x$HAVE_PORTAUDIO" != "xyes" ; then
  		AC_MSG_ERROR( PortAudio is not available. )
  	fi
    AUDIO_CFLAGS=""
    AUDIO_LIBS="-lportaudio"
  ;;
  sdl)
  	if test "x$HAVE_SDL" != "xyes" ; then
  		AC_MSG_ERROR( SDL is not available for audio output subsystem. )
  	fi
    AUDIO_CFLAGS=$SDL_CFLAGS
    AUDIO_LIBS=$SDL_LIBS
  ;;
  sgi)
    # Currently unsupported in the new build system
  ;;
  sun)
    if test "x${ac_cv_header_sun_audioio_h}" = "xyes" \
         -o "x${ac_cv_header_sys_audioio_h}" = "xyes" \
         -o "x${ac_cv_header_asm_audioio_h}" = "xyes";
    then
		AUDIO_CFLAGS=""
		AUDIO_LIBS=""
	else 
		AC_MSG_ERROR([Can't find Sun Audio header files])
	fi
  ;;
  *)
  	AC_MSG_ERROR([Unknown Audio sub-system '$audio_system'.])
  ;;
esac

AC_SUBST(AUDIO_OBJ)
AC_SUBST(AUDIO_CFLAGS)
AC_SUBST(AUDIO_LIBS)



dnl ############## Final Output

AC_OUTPUT([Makefile src/Makefile])



dnl ############## Display Message

echo "
  $PACKAGE $VERSION

  Install path ............ $prefix
  CPU Optimisation......... $cpu_type
  Compiler Optimization ... $with_optimization
  Audio output ............ $audio_system
  Gapless Support ......... $gapless
  Debugging ............... $debugging
"
  
if test "$debugging" = "enabled"; then
	echo "CFLAGS='$CFLAGS'"
	echo "LIBS='$LIBS'"
	echo "AUDIO_CFLAGS='$AUDIO_CFLAGS'"
	echo "AUDIO_LIBS='$AUDIO_LIBS'"
else
    echo "Next type 'make' and then 'make install'."
fi


