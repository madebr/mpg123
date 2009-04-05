/*
	synth_x86_64_float: SSE optimized synth for x86-64 (float output version)

	copyright 1995-2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "mangle.h"

/* real *window; */
#define ARG0 %rdi
/* real *b0; */
#define ARG1 %rsi
/* real *samples; */
#define ARG2 %rdx
/* int bo1; */
#define ARG3 %ecx

#define XMMREG_SCALE %xmm15  /* {1/32768.0, 1/32768.0, 1/32768.0, 1/32768.0} */

/*
	int synth_1to1_real_x86_64_asm(real *window, real *b0, real *samples, int bo1);
	return value: number of clipped samples (0)
*/

#ifndef __APPLE__
	.section	.rodata
#else
	.data
#endif
	ALIGN32
ASM_NAME(scale_x86_64):
	.long   939524096
	.long   939524096
	.long   939524096
	.long   939524096
	.text
	ALIGN16,,15
.globl ASM_NAME(synth_1to1_real_x86_64_asm)
ASM_NAME(synth_1to1_real_x86_64_asm):
	
	leaq		ASM_NAME(scale_x86_64)(%rip), %rax
	movaps		(%rax), XMMREG_SCALE
	
	movl		ARG3, %eax
	shll		$2, %eax
	cltq
	leaq		64(ARG0), ARG0
	subq		%rax, ARG0
	movq		%rax, %r8
	shlq		$1, %r8

	movl		$4, %ecx
	
	ALIGN16
loop_start_1:
	movups		(ARG0), %xmm8
	movups		16(ARG0), %xmm1
	movups		32(ARG0), %xmm2
	movups		48(ARG0), %xmm3
	movups		128(ARG0), %xmm9
	movups		144(ARG0), %xmm5
	movups		160(ARG0), %xmm6
	movups		176(ARG0), %xmm7
	mulps		(ARG1), %xmm8
	mulps		16(ARG1), %xmm1
	mulps		32(ARG1), %xmm2
	mulps		48(ARG1), %xmm3
	mulps		64(ARG1), %xmm9
	mulps		80(ARG1), %xmm5
	mulps		96(ARG1), %xmm6
	mulps		112(ARG1), %xmm7
	
	addps		%xmm1, %xmm8
	addps		%xmm2, %xmm3
	addps		%xmm5, %xmm9
	addps		%xmm7, %xmm6
	addps		%xmm3, %xmm8
	addps		%xmm6, %xmm9
	leaq		256(ARG0), ARG0
	leaq		128(ARG1), ARG1
	
	movups		(ARG0), %xmm10
	movups		16(ARG0), %xmm1
	movups		32(ARG0), %xmm2
	movups		48(ARG0), %xmm3
	movups		128(ARG0), %xmm11
	movups		144(ARG0), %xmm5
	movups		160(ARG0), %xmm6
	movups		176(ARG0), %xmm7
	mulps		(ARG1), %xmm10
	mulps		16(ARG1), %xmm1
	mulps		32(ARG1), %xmm2
	mulps		48(ARG1), %xmm3
	mulps		64(ARG1), %xmm11
	mulps		80(ARG1), %xmm5
	mulps		96(ARG1), %xmm6
	mulps		112(ARG1), %xmm7
	
	addps		%xmm1, %xmm10
	addps		%xmm2, %xmm3
	addps		%xmm5, %xmm11
	addps		%xmm7, %xmm6
	addps		%xmm3, %xmm10
	addps		%xmm6, %xmm11
	leaq		256(ARG0), ARG0
	leaq		128(ARG1), ARG1
	
	movaps		%xmm8, %xmm0
	movaps		%xmm9, %xmm1
	unpcklps	%xmm10, %xmm8
	unpcklps	%xmm11, %xmm9
	unpckhps	%xmm10, %xmm0
	unpckhps	%xmm11, %xmm1
	movaps		%xmm8, %xmm2
	movaps		%xmm0, %xmm3
	unpcklps	%xmm9, %xmm8
	unpckhps	%xmm9, %xmm2
	unpcklps	%xmm1, %xmm0
	unpckhps	%xmm1, %xmm3
	subps		%xmm2, %xmm8
	subps		%xmm3, %xmm0
	addps		%xmm8, %xmm0
	
	movups		(ARG2), %xmm1
	movups		16(ARG2), %xmm2
	mulps		XMMREG_SCALE, %xmm0
	shufps		$0xdd, %xmm2, %xmm1
	movaps		%xmm0, %xmm2
	unpcklps	%xmm1, %xmm0
	unpckhps	%xmm1, %xmm2
	movups		%xmm0, (ARG2)
	movups		%xmm2, 16(ARG2)
	
	leaq		32(ARG2), ARG2
	decl		%ecx
	jnz			loop_start_1
	
	movups		(ARG0), %xmm0
	movups		16(ARG0), %xmm1
	movups		32(ARG0), %xmm2
	movups		48(ARG0), %xmm3
	movaps		(ARG1), %xmm4
	movaps		32(ARG1), %xmm6
	shufps		$0x88, %xmm1, %xmm0
	shufps		$0x88, %xmm3, %xmm2
	shufps		$0x88, 16(ARG1), %xmm4
	shufps		$0x88, 48(ARG1), %xmm6
	mulps		%xmm4, %xmm0
	mulps		%xmm6, %xmm2
	addps		%xmm2, %xmm0
	pshuflw		$0xee, %xmm0, %xmm1
	movhlps		%xmm0, %xmm2
	pshuflw		$0xee, %xmm2, %xmm3
	xorps		%xmm4, %xmm4
	addss		%xmm0, %xmm4
	addss		%xmm1, %xmm4
	addss		%xmm2, %xmm4
	addss		%xmm3, %xmm4
	
	mulss		XMMREG_SCALE, %xmm4
	movss		%xmm4, (ARG2)
	
	leaq		-128(ARG0), ARG0
	leaq		-64(ARG1), ARG1
	leaq		8(ARG2), ARG2
	addq		%r8, ARG0
	
	movl		$4, %ecx
	
	ALIGN16
loop_start_2:
	movups		-16(ARG0), %xmm8
	movups		-32(ARG0), %xmm1
	movups		-48(ARG0), %xmm2
	movups		-64(ARG0), %xmm3
	movups		-144(ARG0), %xmm9
	movups		-160(ARG0), %xmm5
	movups		-176(ARG0), %xmm6
	movups		-192(ARG0), %xmm7
	shufps		$0x1b, %xmm8, %xmm8
	shufps		$0x1b, %xmm1, %xmm1
	shufps		$0x1b, %xmm2, %xmm2
	shufps		$0x1b, %xmm3, %xmm3
	shufps		$0x1b, %xmm9, %xmm9
	shufps		$0x1b, %xmm5, %xmm5
	shufps		$0x1b, %xmm6, %xmm6
	shufps		$0x1b, %xmm7, %xmm7
	mulps		(ARG1), %xmm8
	mulps		16(ARG1), %xmm1
	mulps		32(ARG1), %xmm2
	mulps		48(ARG1), %xmm3
	mulps		-64(ARG1), %xmm9
	mulps		-48(ARG1), %xmm5
	mulps		-32(ARG1), %xmm6
	mulps		-16(ARG1), %xmm7
	
	addps		%xmm1, %xmm8
	addps		%xmm2, %xmm3
	addps		%xmm5, %xmm9
	addps		%xmm7, %xmm6
	addps		%xmm3, %xmm8
	addps		%xmm6, %xmm9
	leaq		-256(ARG0), ARG0
	leaq		-128(ARG1), ARG1
	
	decl		%ecx
	jz			last
	
	movups		-16(ARG0), %xmm10
	movups		-32(ARG0), %xmm1
	movups		-48(ARG0), %xmm2
	movups		-64(ARG0), %xmm3
	movups		-144(ARG0), %xmm11
	movups		-160(ARG0), %xmm5
	movups		-176(ARG0), %xmm6
	movups		-192(ARG0), %xmm7
	shufps		$0x1b, %xmm10, %xmm10
	shufps		$0x1b, %xmm1, %xmm1
	shufps		$0x1b, %xmm2, %xmm2
	shufps		$0x1b, %xmm3, %xmm3
	shufps		$0x1b, %xmm11, %xmm11
	shufps		$0x1b, %xmm5, %xmm5
	shufps		$0x1b, %xmm6, %xmm6
	shufps		$0x1b, %xmm7, %xmm7
	mulps		(ARG1), %xmm10
	mulps		16(ARG1), %xmm1
	mulps		32(ARG1), %xmm2
	mulps		48(ARG1), %xmm3
	mulps		-64(ARG1), %xmm11
	mulps		-48(ARG1), %xmm5
	mulps		-32(ARG1), %xmm6
	mulps		-16(ARG1), %xmm7
	
	addps		%xmm1, %xmm10
	addps		%xmm2, %xmm3
	addps		%xmm5, %xmm11
	addps		%xmm7, %xmm6
	addps		%xmm3, %xmm10
	addps		%xmm6, %xmm11
	leaq		-256(ARG0), ARG0
	leaq		-128(ARG1), ARG1
	
	movaps		%xmm8, %xmm0
	movaps		%xmm9, %xmm1
	unpcklps	%xmm10, %xmm8
	unpcklps	%xmm11, %xmm9
	unpckhps	%xmm10, %xmm0
	unpckhps	%xmm11, %xmm1
	movaps		%xmm8, %xmm2
	movaps		%xmm0, %xmm3
	unpcklps	%xmm9, %xmm8
	unpckhps	%xmm9, %xmm2
	unpcklps	%xmm1, %xmm0
	unpckhps	%xmm1, %xmm3
	addps		%xmm2, %xmm8
	addps		%xmm3, %xmm0
	addps		%xmm8, %xmm0
	pcmpeqd		%xmm1, %xmm1
	pslld		$31, %xmm1
	xorps		%xmm1, %xmm0
	
	movups		(ARG2), %xmm1
	movups		16(ARG2), %xmm2
	mulps		XMMREG_SCALE, %xmm0
	shufps		$0xdd, %xmm2, %xmm1
	movaps		%xmm0, %xmm2
	unpcklps	%xmm1, %xmm0
	unpckhps	%xmm1, %xmm2
	movups		%xmm0, (ARG2)
	movups		%xmm2, 16(ARG2)
	
	leaq		32(ARG2), ARG2
	jmp			loop_start_2
	
	ALIGN16
last:
	movups		-16(ARG0), %xmm10
	movups		-32(ARG0), %xmm1
	movups		-48(ARG0), %xmm2
	movups		-64(ARG0), %xmm3
	shufps		$0x1b, %xmm10, %xmm10
	shufps		$0x1b, %xmm1, %xmm1
	shufps		$0x1b, %xmm2, %xmm2
	shufps		$0x1b, %xmm3, %xmm3
	mulps		(ARG1), %xmm10
	mulps		16(ARG1), %xmm1
	mulps		32(ARG1), %xmm2
	mulps		48(ARG1), %xmm3
	addps		%xmm1, %xmm10
	addps		%xmm2, %xmm10
	addps		%xmm3, %xmm10
	xorps		%xmm11, %xmm11

	movaps		%xmm8, %xmm0
	movaps		%xmm9, %xmm1
	unpcklps	%xmm10, %xmm8
	unpcklps	%xmm11, %xmm9
	unpckhps	%xmm10, %xmm0
	unpckhps	%xmm11, %xmm1
	movaps		%xmm8, %xmm2
	movaps		%xmm0, %xmm3
	unpcklps	%xmm9, %xmm8
	unpckhps	%xmm9, %xmm2
	unpcklps	%xmm1, %xmm0
	unpckhps	%xmm1, %xmm3
	addps		%xmm2, %xmm8
	addps		%xmm3, %xmm0
	addps		%xmm8, %xmm0
	pcmpeqd		%xmm1, %xmm1
	pslld		$31, %xmm1
	xorps		%xmm1, %xmm0
	
	movups		(ARG2), %xmm1
	movups		16(ARG2), %xmm2
	mulps		XMMREG_SCALE, %xmm0
	shufps		$0xdd, %xmm2, %xmm1
	movaps		%xmm0, %xmm2
	unpcklps	%xmm1, %xmm0
	unpckhps	%xmm1, %xmm2
	movups		%xmm0, (ARG2)
	movlps		%xmm2, 16(ARG2)
	
	xorl		%eax, %eax
	
	ret

/* Mark non-executable stack. */
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
