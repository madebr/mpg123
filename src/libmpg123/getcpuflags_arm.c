/*
	getcpuflags_arm: get cpuflags for ARM

	copyright 1995-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Momma

	This has been reduced to a no-op if the compiler is set up to emit NEON code
	itself, avoiding the problematic messing with signals and long jumps.
	This business only works in a single-threaded context.
*/

// For sigsetjmp, we need POSIX 2001
#define _POSIX_C_SOURCE 200112L
// For SA_RESTART, XSI is needed (or POSIX 2008).
// This here should include the above, keeping that for clarity.
#define _XOPEN_SOURCE 600

#include <setjmp.h>
#include <signal.h>
#include "mpg123lib_intern.h"
#include "getcpuflags.h"

// Try to avoid the nasty runtime check.
#ifdef __ARM_NEON
// Compiler is configured to emit NEON itself, no sense in checking.
#define NEON_IS_BUILTIN
#elif defined(HAVE_SYS_AUXV_H)
// Try the auxv method to query capabilities from the kernel.
// Documenation varies about which flag to test. I assume it's all defines and
// can be checked. I see discussions with and without ARM infix. Just trying all.
#include <sys/auxv.h>
#ifdef HWCAP_ARM_NEON
#define NEON_FLAG HWCAP_ARM_NEON
#elif defined(HWCAP_NEON)
#define NEON_FLAG HWCAP_NEON
#elif defined(HWCAP_ASIMD)
#define NEON_FLAG HWCAP_ASIMD
#elif defined(HWCAP_ARM_ASIMD)
#define NEON_FLAG HWCAP_ARM_ASIMD
#endif /* flags */
#endif

#if !(defined(NEON_IS_BUILTIN) || defined(NEON_FLAG))
#define NEON_CHECK
#endif


#if defined(NEON_CHECK)
extern void INT123_check_neon(void);

#ifndef _WIN32
static sigjmp_buf jmpbuf;
#else
static jmp_buf jmpbuf;
#endif

static void mpg123_arm_catch_sigill(int sig)
{
#ifndef _WIN32
	siglongjmp(jmpbuf, 1);
#else
	longjmp(jmpbuf, 1);
#endif
}
#endif /* NEON_CHECK */

unsigned int INT123_getcpuflags(struct cpuflags* cf)
{
	cf->has_neon = 0;
#ifdef NEON_IS_BUILTIN
	cf->has_neon = 1;
#elif defined(NEON_FLAG)
	unsigned long hwcaps = getauxval(AT_HWCAP);
	if(hwcaps & NEON_FLAG)
		cf->has_neon = 1;
#elif defined(NEON_CHECK)

// This code should vanish at some point. It is dangerous.
#ifndef _WIN32
	struct sigaction act, act_old;
	act.sa_handler = mpg123_arm_catch_sigill;
	act.sa_flags = SA_RESTART;
	sigemptyset(&act.sa_mask);
	sigaction(SIGILL, &act, &act_old);
	
	
	if(!sigsetjmp(jmpbuf, 1)) {
		INT123_check_neon();
		cf->has_neon = 1;
	}
	
	sigaction(SIGILL, &act_old, NULL);
#else
	if (!setjmp(jmpbuf)) {
		signal(SIGILL, mpg123_arm_catch_sigill);
		INT123_check_neon();
		cf->has_neon = 1;
	}

	signal(SIGILL, SIG_DFL);
#endif

#endif /* builtin / hwcap */
	
	return 0;
}
