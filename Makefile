###
###   mpg123  Makefile
###

# Where to install binary and manpage on "make install":

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/man
SECTION=1

###################################################
######                                       ######
######   End of user-configurable settings   ######
######                                       ######
###################################################

nothing-specified:
	@echo ""
	@echo "You must specify the system which you want to compile for:"
	@echo ""
	@echo "make linux-help     Linux, more help"
	@echo "make freebsd-help   FreeBSD more help"
	@echo "make solaris        Solaris 2.x (tested: 2.5 and 2.5.1) using SparcWorks cc"
	@echo "make solaris-gcc    Solaris 2.x using GNU cc (somewhat slower)"
	@echo "make solaris-gcc-esd  Solaris 2.x using gnu cc and Esound as audio output"
	@echo "make solaris-x86-gcc-oss Solaris with (commercial) OSS"
	@echo "make solaris-gcc-nas Solaris with gcc and NAS"
	@echo "make sunos          SunOS 4.x (tested: 4.1.4)"
	@echo "make hpux           HP/UX 9/10, /7xx"
	@echo "make hpux-gcc       HP/UX 9/10, /7xx using GCC cc"
	@echo "make hpux-alib      HP/UX with ALIB audio"
	@echo "make sgi            SGI running IRIX"
	@echo "make sgi-gcc        SGI running IRIX using GCC cc"
	@echo "make dec            DEC Unix (tested: 3.2 and 4.0), OSF/1"
	@echo "make ultrix         DEC Ultrix (tested: 4.4)"
	@echo "make aix-gcc        IBM AIX using gcc (tested: 4.2)"
	@echo "make aix-xlc        IBM AIX using xlc (tested: 4.3)"
	@echo "make aix-tk3play    IBM AIX"
	@echo "make os2            IBM OS/2"
	@echo "make netbsd         NetBSD"
	@echo "make bsdos          BSDI BSD/OS"
	@echo "make bsdos4         BSDI BSD/OS 4.0"
	@echo "make bsdos-nas      BSDI BSD/OS with NAS support"
	@echo "make mint           MiNT on Atari"
	@echo "make generic        try this one if your system isn't listed above"
	@echo ""
	@echo "Please read the file INSTALL for additional information."
	@echo ""

linux-help:
	@echo ""
	@echo "There are several Linux flavours. Choose one:"
	@echo ""
	@echo "make linux          Linux (i386, Pentium or unlisted platform)"
	@echo "make linux-i486     Linux (optimized for i486 ONLY)"
	@echo "make linux-3dnow    Linux, output 3DNow!(TM) optimized code"
	@echo "                    (ie with 'as' from binutils-2.9.1.0.19a or later)"
	@echo "make linux-alpha    make with minor changes for ALPHA-Linux"
	@echo "make linux-ppc      LinuxPPC or MkLinux for the PowerPC"
	@echo "make linux-m68k     Linux/m68k (Amiga, Atari) using OSS"
	@echo "make linux-nas      Linux, output to Network Audio System"
	@echo "make linux-sparc    Linux/Sparc"
	@echo "make linux-sajber   Linux, build binary for Sajber Jukebox frontend"
	@echo "make linux-alsa     Linux with ALSA sound driver"
	@echo "make linux-mips-alsa Linux/MIPS with ALSA sound driver"
	@echo ""
	@echo "make linux-esd      Linux, output to EsounD"
	@echo "make linux-alpha-esd Linux/Alpha, output to EsounD"
	@echo "make linux-ppc-esd  Linux/PPC, output to EsounD"
	@echo "    NOTE: esd flavours require libaudiofile, available from: "
	@echo "          http://www.68k.org/~michael/audiofile/"
	@echo ""
	@echo "Please read the file INSTALL for additional information."
	@echo ""

freebsd-help:
	@echo ""
	@echo "There are several FreeBSD flavours. Choose one:"
	@echo ""
	@echo "make freebsd         FreeBSD"
	@echo "make freebsd-sajber  FreeBSD, build binary for Sajber Jukebox frontend"
	@echo "make freebsd-tk3play FreeBSD, build binary for tk3play frontend"
	@echo "make freebsd-esd     FreeBSD, output to EsounD"
	@echo ""
	@echo "Please read the file INSTALL for additional information."
	@echo ""

linux-devel:
	$(MAKE) OBJECTS='decode_i386.o dct64_i386.o audio_oss.o' \
        CC=gcc LDFLAGS= \
        CFLAGS='-DREAL_IS_FLOAT -DLINUX -Wall -g -m486 \
		-DREAD_MMAP -DOSS -funroll-all-loops \
		-finline-functions -ffast-math' \
        mpg123-make

linux-profile:
	$(MAKE) OBJECTS='decode_i386.o dct64_i386.o audio_oss.o' \
        CC=gcc LDFLAGS='-pg' \
        CFLAGS='-DREAL_IS_FLOAT -DLINUX -Wall -pg -m486 \
		-DREAD_MMAP -DOSS -funroll-all-loops \
		-finline-functions -ffast-math' \
        mpg123-make

linux:
	$(MAKE) CC=gcc LDFLAGS= \
		OBJECTS='decode_i386.o dct64_i386.o decode_i586.o \
			audio_oss.o term.o' \
		CFLAGS='-DI386_ASSEM -DPENTIUM_OPT -DREAL_IS_FLOAT -DLINUX \
			-DREAD_MMAP -DOSS -DTERM_CONTROL\
			-Wall -O2 -m486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		mpg123-make

linux-thor:
	$(MAKE) CC=gcc LDFLAGS= \
		OBJECTS='decode_i386.o dct64_i386.o decode_i586.o \
			audio_oss.o term.o' \
		CFLAGS='-static -DI386_ASSEM -DPENTIUM_OPT -DREAL_IS_FLOAT -DLINUX \
			-DREAD_MMAP -DOSS -DTERM_CONTROL\
			-Wall -O2 -march=i486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		mpg123-make
		strip --strip-debug mpg123


linux-3dnow:
	$(MAKE) CC=gcc LDFLAGS= \
		OBJECTS='decode_i386.o dct64_3dnow.o \
			decode_3dnow.o audio_oss.o term.o' \
		CFLAGS='-DI386_ASSEM -DREAL_IS_FLOAT -DPENTIUM_OPT -DLINUX \
			-DUSE_3DNOW -DREAD_MMAP -DOSS -DTERM_CONTROL\
			-Wall -O2 -m486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		mpg123-make

linux-i486:
	$(MAKE) CC=gcc LDFLAGS= \
		OBJECTS='decode_i386.o dct64_i386.o decode_i586.o \
			decode_i486.o dct64_i486.o audio_oss.o term.o' \
		CFLAGS='-DI386_ASSEM -DREAL_IS_FLOAT -DI486_OPT -DLINUX \
			-DREAD_MMAP -DOSS -DTERM_CONTROL\
			-Wall -O2 -m486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		mpg123-make

linux-esd:
	$(MAKE) CC=gcc LDFLAGS= \
		AUDIO_LIB='-lesd -laudiofile' \
		OBJECTS='decode_i386.o dct64_i386.o decode_i586.o \
			audio_esd.o' \
		CFLAGS='-DI386_ASSEM -DREAL_IS_FLOAT -DPENTIUM_OPT -DLINUX \
			-DREAD_MMAP -DOSS -DUSE_ESD \
			-Wall  -O2 -m486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math \
			$(RPM_OPT_FLAGS)' \
		mpg123-make

linux-alsa:
	$(MAKE) CC=gcc LDFLAGS= \
		AUDIO_LIB='-lasound' \
		OBJECTS='decode_i386.o dct64_i386.o decode_i586.o \
			audio_alsa.o term.o' \
		CFLAGS='-DI386_ASSEM -DREAL_IS_FLOAT -DPENTIUM_OPT -DLINUX \
			-DREAD_MMAP -DALSA -DTERM_CONTROL\
			-Wall  -O2 -m486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math \
			$(RPM_OPT_FLAGS)' \
		mpg123-make

linux-mips-alsa:
	$(MAKE) CC=gcc LDFLAGS= \
		AUDIO_LIB='-lasound' \
		OBJECTS='decode.o dct64.o audio_alsa.o term.o' \
		CFLAGS='-DREAL_IS_FLOAT -DLINUX -DREAD_MMAP -DALSA \
			-DTERM_CONTROL -Wall  -O2 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math \
			$(RPM_OPT_FLAGS)' \
		mpg123-make

linux-alpha:
	$(MAKE) CC=gcc LDFLAGS= OBJECTS='decode.o dct64.o audio_oss.o' \
		CFLAGS='-DLINUX -DOSS -Wall -O2 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math \
			-Wall -O6 -DUSE_MMAP \
			$(RPM_OPT_FLAGS)' \
		mpg123-make

linux-alpha-esd:
	$(MAKE) CC=gcc LDFLAGS= \
		AUDIO_LIB='-lesd -laudiofile' \
		OBJECTS='decode.o dct64.o audio_esd.o' \
		CFLAGS='-DLINUX -DOSS -Wall -O2 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math \
			-Wall -O6 -DUSE_MMAP \
			$(RPM_OPT_FLAGS)' \
		mpg123-make

#linux-ppc:
#	$(MAKE) CC=gcc  LDFLAGS= \
#		OBJECTS='decode.o dct64.o audio_oss.o' \
#		CFLAGS='-DREAL_IS_FLOAT -DLINUX -Wall -O2 -mcpu=ppc \
#			-DOSS -DPPC_ENDIAN \
#			-fomit-frame-pointer -funroll-all-loops \
#			-finline-functions -ffast-math' \
#		mpg123-make

#linux-ppc-esd:
#	$(MAKE) CC=gcc  LDFLAGS= \
#		AUDIO_LIB='-lesd -laudiofile' \
#		OBJECTS='decode.o dct64.o audio_esd.o' \
#		CFLAGS='-DREAL_IS_FLOAT -DLINUX -Wall -O2 -mcpu=ppc \
#			-DOSS -DPPC_ENDIAN \
#			-fomit-frame-pointer -funroll-all-loops \
#			-finline-functions -ffast-math' \
#		mpg123-make

linux-ppc:
	$(MAKE) CC=gcc  LDFLAGS= \
		OBJECTS='decode.o dct64.o audio_oss.o' \
		CFLAGS='-DREAL_IS_FLOAT -DLINUX -Wall -O2 -mcpu=ppc \
			-DOSS \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		mpg123-make

linux-ppc-esd:
	$(MAKE) CC=gcc  LDFLAGS= \
		AUDIO_LIB='-lesd -laudiofile' \
		OBJECTS='decode.o dct64.o audio_esd.o' \
		CFLAGS='-DREAL_IS_FLOAT -DLINUX -Wall -O2 -mcpu=ppc \
			-DOSS  \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		mpg123-make

linux-sparc:
	$(MAKE) CC=gcc  LDFLAGS= \
		OBJECTS='decode.o dct64.o audio_sun.o' \
		CFLAGS='-DREAL_IS_FLOAT -DUSE_MMAP -DSPARCLINUX -Wall -O2 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		mpg123-make

linux-m68k:
	$(MAKE) CC=gcc LDFLAGS= OBJECTS='decode.o dct64.o audio_oss.o' \
		CFLAGS='-DREAL_IS_FLOAT -DLINUX -DREAD_MMAP \
			-DOSS -DOSS_BIG_ENDIAN -Wall -O2 -m68040 \
			-fomit-frame-pointer -funroll-loops \
			-finline-functions -ffast-math' \
		mpg123-make

linux-sajber:
	@ $(MAKE) FRONTEND=sajberplay-make linux-frontend

linux-tk3play:
	@ $(MAKE) FRONTEND=mpg123m-make linux-frontend

freebsd-sajber:
	@ $(MAKE) FRONTEND=sajberplay-make freebsd-frontend

freebsd-tk3play:
	@ $(MAKE) FRONTEND=mpg123m-make freebsd-frontend

linux-frontend:
	$(MAKE) CC=gcc LDFLAGS= \
		OBJECTS='decode_i386.o dct64_i386.o decode_i586.o \
			control_sajber.o control_tk3play.o audio_oss.o' \
		CFLAGS='-DFRONTEND -DOSS -DI386_ASSEM -DREAL_IS_FLOAT \
			-DPENTIUM_OPT -DLINUX -Wall -O2 -m486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		$(FRONTEND)

linux-nas:
	$(MAKE) CC=gcc LDFLAGS='-L/usr/X11R6/lib' \
		AUDIO_LIB='-laudio -lXau' \
		OBJECTS='decode_i386.o dct64_i386.o audio_nas.o' \
		CFLAGS='-I/usr/X11R6/include \
			-DI386_ASSEM -DREAL_IS_FLOAT -DLINUX -DNAS \
			-Wall -O2 -m486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		mpg123-make

#### the following defines are for experimental use ... 
#
#CFLAGS='-pg -DI386_ASSEM -DREAL_IS_FLOAT -DLINUX -Wall -O2 -m486 -funroll-all-loops -finline-functions -ffast-math' mpg123
#CFLAGS='-DI386_ASSEM -O2 -DREAL_IS_FLOAT -DLINUX -Wall -g'
#CFLAGS='-DI386_ASSEM -DREAL_IS_FLOAT -DLINUX -Wall -O2 -m486 -fomit-frame-pointer -funroll-all-loops -finline-functions -ffast-math -malign-loops=2 -malign-jumps=2 -malign-functions=2'

freebsd:
	$(MAKE) CC=cc LDFLAGS= \
		OBJECTS='decode_i386.o dct64_i386.o audio_oss.o' \
		CFLAGS='-Wall -ansi -pedantic -O4 -m486 -fomit-frame-pointer \
			-funroll-all-loops -ffast-math -DROT_I386 \
			-DREAD_MMAP \
			-DI386_ASSEM -DREAL_IS_FLOAT -DUSE_MMAP -DOSS' \
		mpg123-make

freebsd-esd:
	$(MAKE) CC=cc LDFLAGS= \
		AUDIO_LIB='-lesd -laudiofile' \
		OBJECTS='decode_i386.o dct64_i386.o $(GETBITS) audio_esd.o' \
		CFLAGS='-Wall -ansi -pedantic -O4 -m486 -fomit-frame-pointer \
			-funroll-all-loops -ffast-math -DROT_I386 \
			-DREAD_MMAP \
			-DI386_ASSEM -DREAL_IS_FLOAT -DUSE_MMAP -DOSS \
			-I/usr/local/include -L/usr/local/lib \
			$(CFLAGS)' \
		mpg123-make

freebsd-frontend:
	$(MAKE) CC=cc LDFLAGS= \
		OBJECTS='decode_i386.o dct64_i386.o audio_oss.o \
			control_sajber.o control_tk3play.o' \
		CFLAGS='-Wall -ansi -pedantic -O4 -m486 -fomit-frame-pointer \
			-funroll-all-loops -ffast-math -DROT_I386 \
			-DFRONTEND \
			-DI386_ASSEM -DREAL_IS_FLOAT -DUSE_MMAP -DOSS' \
		$(FRONTEND)
 

# -mno-epilogue
# -mflat -mv8 -mcpu=ultrasparc

# these are MY EXPERIMENTAL compile entries
solaris-pure:
	$(MAKE) CC='purify -cache-dir=/tmp cc' \
		LDFLAGS='-lsocket -lnsl' \
		OBJECTS='decode.o dct64.o audio_sun.o term.o' \
		CFLAGS='-fast -native -xO4 -DSOLARIS -DTERM_CONTROL \
			-DUSE_MMAP ' \
		mpg123-make

solaris-ccscc:
	$(MAKE) CC=/usr/ccs/bin/ucbcc LDFLAGS='-lsocket -lnsl' \
		OBJECTS='decode.o dct64.o audio_sun.o term.o' \
		CFLAGS='-fast -native -xO4 -DSOLARIS \
			-DUSE_MMAP ' \
		mpg123-make

# common solaris compile entries
solaris:
	$(MAKE) CC=cc LDFLAGS='-lsocket -lnsl' \
		OBJECTS='decode.o dct64.o audio_sun.o term.o' \
		CFLAGS='-fast -native -xO4 -DSOLARIS \
			-DUSE_MMAP -DTERM_CONTROL' \
		mpg123-make

solaris-gcc-profile:
	$(MAKE) CC='gcc' \
		LDFLAGS='-lsocket -lnsl -pg' \
		OBJECTS='decode.o dct64.o audio_sun.o' \
		CFLAGS='-g -pg -O2 -Wall -DSOLARIS -DREAL_IS_FLOAT -DUSE_MMAP \
			-DREAD_MMAP \
			-funroll-all-loops -finline-functions' \
		mpg123-make

solaris-gcc:
	$(MAKE) CC=gcc \
		LDFLAGS='-lsocket -lnsl' \
		OBJECTS='decode.o dct64.o audio_sun.o term.o' \
		CFLAGS='-O2 -Wall -pedantic -DSOLARIS -DREAL_IS_FLOAT -DUSE_MMAP \
			-DREAD_MMAP -DTERM_CONTROL \
			-funroll-all-loops  -finline-functions' \
		mpg123-make

solaris-gcc-esd:
	$(MAKE) CC=gcc LDFLAGS='-lsocket -lnsl' \
		AUDIO_LIB='-lesd -lresolv' \
		OBJECTS='decode.o dct64.o audio_esd.o' \
		CFLAGS='-O2 -Wall -DSOLARIS -DREAL_IS_FLOAT -DUSE_MMAP \
			-DREAD_MMAP \
			-funroll-all-loops -finline-functions' \
		mpg123-make

solaris-x86-gcc-oss:
	$(MAKE) CC=gcc LDFLAGS='-lsocket -lnsl' \
		OBJECTS='decode_i386.o dct64_i386.o decode_i586.o \
			audio_oss.o' \
		CFLAGS='-DI386_ASSEM -DREAL_IS_FLOAT -DPENTIUM_OPT -DUSE_MMAP \
			-DREAD_MMAP -DOSS \
			-Wall -O2 -m486 \
			-funroll-all-loops -finline-functions' \
		mpg123-make

solaris-gcc-nas:
	$(MAKE) CC=gcc LDFLAGS='-lsocket -lnsl' \
		AUDIO_LIB='-L/usr/openwin/lib -laudio -lXau'\
		OBJECTS='decode.o dct64.o audio_nas.o' \
		CFLAGS='-O2 -I/usr/openwin/include -Wall \
			-DSOLARIS -DREAL_IS_FLOAT -DUSE_MMAP \
			-DREAD_MMAP -DNAS \
			-funroll-all-loops -finline-functions' \
		mpg123-make

sunos:
	$(MAKE) CC=gcc LDFLAGS= \
		OBJECTS='decode.o dct64.o audio_sun.o' \
		CFLAGS='-O2 -DSUNOS -DREAL_IS_FLOAT -DUSE_MMAP \
			-funroll-loops' \
		mpg123-make

#		CFLAGS='-DREAL_IS_FLOAT -Aa +O3 -D_HPUX_SOURCE -DHPUX'
hpux:
	$(MAKE) CC=cc LDFLAGS= \
		OBJECTS='decode.o dct64.o audio_hp.o' \
		CFLAGS='-DREAL_IS_FLOAT -Ae +O3 -D_HPUX_SOURCE -DHPUX' \
		mpg123-make

hpux-alib:
	$(MAKE) CC=cc LDFLAGS='-L/opt/audio/lib' \
		OBJECTS='decode.o dct64.o audio_alib.o' \
		AUDIO_LIB=-lAlib \
		CFLAGS='-DREAL_IS_FLOAT -Ae +O3 -D_HPUX_SOURCE -DHPUX \
			-I/opt/audio/include' \
		mpg123-make

hpux-gcc:
	$(MAKE) CC=gcc LDFLAGS= OBJECTS='decode.o dct64.o audio_hp.o' \
		CFLAGS='-DREAL_IS_FLOAT -O3 -D_HPUX_SOURCE -DHPUX' \
		mpg123-make
sgi:
	$(MAKE) CC=cc LDFLAGS= \
		OBJECTS='decode.o dct64.o audio_sgi.o' AUDIO_LIB=-laudio \
		CFLAGS='-O2 -DSGI -DREAL_IS_FLOAT -DUSE_MMAP' \
		mpg123-make

sgi-gcc:
	$(MAKE) CC=gcc LDFLAGS= \
		OBJECTS='decode.o dct64.o audio_sgi.o' AUDIO_LIB=-laudio \
		CFLAGS='-O2 -DSGI -DREAL_IS_FLOAT -DUSE_MMAP' \
		mpg123-make

dec:
	$(MAKE) CC=cc LDFLAGS= OBJECTS='decode.o dct64.o audio_dummy.o' \
		CFLAGS='-std1 -warnprotos -O4 -DUSE_MMAP' \
		mpg123-make

dec-nas:
	$(MAKE) CC=cc LDFLAGS='-L/usr/X11R6/lib' \
		AUDIO_LIB='-laudio -lXau -ldnet_stub'\
		OBJECTS='decode.o dct64.o  audio_nas.o' \
		CFLAGS='-I/usr/X11R6/include -std1 -warnprotos -O4 -DUSE_MMAP' \
		mpg123-make

ultrix:
	$(MAKE) CC=cc LDFLAGS= OBJECTS='decode.o dct64.o audio_dummy.o' \
		CFLAGS='-std1 -O2 -DULTRIX' \
		mpg123-make

aix-gcc:
	$(MAKE) CC=gcc LDFLAGS= OBJECTS='decode.o dct64.o audio_aix.o' \
		CFLAGS='-DAIX -Wall -O6 -DUSE_MMAP -DREAD_MMAP -DREAL_IS_FLOAT \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		mpg123-make

aix-xlc:
	$(MAKE) LDFLAGS= OBJECTS='decode.o dct64.o audio_aix.o' \
		CFLAGS="$(CFLAGS) -O3 -qstrict -qcpluscmt -DAIX -DUSE_MMAP \
			-DREAD_MMAP " \
		mpg123-make

aix-tk3play:
	@ $(MAKE) FRONTEND=mpg123m-make aix-frontend

aix-frontend:
	$(MAKE) LDFLAGS= OBJECTS='decode.o dct64.o audio_aix.o \
			control_sajber.o control_tk3play.o' \
		CFLAGS='-DAIX -Wall -O6 -DUSE_MMAP -DFRONTEND \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		$(FRONTEND)

os2:
	$(MAKE) CC=gcc LDFLAGS= \
		OBJECTS='decode_i386.o dct64_i386.o audio_os2.o' \
		CFLAGS='-DREAL_IS_FLOAT -DNOXFERMEM -DOS2 -Wall -O2 -m486 \
		-fomit-frame-pointer -funroll-all-loops \
		-finline-functions -ffast-math' \
		LIBS='-los2me -lsocket' \
		mpg123.exe

netbsd:
	$(MAKE) CC=cc LDFLAGS= \
		OBJECTS='decode.o dct64.o audio_sun.o' \
		CFLAGS='-Wall -ansi -pedantic -O3 -fomit-frame-pointer \
			-funroll-all-loops -ffast-math \
			-DREAL_IS_FLOAT -DUSE_MMAP -DNETBSD' \
		mpg123-make

netbsd-i386:
	$(MAKE) CC=cc LDFLAGS= \
		OBJECTS='decode_i386.o dct64_i386.o audio_sun.o' \
		CFLAGS='-Wall -ansi -pedantic -O4 -m486 -fomit-frame-pointer \
			-funroll-all-loops -ffast-math -DROT_I386 \
			-DI386_ASSEM -DREAL_IS_FLOAT -DUSE_MMAP -DNETBSD' \
		mpg123-make

bsdos:
	$(MAKE) CC=shlicc2 LDFLAGS= \
		OBJECTS='decode_i386.o dct64_i386.o \
			 audio_oss.o' \
		CFLAGS='-Wall -O4 -m486 -fomit-frame-pointer \
			-funroll-all-loops -ffast-math -DROT_I386 \
			-DI386_ASSEM -DREAL_IS_FLOAT -DUSE_MMAP -DOSS \
			-DDONT_CATCH_SIGNALS' \
		mpg123-make

bsdos4:
	$(MAKE) CC=gcc LDFLAGS= \
		OBJECTS='decode_i386.o dct64_i386.o audio_oss.o' \
		CFLAGS='-Wall -O4 -m486 -fomit-frame-pointer \
			-funroll-all-loops -ffast-math -DROT_I386 \
			-DI386_ASSEM -DREAL_IS_FLOAT -DUSE_MMAP -DOSS \
			-DREAD_MMAP -DDONT_CATCH_SIGNALS' \
		mpg123-make

bsdos-nas:
	$(MAKE) CC=shlicc2 LDFLAGS= \
		AUDIO_LIB='-laudio -lXau -L/usr/X11R6/lib' \
		OBJECTS='decode_i386.o dct64_i386.o \
			audio_nas.o' \
		CFLAGS='-Wall -O4 -m486 -fomit-frame-pointer \
			-funroll-all-loops -ffast-math -DROT_I386 \
			-DI386_ASSEM -DREAL_IS_FLOAT -DUSE_MMAP -DOSS \
			-DDONT_CATCH_SIGNALS -DNAS' \
		mpg123-make

mint:
	$(MAKE) CC=gcc LDFLAGS= \
		OBJECTS='decode.o dct64.o audio_mint.o' \
		CFLAGS='-Wall -O2 -m68020-40 -m68881 \
		-fomit-frame-pointer -funroll-all-loops \
		-finline-functions -ffast-math \
		-DREAL_IS_FLOAT -DMINT -DNOXFERMEM' \
		AUDIO_LIB='-lsocket' \
		mpg123-make

# maybe you need the additonal options LDFLAGS='-lnsl -lsocket' when linking (see solaris:)
generic:
	$(MAKE) LDFLAGS= OBJECTS='decode.o dct64.o audio_dummy.o' \
		CFLAGS='-O -DGENERIC -DNOXFERMEM' \
		mpg123-make

###########################################################################
###########################################################################
###########################################################################

sajberplay-make:
	@ $(MAKE) CFLAGS='$(CFLAGS)' BINNAME=sajberplay mpg123

mpg123m-make:
	@ $(MAKE) CFLAGS='$(CFLAGS)' BINNAME=mpg123m mpg123

mpg123-make:
	@ $(MAKE) CFLAGS='$(CFLAGS)' BINNAME=mpg123 mpg123

mpg123: mpg123.o common.o $(OBJECTS) decode_2to1.o decode_4to1.o \
		tabinit.o audio.o layer1.o layer2.o layer3.o buffer.o \
		getlopt.o httpget.o xfermem.o equalizer.o \
		decode_ntom.o Makefile wav.o readers.o getbits.o \
		control_generic.o
	$(CC) $(CFLAGS) $(LDFLAGS)  mpg123.o tabinit.o common.o layer1.o \
		layer2.o layer3.o audio.o buffer.o decode_2to1.o equalizer.o \
		decode_4to1.o getlopt.o httpget.o xfermem.o decode_ntom.o \
		wav.o readers.o getbits.o control_generic.o \
		$(OBJECTS) -o $(BINNAME) -lm $(AUDIO_LIB)

mpg123.exe: mpg123.o common.o $(OBJECTS) decode_2to1.o decode_4to1.o \
		tabinit.o audio.o layer1.o layer2.o layer3.o buffer.o \
		getlopt.o httpget.o Makefile wav.o readers.o getbits.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o mpg123.exe -lm $(LIBS)

###########################################################################
###########################################################################
###########################################################################

layer1.o:	mpg123.h
layer2.o:	mpg123.h l2tables.h
layer3.o:	mpg123.h huffman.h common.h getbits.h
decode.o:	mpg123.h
decode_2to1.o:	mpg123.h
decode_4to1.o:	mpg123.h
decode_ntom.o:	mpg123.h
decode_i386.o:	mpg123.h
common.o:	mpg123.h common.h
mpg123.o:	mpg123.c mpg123.h getlopt.h xfermem.h version.h buffer.h term.h
mpg123.h:	audio.h
audio.o:	mpg123.h
audio_oss.o:	mpg123.h
audio_sun.o:	mpg123.h
audio_sgi.o:	mpg123.h
audio_hp.o:	mpg123.h
audio_nas.o:	mpg123.h
audio_os2.o:	mpg123.h
audio_dummy.o:	mpg123.h
buffer.o:	mpg123.h xfermem.h buffer.h
getbits.o:	common.h mpg123.h
tabinit.o:	mpg123.h audio.h
getlopt.o:	getlopt.h
httpget.o:	mpg123.h
dct64.o:	mpg123.h
dct64_i386.o:	mpg123.h
xfermem.o:	xfermem.h
equalizer.o:	mpg123.h
control_sajber.o:	jukebox/controldata.h mpg123.h
wav.o:		mpg123.h
readers.o:	mpg123.h buffer.h common.h
term.o:		mpg123.h buffer.h term.h common.h

###########################################################################
###########################################################################
###########################################################################

clean:
	rm -f *.o *core *~ mpg123 gmon.out sajberplay system mpg123m

prepared-for-install:
	@if [ ! -x mpg123 ]; then \
		echo '###' ; \
		echo '###  Before doing "make install", you have to compile the software.' ; \
		echo '### Type "make" for more information.' ; \
		echo '###' ; \
		exit 1 ; \
	fi

system: mpg123.h system.c
	$(CC) -o $@ -Wall -O2 system.c

install:	prepared-for-install
	strip mpg123
	if [ -x /usr/ccs/bin/mcs ]; then /usr/ccs/bin/mcs -d mpg123; fi
	mkdir -p $(BINDIR)
	mkdir -p $(MANDIR)/man$(SECTION)
	cp -f mpg123 $(BINDIR)
	chmod 755 $(BINDIR)/mpg123
	cp -f mpg123.1 $(MANDIR)/man$(SECTION)
	chmod 644 $(MANDIR)/man$(SECTION)/mpg123.1

dist:	clean
	DISTNAME="`basename \`pwd\``" ; \
	sed '/prgDate/s_".*"_"'`date +%Y/%m/%d`'"_' version.h > version.new; \
	mv -f version.new version.h; \
	cd .. ; \
	rm -f "$$DISTNAME".tar.gz "$$DISTNAME".tar ; \
	tar cvf "$$DISTNAME".tar "$$DISTNAME" ; \
	gzip -9 "$$DISTNAME".tar


