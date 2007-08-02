dnl configure.ac: source for the configure script

dnl copyright by the mpg123 project - free software under the terms of the LGPL 2.1
dnl see COPYING and AUTHORS files in distribution or http://mpg123.org
dnl initially written by Nicholas J. Humfrey

dnl Require autoconf version >= 2.57
AC_PREREQ(2.57)


dnl ############# Initialisation

AC_INIT([mpg123], [0.67], [mpg123-devel@lists.sourceforge.net])

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
              [
                if test "x$enableval" = xyes
                then
                  debugging="enabled"
                  AC_DEFINE(DEBUG, 1, [ Define if debugging is enabled. ])
                else
                  debugging="disabled"
                fi
              ],
              [ debugging="disabled" ]
)

AC_ARG_ENABLE(gapless,
              [  --enable-gapless=[no/yes] turn on gapless (enabled per default)],
              [
                if test "x$enableval" = xyes
                then
                  gapless="enabled"
                  AC_DEFINE(GAPLESS, 1, [ Define if gapless is enabled. ])
                else
                  gapless="disabled"
                fi
              ],
              [
                gapless="enabled"
                AC_DEFINE(GAPLESS, 1, [ Define if gapless is enabled. ])
              ]
)

AC_ARG_ENABLE(fifo,
              [  --enable-fifo=[no/yes] FIFO support for control interface (auto-enabled on linux) ],
              [
                if test "x$enableval" = xyes
                then
                  fifo="enabled"
                  AC_DEFINE(FIFO, 1, [ Define if FIFO support is enabled. ])
                else
                  fifo="disabled"
                fi
              ],
              [
                fifo="auto"
              ]
)

AC_ARG_WITH([cpu], [
  --with-cpu=generic[_fpu]      Use generic processor code with floating point arithmetic
  --with-cpu=generic_float      A special build with generic fpu code that produces 32bit float output
  --with-cpu=generic_nofpu      Use generic processor code with fixed point arithmetic (p.ex. ARM, experimental)
  --with-cpu=i386[_fpu]         Use code optimized for i386 processors with floating point arithmetic
  --with-cpu=i386_nofpu         Use code optimized for i386 processors with fixed point arithmetic (experimental)
  --with-cpu=i486         Use code optimized for i486 processors
  --with-cpu=i586         Use code optimized for i586 processors
  --with-cpu=i586_dither  Use code optimized for i586 processors with dithering (noise shaping), adds 256K to binary size
  --with-cpu=3dnow        Use code optimized for 3DNow processors
  --with-cpu=3dnowext     Use code optimized for 3DNowExt processors (K6-3+, Athlon)
  --with-cpu=mmx          Use code optimized for MMX processors
  --with-cpu=sse          Use code optimized for SSE processors
  --with-cpu=x86          Pack all x86 opts into one binary (excluding i486 and dithered i586)
  --with-cpu=x86_dither   Pack all x86 opts into one binary (excluding i486, including dither)
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
  --with-audio=win32      Use Windows audio output (WinMM)
])

AC_ARG_WITH([optimization], [
  --with-optimization=0   No Optimization
  --with-optimization=1   Limited Optimization (-O) (for gcc)
  --with-optimization=2   Default Optimization (-O2 ...) (for gcc)
  --with-optimization=3   More Optimize than default (-O3 ...) (for gcc)
  --with-optimization=4   Optimize yet more (-O4 ...) (for gcc)
])

AC_ARG_WITH([seektable], [
  --with-seektable=<size> choose size of seek index table (0 disables it), default 1000
])

dnl ############## Assembler, compiler properties

# based on posting from John Dalgliesh <johnd@defyne.org> on ffmpeg (LGPL) mailing list
# find if .align arg is power-of-two or not
asmalign_exp="unknown"
if test $asmalign_exp = "unknown"; then
	AC_MSG_CHECKING([if .align takes 2-exponent])
	asmalign_exp="no"
	echo '.align 3' > conftest.s
	if $CCAS -c -o conftest.o conftest.s; then
		asmalign_exp="yes"
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi
	rm -f conftest.o conftest.s
fi
if test $asmalign_exp = "yes"; then
	AC_DEFINE(ASMALIGN_EXP, 1, [ Define if .align takes 3 for alignment of 2^3=8 bytes instead of 8. ])
fi

ccalign="unknown"
if test $ccalign = "unknown"; then
	AC_MSG_CHECKING([__attribute__((aligned(16)))])
	ccalign="no"
	echo '__attribute__((aligned(16))) float var;' > conftest.c
	if $CC -c -o conftest.o conftest.c; then
		ccalign="yes"
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi
	rm -f conftest.o conftest.c
fi
if test $ccalign = "yes"; then
	AC_DEFINE(CCALIGN, 1, [ Define if __attribute__((aligned(16))) works ])
fi

dnl ############## Types

AC_TYPE_SIZE_T()
AC_TYPE_SSIZE_T()
AC_TYPE_OFF_T()

dnl ############## Function Checks

AC_FUNC_MMAP

# Check for sched_setscheduler
AC_CHECK_FUNCS( sched_setscheduler setuid getuid)

# Check for setpriority
AC_CHECK_FUNCS( setpriority )

AC_CHECK_FUNCS( strerror )

AC_CHECK_FUNCS( mkfifo, [ have_mkfifo=yes ], [ have_mkfifo=no ] )

if test $fifo = "auto"; then
	if test $have_mkfifo = "yes"; then
		fifo=yes
		AC_DEFINE(FIFO, 1, [ Define if FIFO support is enabled. ])
	else
		fifo=no
	fi
fi
if test $fifo = "yes" && test $have_mkfifo = "no"; then
	AC_MSG_WARN( [ You forced FIFO code while I think there is no mkfifo() available! ] )
fi

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
		[ AC_MSG_WARN([Found NAS library but NOT header files on your system]) ] )
	]
)

# Check for PortAudio
case $target in
	*-*-mingw32)
		# We tested portaudio with MME
		if test "x$HAVE_PORTAUDIO" != "xyes"; then
			AC_CHECK_LIB( [portaudio], [Pa_Initialize],
				[ AC_CHECK_HEADER( [portaudio.h],
					[ HAVE_PORTAUDIO_WINMM="yes" && HAVE_PORTAUDIO="yes" ],
					[ AC_MSG_WARN([Found PortAudio library but NOT header files on your system]) ] )
				],
				[ HAVE_PORTAUDIO="no" ],
				[ -lwinmm ]
			)
		fi
	;;
	*)
		AC_CHECK_LIB( [portaudio], [Pa_Initialize],
			[ AC_CHECK_HEADER( [portaudio.h],
				[ HAVE_PORTAUDIO="yes" ],
				[ AC_MSG_WARN([Found PortAudio library but NOT header files on your system]) ] )
			]
		)
	;;
esac

# Check for windows ... and win32 audio
# Does not work... instead just check for header
# AC_CHECK_LIB( [winmm], [waveOutOpen] )
AC_CHECK_HEADERS([windows.h], HAVE_WINMM=yes, HAVE_WINMM=no)

# Check for ALSA
AC_CHECK_LIB( [asound], [snd_pcm_open],
	[ AC_CHECK_HEADER( [alsa/asoundlib.h],
		[ HAVE_ALSA="yes" ],
		[ AC_MSG_WARN([Found ALSA library but NOT header files on your system]) ] )
	]
)

dnl ############## Header Checks

AC_HEADER_STDC
dnl Is it too paranoid to specifically check for stdint.h and limits.h?
AC_CHECK_HEADERS([stdlib.h string.h unistd.h sched.h sys/ioctl.h stdint.h limits.h inttypes.h sys/time.h sys/wait.h sys/resource.h signal.h])

# attempt to make the signal stuff work... also with GENERIC - later
#if test $ac_cv_header_sys_signal_h = yes; then
#	AC_CHECK_FUNCS( sigemptyset sigaddset sigprocmask sigaction )
#	if test $ac_cv_func_sigemptyset = yes &&
#	   test $ac_cv_func_sigaddset = yes &&
#	   test $ac_cv_func_sigprocmask = yes &&
#	   test $ac_cv_func_sigaction = yes; then
#	AC_DEFINE( 
#fi

# OSS Header files: One of them is enough.
AC_CHECK_HEADERS(sys/soundcard.h)
AC_CHECK_HEADERS(linux/soundcard.h)
AC_CHECK_HEADERS(machine/soundcard.h)
if test "x${ac_cv_header_sys_soundcard_h}" = "xyes" ||
   test "x${ac_cv_header_linux_soundcard_h}" = "xyes" ||
   test "x${ac_cv_header_machine_soundcard_h}" = "xyes"; then
	HAVE_OSS=yes
else
	HAVE_OSS=no
fi

# Sun Audio headers
AC_CHECK_HEADERS([sun/audioio.h sys/audioio.h asm/audioio.h sys/audio.h], HAVE_SUNAUDIO=yes, HAVE_SUNAUDIO=no)

# Mac OS X
AC_CHECK_HEADERS([AudioUnit/AudioUnit.h CoreServices/CoreServices.h AudioToolbox/AudioToolbox.h],
					HAVE_MACOSXAUDIO=yes, HAVE_MACOSXAUDIO=no)




dnl ############## Choose compiler flags and CPU

# do not assume gcc here, so no flags by default
CFLAGS="$CFLAGS"
LIBS="$LIBS"

# If debugging is enabled then make warnings errors
if test "$debugging" = "enabled"; then
	CFLAGS="$CFLAGS -g"
	# gcc specific...
	if test "$GCC" = "yes"; then
		CFLAGS="$CFLAGS -Wall -Werror -pedantic -fno-strict-aliasing"
	fi
fi

case $target in
  arm-*-linux*)
    # check that... perhaps we are better off on arm with kernel math emulation
    cpu_type="generic_nofpu"
  ;;
  i386-*-linux*|i386-*-kfreebsd*-gnu)
    cpu_type="i386_fpu"
  ;;
  i486-*-linux*|i486-*-kfreebsd*-gnu)
    cpu_type="i486"
  ;;
  i586-*-linux*|i586-*-kfreebsd*-gnu)
    cpu_type="x86"
  ;;
  i686-*-linux*|i686-*-kfreebsd*-gnu)
    cpu_type="x86"
  ;;
  *-*-linux*|*-*-kfreebsd*-gnu)
    cpu_type="generic_fpu"
  ;;
  i686-apple-darwin*)
    cpu_type="x86"
  ;;
  *-apple-darwin*)
    AC_MSG_CHECKING([if CPU type supports AltiVec])
    case `machine` in
      ppc7400 | ppc7450 | ppc970)
        AC_MSG_RESULT([yes])
        cpu_type="altivec"
      ;;
      *)
        AC_MSG_RESULT([no])
        cpu_type="generic_fpu"
      ;;
    esac
  ;;
  i386-*-freebsd*)
    cpu_type="i386_fpu"
  ;;
  *-*-freebsd*)
    cpu_type="generic_fpu"
  ;;
  *-*-solaris*)
    cpu_type="generic_fpu"
    LIBS="$LIBS -lsocket -lnsl"
  ;;
  *-dec-osf*)
    cpu_type="generic_fpu"
  ;;
  i686-pc-cygwin*)
    cpu_type="x86"
  ;;
  i586-pc-cygwin*)
    cpu_type="x86"
  ;;
  i486-pc-cygwin*)
    cpu_type="i486"
  ;;
  i386-pc-cygwin*)
    cpu_type="i386"
  ;;
  *-pc-cygwin*)
    cpu_type="generic_fpu"
  ;;
  *-*-mingw32)
    LIBS="-lwsock32 $LIBS"
    CPPFLAGS="-DNOXFERMEM $CPPFLAGS"
    cpu_type="x86"
  ;;
	i386-*)
		AC_MSG_WARN([Unknown target operating system])
		cpu_type="i386"
		CFLAGS="-DGENERIC -DNOXFERMEM $CFLAGS"
	;;
	i486-*)
		AC_MSG_WARN([Unknown target operating system])
		cpu_type="i486"
		CFLAGS="-DGENERIC -DNOXFERMEM $CFLAGS"
	;;
	i586-*)
		AC_MSG_WARN([Unknown target operating system])
		cpu_type="i586"
		CFLAGS="-DGENERIC -DNOXFERMEM $CFLAGS"
	;;
	i686-*)
		AC_MSG_WARN([Unknown target operating system])
		cpu_type="x86"
		CFLAGS="-DGENERIC -DNOXFERMEM $CFLAGS"
	;;
  *)
  	AC_MSG_WARN([Unknown target operating system])
    cpu_type="generic_fpu"
    CFLAGS="-DGENERIC -DNOXFERMEM $CFLAGS"
  ;;
esac


dnl Did user choose other CPU type ?
if test "x$with_cpu" != "x"; then
	cpu_type=$with_cpu
fi


dnl CPU specific compiler flags
case $cpu_type in
  generic)
    CFLAGS="$CFLAGS -DOPT_GENERIC -DREAL_IS_FLOAT"
  ;;
  generic_float)
    CFLAGS="$CFLAGS -DOPT_GENERIC -DFLOATOUT -DNOXFERMEM -DREAL_IS_FLOAT"
  ;;
  generic_fpu)
    CFLAGS="$CFLAGS -DOPT_GENERIC -DREAL_IS_FLOAT"
  ;;
  generic_nofpu)
    CFLAGS="$CFLAGS -DOPT_GENERIC -DREAL_IS_FIXED"
  ;;
  altivec)
    CFLAGS="$CFLAGS -maltivec -faltivec -DREAL_IS_FLOAT -DOPT_ALTIVEC"
  ;;
  i386) 
    CFLAGS="$CFLAGS -DOPT_I386 -DREAL_IS_FLOAT"
  ;;
  i386_fpu) 
    CFLAGS="$CFLAGS -DOPT_I386 -DREAL_IS_FLOAT"
  ;;
  i386_nofpu) 
    CFLAGS="$CFLAGS -DOPT_I386 -DREAL_IS_FIXED"
  ;;
  i486) 
    CFLAGS="$CFLAGS -DOPT_I486 -DREAL_IS_FLOAT"
  ;;
  i586) 
    CFLAGS="$CFLAGS -DOPT_I586 -DREAL_IS_FLOAT"
  ;;
  i586_dither) 
    CFLAGS="$CFLAGS -DOPT_I586_DITHER -DREAL_IS_FLOAT"
  ;;
  3dnow)
    # legacy 3dnow had the 3dnow optional... keeping that
    CFLAGS="$CFLAGS -DOPT_MULTI -DOPT_I586 -DOPT_3DNOW -DREAL_IS_FLOAT"
  ;;
  3dnowext) 
    CFLAGS="$CFLAGS -DOPT_3DNOWEXT -DREAL_IS_FLOAT"
  ;;
  mmx) 
    CFLAGS="$CFLAGS -DOPT_MMX -DREAL_IS_FLOAT"
  ;;
  sse) 
    CFLAGS="$CFLAGS -DOPT_SSE -DREAL_IS_FLOAT"
  ;;
  x86) 
    CFLAGS="$CFLAGS -DOPT_MULTI -DOPT_GENERIC -DOPT_I386 -DOPT_I586 -DOPT_MMX -DOPT_3DNOW -DOPT_3DNOWEXT -DOPT_SSE -DREAL_IS_FLOAT"
  ;;
  x86_dither) 
    CFLAGS="$CFLAGS -DOPT_MULTI -DOPT_GENERIC -DOPT_I386 -DOPT_I586 -DOPT_I586_DITHER -DOPT_MMX -DOPT_3DNOW -DOPT_3DNOWEXT -DOPT_SSE -DREAL_IS_FLOAT"
  ;;
  *)
  	AC_MSG_ERROR([Unknown CPU type '$cpu_type'])
  ;;
esac

if test $cpu_type = "generic_float"; then
	CPU_TYPE_LIB="libcpu_generic.a"
else
	CPU_TYPE_LIB="libcpu_$cpu_type.a"
fi
AC_SUBST( CPU_TYPE_LIB )




dnl ############## Compiler Optimizations

# None chosen?
if test "x$with_optimization" = "x"; then
	if test "$debugging" = "enabled"; then
		with_optimization="0"
	else
		# enable (gcc specific) default opts only with gcc
		if test "$GCC" = yes; then
			with_optimization="2"
		else
			with_optimization="0"
		fi
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

dnl ############## Seektable size

if test "x$with_seektable" = "x"; then
	seektable=1000
else
	seektable=$with_seektable
fi

AC_DEFINE_UNQUOTED(INDEX_SIZE, $seektable, [size of the frame index seek table])

dnl ############## Audio Sub-system Settings

# Choose audio subsystem based on what we have.
if test $cpu_type = "generic_float"; then
	audio_system="dummy"
	AC_MSG_WARN( Defaulting to dummy output with float samples, as the audio device system cannot handle floats. )
elif   test "x$HAVE_ALSA" = "xyes"; then
	audio_system="alsa"
elif test "x$HAVE_OSS" = "xyes"; then
	audio_system="oss"
elif test "x$HAVE_MACOSXAUDIO" = "xyes"; then
	audio_system="coreaudio"
elif test "x$HAVE_SUNAUDIO" = "xyes"; then
	audio_system="sun"
elif test "x$HAVE_WINMM" = "xyes"; then
	audio_system="win32"
elif test "x$HAVE_JACK" = "xyes"; then
	audio_system="jack"
elif test "x$HAVE_PORTAUDIO" = "xyes"; then
	audio_system="portaudio"
elif test "x$HAVE_NAS" = "xyes"; then
	audio_system="nas"
elif test "x$HAVE_SDL" = "xyes"; then
	audio_system="sdl"
elif test "x$HAVE_ESOUND" = "xyes"; then
	audio_system="esd"
else
	audio_system="dummy"
fi


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
    AUDIO_CFLAGS=""
    AUDIO_LIBS="-lasound"
  ;;
  dummy) 
    AC_MSG_WARN( Using dummy audio output. )
  ;;
  esd)
  	if test "x$HAVE_ESOUND" != "xyes" ; then
  		AC_MSG_WARN( ESounD is not available for audio output subsystem. )
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
  coreaudio|macosx)
  	if test "x$HAVE_MACOSXAUDIO" != "xyes" ; then
  		AC_MSG_ERROR( Can't find Mac OS X's audio header files )
  	fi
  	AUDIO_LIBS="-framework AudioToolbox -framework AudioUnit -framework CoreServices"
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
    if test "x$HAVE_OSS" = "xyes"
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
		if test "x$HAVE_PORTAUDIO_WINMM" = "xyes"; then
			AUDIO_LIBS="$AUDIO_LIBS -lwinmm"
		fi
  ;;
  sdl)
		if test "x$HAVE_SDL" != "xyes" ; then
			AC_MSG_WARN( SDL not found via pkgconfig. Make sure you add some CPP/CFLAGS and LDFLAGS. )
		fi
		AUDIO_CFLAGS=$SDL_CFLAGS
		AUDIO_LIBS=$SDL_LIBS
  ;;
  sgi)
  # experimenting
    AUDIO_CFLAGS=-DSGI
    AUDIO_LIBS=-laudio
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
	win32)
		AUDIO_LIBS=-lwinmm
		AUDIO_CFLAGS=
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

echo -n "
  $PACKAGE $VERSION

  Install path ............ $prefix
  CPU Optimisation......... $cpu_type
  Compiler Optimization ... $with_optimization
  Audio output ............ $audio_system (detected:"
test "x$HAVE_ALSA"        = "xyes" && echo -n " alsa"
test "x$HAVE_OSS"         = "xyes" && echo -n " oss"
test "x$HAVE_MACOSXAUDIO" = "xyes" && echo -n " coreaudio"
test "x$HAVE_SUNAUDIO"    = "xyes" && echo -n " sun"
test "x$HAVE_JACK"        = "xyes" && echo -n " jack"
test "x$HAVE_PORTAUDIO"   = "xyes" && echo -n " portaudio"
test "x$HAVE_NAS"         = "xyes" && echo -n " nas"
test "x$HAVE_SDL"         = "xyes" && echo -n " sdl"
test "x$HAVE_ESOUND"      = "xyes" && echo -n " esd"
test "x$HAVE_WINMM"       = "xyes" && echo -n " win32"
echo ")
  Gapless Support ......... $gapless
  Debugging ............... $debugging
  Seek table size ......... $seektable
  FIFO support ............ $fifo
"
if test $cpu_type = "generic_float"; then
	echo "You chose the generic build with 32bit float output.
That means output via -s, -O or -w to a file/pipe and no buffer.
"
fi
if test $with_optimization = "0"; then
	echo "No optimization flags chosen, make sure you have something basic in your CFLAGS at least...
"
fi
if test "$debugging" = "enabled"; then
	echo "CFLAGS='$CFLAGS'"
	echo "LIBS='$LIBS'"
	echo "AUDIO_CFLAGS='$AUDIO_CFLAGS'"
	echo "AUDIO_LIBS='$AUDIO_LIBS'"
else
    echo "Next type 'make' and then 'make install'."
fi


