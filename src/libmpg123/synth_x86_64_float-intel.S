#include "gas_nasm_compat.h"
/*
	synth_x86_64_float: SSE optimized synth for x86-64 (float output version)

	copyright 1995-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "x86_64_defs.h"
#include "mangle.h"

#ifdef IS_MSABI
/* short *window; */
#define ARG0 r10
/* short *b0; */
#define ARG1 rdx
/* short *samples; */
#define ARG2 r8
/* int bo1; */
#define ARG3 r9
#else
/* real *window; */
#define ARG0 rdi
/* real *b0; */
#define ARG1 rsi
/* real *samples; */
#define ARG2 rdx
/* int bo1; */
#define ARG3 rcx
#endif

#define XMMREG_SCALE xmm15  /* {1/32768.0, 1/32768.0, 1/32768.0, 1/32768.0} */

/*
	int synth_1to1_real_x86_64_asm(real *window, real *b0, real *samples, int bo1);
	return value: number of clipped samples (0)
*/

	RODATA
	ALIGN32
ASM_NAME(INT123_scale_x86_64):
	DATA_LONG   939524096
	DATA_LONG   939524096
	DATA_LONG   939524096
	DATA_LONG   939524096
	SECTION_TEXT
	ALIGN16
GLOBAL ASM_NAME(INT123_synth_1to1_real_x86_64_asm)
ASM_NAME(INT123_synth_1to1_real_x86_64_asm):
#ifdef IS_MSABI /* should save xmm6-15 */
	mov		ARG0, rcx
	sub		rsp, 120 /* stack alignment + 7 xmm registers */
	movaps		[rsp], xmm6
	movaps		[rsp+16], xmm7
	movaps		[rsp+32], xmm8
	movaps		[rsp+48], xmm9
	movaps		[rsp+64], xmm10
	movaps		[rsp+80], xmm11
	movaps		[rsp+96], xmm15
#endif

	lea		rax, ASM_NAME(INT123_scale_x86_64) RIP_REL
	movaps		XMMREG_SCALE, [rax]
	
	and		ARG3, HEX(f)
	shl		ARG3, 2
	lea		ARG0, 64[ARG0]
	sub		ARG0, ARG3

	mov		ecx, 4
	
	ALIGN16
loop1:
	movups		xmm8, [ARG0]
	movups		xmm1, [ARG0+16]
	movups		xmm2, [ARG0+32]
	movups		xmm3, [ARG0+48]
	movups		xmm9, [ARG0+128]
	movups		xmm5, [ARG0+144]
	movups		xmm6, [ARG0+160]
	movups		xmm7, [ARG0+176]
	mulps		xmm8, [ARG1]
	mulps		xmm1, [ARG1+16]
	mulps		xmm2, [ARG1+32]
	mulps		xmm3, [ARG1+48]
	mulps		xmm9, [ARG1+64]
	mulps		xmm5, [ARG1+80]
	mulps		xmm6, [ARG1+96]
	mulps		xmm7, [ARG1+112]
	
	addps		xmm8, xmm1
	addps		xmm3, xmm2
	addps		xmm9, xmm5
	addps		xmm6, xmm7
	addps		xmm8, xmm3
	addps		xmm9, xmm6
	lea		ARG0, 256[ARG0]
	lea		ARG1, 128[ARG1]
	
	movups		xmm10, [ARG0]
	movups		xmm1, [ARG0+16]
	movups		xmm2, [ARG0+32]
	movups		xmm3, [ARG0+48]
	movups		xmm11, [ARG0+128]
	movups		xmm5, [ARG0+144]
	movups		xmm6, [ARG0+160]
	movups		xmm7, [ARG0+176]
	mulps		xmm10, [ARG1]
	mulps		xmm1, [ARG1+16]
	mulps		xmm2, [ARG1+32]
	mulps		xmm3, [ARG1+48]
	mulps		xmm11, [ARG1+64]
	mulps		xmm5, [ARG1+80]
	mulps		xmm6, [ARG1+96]
	mulps		xmm7, [ARG1+112]
	
	addps		xmm10, xmm1
	addps		xmm3, xmm2
	addps		xmm11, xmm5
	addps		xmm6, xmm7
	addps		xmm10, xmm3
	addps		xmm11, xmm6
	lea		ARG0, 256[ARG0]
	lea		ARG1, 128[ARG1]
	
	movaps		xmm0, xmm8
	movaps		xmm1, xmm10
	unpcklps	xmm8, xmm9
	unpcklps	xmm10, xmm11
	unpckhps	xmm0, xmm9
	unpckhps	xmm1, xmm11
	movaps		xmm2, xmm8
	movaps		xmm3, xmm0
	movlhps		xmm8, xmm10
	movhlps		xmm10, xmm2
	movlhps		xmm0, xmm1
	movhlps		xmm1, xmm3
	subps		xmm8, xmm10
	subps		xmm0, xmm1
	addps		xmm0, xmm8
	
	movups		xmm1, [ARG2]
	movups		xmm2, 16[ARG2]
	mulps		xmm0, XMMREG_SCALE
	shufps		xmm1, xmm2, HEX(dd)
	movaps		xmm2, xmm0
	unpcklps	xmm0, xmm1
	unpckhps	xmm2, xmm1
	movups		[ARG2], xmm0
	movups		[ARG2+16], xmm2
	
	lea		ARG2, [ARG2+32]
	dec		ecx
	jnz			loop1
	
	mov		ecx, 4
	
	ALIGN16
loop2:
	movups		xmm8, [ARG0]
	movups		xmm1, 16[ARG0]
	movups		xmm2, 32[ARG0]
	movups		xmm3, 48[ARG0]
	movups		xmm9, 128[ARG0]
	movups		xmm5, 144[ARG0]
	movups		xmm6, 160[ARG0]
	movups		xmm7, 176[ARG0]
	mulps		xmm8, [ARG1]
	mulps		xmm1, 16[ARG1]
	mulps		xmm2, 32[ARG1]
	mulps		xmm3, 48[ARG1]
	mulps		xmm9, [ARG1-64]
	mulps		xmm5, [ARG1-48]
	mulps		xmm6, [ARG1-32]
	mulps		xmm7, [ARG1-16]
	
	addps		xmm8, xmm1
	addps		xmm3, xmm2
	addps		xmm9, xmm5
	addps		xmm6, xmm7
	addps		xmm8, xmm3
	addps		xmm9, xmm6
	lea		ARG0, [ARG0+256]
	lea		ARG1, [ARG1-128]
	
	movups		xmm10, [ARG0]
	movups		xmm1, [ARG0+16]
	movups		xmm2, [ARG0+32]
	movups		xmm3, [ARG0+48]
	movups		xmm11, [ARG0+128]
	movups		xmm5, [ARG0+144]
	movups		xmm6, [ARG0+160]
	movups		xmm7, [ARG0+176]
	mulps		xmm10, [ARG1]
	mulps		xmm1, [ARG1+16]
	mulps		xmm2, [ARG1+32]
	mulps		xmm3, [ARG1+48]
	mulps		xmm11, [ARG1-64]
	mulps		xmm5, [ARG1-48]
	mulps		xmm6, [ARG1-32]
	mulps		xmm7, [ARG1-16]
	
	addps		xmm10, xmm1
	addps		xmm3, xmm2
	addps		xmm11, xmm5
	addps		xmm6, xmm7
	addps		xmm10, xmm3
	addps		xmm11, xmm6
	lea		ARG0, 256[ARG0]
	lea		ARG1, -128[ARG1]
	
	movaps		xmm0, xmm8
	movaps		xmm1, xmm10
	unpcklps	xmm8, xmm9
	unpcklps	xmm10, xmm11
	unpckhps	xmm0, xmm9
	unpckhps	xmm1, xmm11
	movaps		xmm2, xmm8
	movaps		xmm3, xmm0
	movlhps		xmm8, xmm10
	movhlps		xmm10, xmm2
	movlhps		xmm0, xmm1
	movhlps		xmm1, xmm3
	addps		xmm8, xmm10
	addps		xmm0, xmm1
	addps		xmm0, xmm8
	
	movups		xmm1, [ARG2]
	movups		xmm2, 16[ARG2]
	mulps		xmm0, XMMREG_SCALE
	shufps		xmm1, xmm2, HEX(dd)
	movaps		xmm2, xmm0
	unpcklps	xmm0, xmm1
	unpckhps	xmm2, xmm1
	movups		[ARG2], xmm0
	movups		16[ARG2], xmm2
	
	lea		ARG2, 32[ARG2]
	dec		ecx
	jnz			loop2
	
	xor		eax, eax
	
#ifdef IS_MSABI
	movaps		xmm6, [rsp]
	movaps		xmm7, 16[rsp]
	movaps		xmm8, 32[rsp]
	movaps		xmm9, 48[rsp]
	movaps		xmm10, 64[rsp]
	movaps		xmm11, 80[rsp]
	movaps		xmm15, 96[rsp]
	add		rsp, 120
#endif
	ret

NONEXEC_STACK

END_MODULE
