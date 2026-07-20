#include "gas_nasm_compat.h"
/*
	synth_stereo_x86_64_float: SSE optimized synth for x86-64 (stereo specific, float output version)

	copyright 1995-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "x86_64_defs.h"
#include "mangle.h"

#ifdef IS_MSABI
/* short *window; */
#define WINDOW r10
/* short *b0l; */
#define B0L rdx
/* short *b0r; */
#define B0R r8
/* short *samples; */
#define SAMPLES r9
#else
/* real *window; */
#define WINDOW rdi
/* real *b0l; */
#define B0L rsi
/* real *b0r; */
#define B0R rdx
/* real *samples; */
#define SAMPLES r9
#endif

#define XMMREG_SCALE [r11]  /* {1/32768.0, 1/32768.0, 1/32768.0, 1/32768.0} */

/*
	int synth_1to1_real_s_x86_64_asm(real *window, real *b0l, real *b0r, real *samples, int bo1);
	return value: number of clipped samples (0)
*/

	RODATA
	ALIGN32
ASM_NAME(INT123_scale_x86_64):
	DATA_LONG   939524096
	DATA_LONG   939524096
	DATA_LONG   939524096
	DATA_LONG   939524096
	.text
	ALIGN16
.globl ASM_NAME(INT123_synth_1to1_real_s_x86_64_asm)
ASM_NAME(INT123_synth_1to1_real_s_x86_64_asm):
#ifdef IS_MSABI /* should save xmm6-15 */
	mov		eax, dword ptr 40[rsp] /* 5th argument; placed after 32-byte shadow space */
	sub		rsp, 168 /* stack alignment + 10 xmm registers */
	movaps		[rsp], xmm6
	movaps		16[rsp], xmm7
	movaps		32[rsp], xmm8
	movaps		48[rsp], xmm9
	movaps		64[rsp], xmm10
	movaps		80[rsp], xmm11
	movaps		96[rsp], xmm12
	movaps		112[rsp], xmm13
	movaps		128[rsp], xmm14
	movaps		144[rsp], xmm15
#endif

	lea		r11, qword ptr ASM_NAME(INT123_scale_x86_64)[rip]
	
#ifdef IS_MSABI
	shl		rax, 32
	shr		rax, 30
	mov		r10, rcx
#else
	mov		rax, r8
	shl		rax, 32
	shr		rax, 30
	mov		r9, rcx
#endif
	lea		WINDOW, qword ptr 64[WINDOW]
	sub		WINDOW, rax

	mov		ecx, 4
	
	ALIGN16
1:
	movups		xmm8, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movups		xmm9, 128[WINDOW]
	movups		xmm5, 144[WINDOW]
	movups		xmm6, 160[WINDOW]
	movups		xmm7, 176[WINDOW]
	movaps		xmm0, xmm8
	movaps		xmm4, xmm1
	movaps		xmm10, xmm2
	movaps		xmm11, xmm3
	movaps		xmm12, xmm9
	movaps		xmm13, xmm5
	movaps		xmm14, xmm6
	movaps		xmm15, xmm7
	mulps		xmm8, [B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm9, 64[B0L]
	mulps		xmm5, 80[B0L]
	mulps		xmm6, 96[B0L]
	mulps		xmm7, 112[B0L]
	mulps		xmm0, [B0R]
	mulps		xmm4, 16[B0R]
	mulps		xmm10, 32[B0R]
	mulps		xmm11, 48[B0R]
	mulps		xmm12, 64[B0R]
	mulps		xmm13, 80[B0R]
	mulps		xmm14, 96[B0R]
	mulps		xmm15, 112[B0R]
	
	addps		xmm8, xmm1
	addps		xmm3, xmm2
	addps		xmm0, xmm4
	addps		xmm10, xmm11
	addps		xmm9, xmm5
	addps		xmm6, xmm7
	addps		xmm12, xmm13
	addps		xmm14, xmm15
	addps		xmm8, xmm3
	addps		xmm9, xmm6
	addps		xmm0, xmm10
	addps		xmm14, xmm12
	movaps		xmm12, xmm0
	movaps		xmm13, xmm14
	lea		WINDOW, qword ptr 256[WINDOW]
	lea		B0L, qword ptr 128[B0L]
	lea		B0R, qword ptr 128[B0R]
	
	movups		xmm10, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movups		xmm11, 128[WINDOW]
	movups		xmm5, 144[WINDOW]
	movups		xmm6, 160[WINDOW]
	movups		xmm7, 176[WINDOW]
	movaps		xmm0, xmm10
	movaps		xmm4, xmm1
	movaps		xmm14, xmm2
	movaps		xmm15, xmm3
	mulps		xmm10, [B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm0, [B0R]
	mulps		xmm4, 16[B0R]
	mulps		xmm14, 32[B0R]
	mulps		xmm15, 48[B0R]
	addps		xmm10, xmm1
	addps		xmm3, xmm2
	addps		xmm0, xmm4
	addps		xmm14, xmm15
	movaps		xmm1, xmm11
	movaps		xmm2, xmm5
	movaps		xmm4, xmm6
	movaps		xmm15, xmm7
	mulps		xmm11, 64[B0L]
	mulps		xmm5, 80[B0L]
	mulps		xmm6, 96[B0L]
	mulps		xmm7, 112[B0L]
	mulps		xmm1, 64[B0R]
	mulps		xmm2, 80[B0R]
	mulps		xmm4, 96[B0R]
	mulps		xmm15, 112[B0R]
	addps		xmm11, xmm5
	addps		xmm6, xmm7
	addps		xmm1, xmm2
	addps		xmm4, xmm15
	
	addps		xmm10, xmm3
	addps		xmm11, xmm6
	addps		xmm14, xmm0
	addps		xmm1, xmm4
	movaps		xmm15, xmm1
	lea		WINDOW, qword ptr 256[WINDOW]
	lea		B0L, qword ptr 128[B0L]
	lea		B0R, qword ptr 128[B0R]
	
	movaps		xmm0, xmm8
	movaps		xmm1, xmm10
	movaps		xmm4, xmm12
	movaps		xmm5, xmm14
	unpcklps	xmm8, xmm9
	unpcklps	xmm10, xmm11
	unpckhps	xmm0, xmm9
	unpckhps	xmm1, xmm11
	unpcklps	xmm12, xmm13
	unpcklps	xmm14, xmm15
	unpckhps	xmm4, xmm13
	unpckhps	xmm5, xmm15
	movaps		xmm2, xmm8
	movaps		xmm3, xmm0
	movaps		xmm6, xmm12
	movaps		xmm7, xmm4
	movlhps		xmm8, xmm10
	movhlps		xmm10, xmm2
	movlhps		xmm0, xmm1
	movhlps		xmm1, xmm3
	movlhps		xmm12, xmm14
	movhlps		xmm14, xmm6
	movlhps		xmm4, xmm5
	movhlps		xmm5, xmm7
	subps		xmm8, xmm10
	subps		xmm0, xmm1
	subps		xmm12, xmm14
	subps		xmm4, xmm5
	addps		xmm0, xmm8
	addps		xmm4, xmm12
	
	mulps		xmm0, XMMREG_SCALE
	mulps		xmm4, XMMREG_SCALE
	movaps		xmm1, xmm0
	unpcklps	xmm0, xmm4
	unpckhps	xmm1, xmm4
	movups		[SAMPLES], xmm0
	movups		16[SAMPLES], xmm1
	
	lea		SAMPLES, qword ptr 32[SAMPLES]
	dec		ecx
	jnz			1b
	
	mov		ecx, 4
	
	ALIGN16
1:
	movups		xmm8, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movups		xmm9, 128[WINDOW]
	movups		xmm5, 144[WINDOW]
	movups		xmm6, 160[WINDOW]
	movups		xmm7, 176[WINDOW]
	movaps		xmm0, xmm8
	movaps		xmm4, xmm1
	movaps		xmm10, xmm2
	movaps		xmm11, xmm3
	movaps		xmm12, xmm9
	movaps		xmm13, xmm5
	movaps		xmm14, xmm6
	movaps		xmm15, xmm7
	mulps		xmm8, [B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm9, -64[B0L]
	mulps		xmm5, -48[B0L]
	mulps		xmm6, -32[B0L]
	mulps		xmm7, -16[B0L]
	mulps		xmm0, [B0R]
	mulps		xmm4, 16[B0R]
	mulps		xmm10, 32[B0R]
	mulps		xmm11, 48[B0R]
	mulps		xmm12, -64[B0R]
	mulps		xmm13, -48[B0R]
	mulps		xmm14, -32[B0R]
	mulps		xmm15, -16[B0R]
	
	addps		xmm8, xmm1
	addps		xmm3, xmm2
	addps		xmm0, xmm4
	addps		xmm10, xmm11
	addps		xmm9, xmm5
	addps		xmm6, xmm7
	addps		xmm12, xmm13
	addps		xmm14, xmm15
	addps		xmm8, xmm3
	addps		xmm9, xmm6
	addps		xmm0, xmm10
	addps		xmm14, xmm12
	movaps		xmm12, xmm0
	movaps		xmm13, xmm14
	lea		WINDOW, qword ptr 256[WINDOW]
	lea		B0L, qword ptr -128[B0L]
	lea		B0R, qword ptr -128[B0R]
	
	movups		xmm10, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movups		xmm11, 128[WINDOW]
	movups		xmm5, 144[WINDOW]
	movups		xmm6, 160[WINDOW]
	movups		xmm7, 176[WINDOW]
	movaps		xmm0, xmm10
	movaps		xmm4, xmm1
	movaps		xmm14, xmm2
	movaps		xmm15, xmm3
	mulps		xmm10, [B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm0, [B0R]
	mulps		xmm4, 16[B0R]
	mulps		xmm14, 32[B0R]
	mulps		xmm15, 48[B0R]
	addps		xmm10, xmm1
	addps		xmm3, xmm2
	addps		xmm0, xmm4
	addps		xmm14, xmm15
	movaps		xmm1, xmm11
	movaps		xmm2, xmm5
	movaps		xmm4, xmm6
	movaps		xmm15, xmm7
	mulps		xmm11, -64[B0L]
	mulps		xmm5, -48[B0L]
	mulps		xmm6, -32[B0L]
	mulps		xmm7, -16[B0L]
	mulps		xmm1, -64[B0R]
	mulps		xmm2, -48[B0R]
	mulps		xmm4, -32[B0R]
	mulps		xmm15, -16[B0R]
	addps		xmm11, xmm5
	addps		xmm6, xmm7
	addps		xmm1, xmm2
	addps		xmm4, xmm15
	
	addps		xmm10, xmm3
	addps		xmm11, xmm6
	addps		xmm14, xmm0
	addps		xmm1, xmm4
	movaps		xmm15, xmm1
	lea		WINDOW, qword ptr 256[WINDOW]
	lea		B0L, qword ptr -128[B0L]
	lea		B0R, qword ptr -128[B0R]
	
	movaps		xmm0, xmm8
	movaps		xmm1, xmm10
	movaps		xmm4, xmm12
	movaps		xmm5, xmm14
	unpcklps	xmm8, xmm9
	unpcklps	xmm10, xmm11
	unpckhps	xmm0, xmm9
	unpckhps	xmm1, xmm11
	unpcklps	xmm12, xmm13
	unpcklps	xmm14, xmm15
	unpckhps	xmm4, xmm13
	unpckhps	xmm5, xmm15
	movaps		xmm2, xmm8
	movaps		xmm3, xmm0
	movaps		xmm6, xmm12
	movaps		xmm7, xmm4
	movlhps		xmm8, xmm10
	movhlps		xmm10, xmm2
	movlhps		xmm0, xmm1
	movhlps		xmm1, xmm3
	movlhps		xmm12, xmm14
	movhlps		xmm14, xmm6
	movlhps		xmm4, xmm5
	movhlps		xmm5, xmm7
	addps		xmm8, xmm10
	addps		xmm0, xmm1
	addps		xmm12, xmm14
	addps		xmm4, xmm5
	addps		xmm0, xmm8
	addps		xmm4, xmm12
	
	mulps		xmm0, XMMREG_SCALE
	mulps		xmm4, XMMREG_SCALE
	movaps		xmm1, xmm0
	unpcklps	xmm0, xmm4
	unpckhps	xmm1, xmm4
	movups		[SAMPLES], xmm0
	movups		16[SAMPLES], xmm1
	
	lea		SAMPLES, qword ptr 32[SAMPLES]
	dec		ecx
	jnz			1b
	
	xor		eax, eax
	
#ifdef IS_MSABI
	movaps		xmm6, [rsp]
	movaps		xmm7, 16[rsp]
	movaps		xmm8, 32[rsp]
	movaps		xmm9, 48[rsp]
	movaps		xmm10, 64[rsp]
	movaps		xmm11, 80[rsp]
	movaps		xmm12, 96[rsp]
	movaps		xmm13, 112[rsp]
	movaps		xmm14, 128[rsp]
	movaps		xmm15, 144[rsp]
	add		rsp, 168
#endif
	ret

NONEXEC_STACK
