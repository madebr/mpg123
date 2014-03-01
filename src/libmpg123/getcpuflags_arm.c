/*
	getcpuflags_arm: get cpuflags for ARM

	copyright 1995-2014 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Momma
*/

#include <setjmp.h>
#include <signal.h>
#include "mpg123lib_intern.h"
#include "getcpuflags.h"

static sigjmp_buf jmpbuf;

static void mpg123_arm_catch_sigill(int sig)
{
	siglongjmp(jmpbuf, 1);
}

unsigned int getcpuflags(struct cpuflags* cf)
{
	struct sigaction act, act_old;
	act.sa_handler = mpg123_arm_catch_sigill;
	act.sa_flags = SA_RESTART;
	sigemptyset(&act.sa_mask);
	sigaction(SIGILL, &act, &act_old);
	
	cf->has_neon = 0;
	
	if(!sigsetjmp(jmpbuf, 1)) {
#ifdef __thumb__
		__asm__ __volatile__(".byte 0x20, 0xef, 0x10, 0x01\n\t"); /* vorr d0, d0, d0 */
#else
		__asm__ __volatile__(".byte 0x10, 0x01, 0x20, 0xf2\n\t"); /* vorr d0, d0, d0 */
#endif
		cf->has_neon = 1;
	}
	
	sigaction(SIGILL, &act_old, NULL);
	
	return 0;
}
