
dnl Require autoconf version >= 2.57
AC_PREREQ(2.57)


dnl ############# Initialisation

AC_INIT([mpg123], [0.60-devel], [mpg123-devel@lists.sourceforge.net])

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
              [--enable-debug=[no/yes] turn on debugging],
              [ debugging="enabled" 
                AC_DEFINE(DEBUG, 1, [ Define if debugging is enabled. ]) ],
              [ debugging="disabled" ]
)

AC_ARG_WITH([cpu], [
  --with-cpu=generic      Use generic processor code
  --with-cpu=i386         Use code optimized for i386 processors
  --with-cpu=i486         Use code optimized for i486 processors
  --with-cpu=i586         Use code optimized for i586 processors
  --with-cpu=3dnow        Use code optimized for 3dnow processors
]) 

AC_ARG_WITH([audio], [
  --with-audio=aix        Use AIX audio output sub-system
  --with-audio=alib       Use Alib audio output sub-system (for HPUX)
  --with-audio=alsa       Use ALSA audio output sub-system
  --with-audio=dummy      Use dummy audio (when no sound card is available)
  --with-audio=esd        Use ESoundD audio output sub-system
  --with-audio=hp         Use HP audio output sub-system
  --with-audio=jack       Use JACK low-latency audio server
  --with-audio=libao      Use libao audio output sub-system (Audio Out)
  --with-audio=macosx     Use Mac OS X audio output sub-system (CoreAudio)
  --with-audio=mint       Use MinT audio output sub-system (Atari)
  --with-audio=nas        Use NAS audio output (Network Audio System)
  --with-audio=os2        Use OS2 audio output sub-system
  --with-audio=oss        Use OSS audio output sub-system
  --with-audio=sgi        Use SGI audio output sub-system (IRIX)
  --with-audio=sun        Use Sun audio output sub-system (/dev/audio)
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

# Check for ALSA - Advanced Linux Sound Architecture
PKG_CHECK_MODULES(ALSA, alsa, HAVE_ALSA=yes, HAVE_ALSA=no)

# Check for JACK
PKG_CHECK_MODULES(JACK, jack, HAVE_JACK=yes, HAVE_JACK=no)

# Check for libao (Audio Out)
PKG_CHECK_MODULES(LIBAO, ao, HAVE_LIBAO=yes, HAVE_LIBAO=no)

# Check for EsounD
PKG_CHECK_MODULES(ESOUND, esound, HAVE_ESOUND=yes, HAVE_ESOUND=no)



dnl ############## Header Checks

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h sched.h])


# NAS (Network Audio System)
AC_CHECK_HEADERS([audio/audiolib.h audio/soundlib.h])

# OSS Header files
AC_CHECK_HEADERS([sys/soundcard.h machine/soundcard.h sys/soundcard.h])

# Mac OS X
AC_CHECK_HEADERS([CoreAudio/CoreAudio.h], HAVE_COREAUDIO=yes, HAVE_COREAUDIO=no)




dnl ############## Choose compiler flags and CPU

CFLAGS="$CFLAGS -DHAVE_CONFIG_H -Wall -std=c99"


# If debugging is enabled then make warnings errors
if test "$Debugging" = "enabled"; then
	CFLAGS="$CFLAGS -g -Werror"
fi


case $target in
  powerpc-apple-darwin*)
    audio_system="macosx"
    cpu_type="generic"
    CFLAGS="$CFLAGS -O2 -DREAL_IS_FLOAT"
  ;;
  *-*-freebsd*)
    audio_system="oss"
    cpu_type="generic"
    CFLAGS="$CFLAGS -O4 -funroll-all-loops -ffast-math -DREAL_IS_FLOAT"
  ;;
  *)
  	AC_MSG_WARN("Unknown target operating system; using dummy audio output")
  	audio_system="dummy"
    cpu_type="generic"
    CFLAGS="$CFLAGS -O -DGENERIC"
  ;;
esac


# Did user choose other CPU type ?
if test "x$with_cpu" != "x"; then
	cpu_type=$with_cpu
fi
CPU_TYPE_LIB="libcpu_$cpu_type.a"
AC_SUBST( CPU_TYPE_LIB )



dnl ############## Audio Sub-system Settings


# Did user choose other audio type ?
if test "x$with_audio" != "x"; then
	audio_system=$with_audio
fi

AUDIO_OBJ="audio_$audio_system.\$(OBJEXT)"
AUDIO_CFLAGS=''
AUDIO_LIBS=''

case $audio_system in
  aix)	;;
  alib) AUDIO_CFLAGS='-I/opt/audio/include' ;;
  alsa)
  	if test "$HAVE_ALSA" = "no" ; then
  		AC_MSG_ERROR( $ALSA_PKG_ERRORS )
  	fi
    AUDIO_CFLAGS=$ALSA_CFLAGS
    AUDIO_LIBS=$ALSA_LIBS
  ;;
  dummy) ;;
  esd)
  	if test "$HAVE_ESD" = "no" ; then
  		AC_MSG_ERROR( ESounD is not available for audio output subsystem. )
  	fi
    AUDIO_CFLAGS=$ESOUND_CFLAGS
    AUDIO_LIBS=$ESOUND_LIBS
  ;;
  hp)
  	AUDIO_CFLAGS="-I/opt/audio/include"
  	AUDIO_LIBS="-L/opt/audio/lib -lAlib"
  ;;
  jack)
  	if test "$HAVE_JACK" = "no" ; then
  		AC_MSG_ERROR( JACK is not available for audio output subsystem. )
  	fi
    AUDIO_CFLAGS=$JACK_CFLAGS
    AUDIO_LIBS=$JACK_LIBS
  ;;
  libao)
  	if test "$HAVE_LIBAO" = "no" ; then
  		AC_MSG_ERROR( libao is not available for audio output subsystem. )
  	fi
    AUDIO_CFLAGS=$LIBAO_CFLAGS
    AUDIO_LIBS=$LIBAO_LIBS
  ;;
  macosx)
  	if test "$HAVE_COREAUDIO" = "no" ; then
  		AC_MSG_ERROR( Can't find Mac OS X's CoreAudio header files )
  	fi
  	AUDIO_LIBS='-framework CoreAudio'
  ;;
  mint) ;;
  nas) ;;
  os2) ;;
  oss)
dnl #  	if test "${ac_cv_header_sys_soundcard_h}" = "yes"; then
dnl #	fi  
    AUDIO_CFLAGS=''
    AUDIO_LIBS=''
  ;;
  sgi)  ;;
  sun)  ;;
  *)
  	AC_MSG_ERROR("Unknown Audio sub-system $(audio_system)")
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
  Audio output ............ $audio_system
  Debugging ............... $debugging
  
Next type 'make' and then 'make install'."

