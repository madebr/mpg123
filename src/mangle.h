/* mangle.h - This file has some CPP macros to deal with different symbol
 * mangling across binary formats.
 * (c)2002 by Felix Buenemann <atmosfear at users.sourceforge.net>
 * File licensed under the GPL, see http://www.fsf.org/ for more info.
 */

/* ThOr: added the plain ASM_NAME
   Also this is getting more generic with the align stuff. */

#ifndef __MANGLE_H
#define __MANGLE_H

#include "config.h"

#ifdef ASMALIGN_EXP
#define ALIGN4  .align 2
#define ALIGN8  .align 3
#define ALIGN16 .align 4
#define ALIGN32 .align 5
#else
#define ALIGN4  .align 4
#define ALIGN8  .align 8
#define ALIGN16 .align 16
#define ALIGN32 .align 32
#endif

/* Feel free to add more to the list, eg. a.out IMO */
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__OS2__) || \
   (defined(__OpenBSD__) && !defined(__ELF__)) || defined(__APPLE__)
#define MANGLE(a) "_" #a
#define ASM_NAME(a) _##a
#define ASM_VALUE(a) $_##a
#else
#define MANGLE(a) #a
#define ASM_NAME(a) a
#define ASM_VALUE(a) "$" #a
#endif

#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__APPLE__)
#define COMM(a,b,c) .comm a,b
#else
#define COMM(a,b,c) .comm a,b,c
#endif
/* more hacks for macosx; no .bss ... */
#ifdef __APPLE__
#define BSS .data
#else
#define BSS .bss
#endif
#endif /* !__MANGLE_H */

