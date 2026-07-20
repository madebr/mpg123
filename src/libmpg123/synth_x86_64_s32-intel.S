#include "gas_nasm_compat.h"
/*
	synth_x86_64_s32: SSE optimized synth for x86-64 (s32 output version)

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

#define XMMREG_SCALE %xmm15  /* {65536.0, 65536.0, 65536.0, 65536.0} */
#define XMMREG_MAX %xmm14  /* {32767.999, 32767.999, 32767.999, 32767.999} */
#define XMMREG_MIN %xmm13  /* {-32768.0, -32768.0, -32768.0, -32768.0} */
#define XMMREG_CLIP %xmm12

/*
	int synth_1to1_s32_x86_64_asm(real *window, real *b0, int32_t *samples, int bo1);
	return value: number of clipped samples
*/

	RODATA
	ALIGN32
ASM_NAME(INT123_scale_s32):
	DATA_LONG   1199570944
	DATA_LONG   1199570944
	DATA_LONG   1199570944
	DATA_LONG   1199570944
	ALIGN16
ASM_NAME(INT123_maxmin_s32):
	DATA_LONG   1191182335
	DATA_LONG   1191182335
	DATA_LONG   1191182335
	DATA_LONG   1191182335
	DATA_LONG   -956301312
	DATA_LONG   -956301312
	DATA_LONG   -956301312
	DATA_LONG   -956301312
	.text
	ALIGN16
.globl ASM_NAME(INT123_synth_1to1_s32_x86_64_asm)
ASM_NAME(INT123_synth_1to1_s32_x86_64_asm):
#ifdef IS_MSABI /* should save xmm6-15 */
	mov		ARG0, rcx
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
	
	lea		rax, qword ptr ASM_NAME(INT123_scale_s32)[rip]
	movaps		XMMREG_SCALE, [rax]
	lea		rax, qword ptr ASM_NAME(INT123_maxmin_s32)[rip]
	movaps		XMMREG_MAX, [rax]
	movaps		XMMREG_MIN, 16[rax]
	
	xorps		XMMREG_CLIP, XMMREG_CLIP
	
	and		ARG3, 0xf
	shl		ARG3, 2
	lea		ARG0, qword ptr 64[ARG0]
	sub		ARG0, ARG3

	mov		ecx, 4
	
	ALIGN16
1:
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
	mulps		xmm9, 64[ARG1]
	mulps		xmm5, 80[ARG1]
	mulps		xmm6, 96[ARG1]
	mulps		xmm7, 112[ARG1]
	
	addps		xmm8, xmm1
	addps		xmm3, xmm2
	addps		xmm9, xmm5
	addps		xmm6, xmm7
	addps		xmm8, xmm3
	addps		xmm9, xmm6
	lea		ARG0, qword ptr 256[ARG0]
	lea		ARG1, qword ptr 128[ARG1]
	
	movups		xmm10, [ARG0]
	movups		xmm1, 16[ARG0]
	movups		xmm2, 32[ARG0]
	movups		xmm3, 48[ARG0]
	movups		xmm11, 128[ARG0]
	movups		xmm5, 144[ARG0]
	movups		xmm6, 160[ARG0]
	movups		xmm7, 176[ARG0]
	mulps		xmm10, [ARG1]
	mulps		xmm1, 16[ARG1]
	mulps		xmm2, 32[ARG1]
	mulps		xmm3, 48[ARG1]
	mulps		xmm11, 64[ARG1]
	mulps		xmm5, 80[ARG1]
	mulps		xmm6, 96[ARG1]
	mulps		xmm7, 112[ARG1]
	
	addps		xmm10, xmm1
	addps		xmm3, xmm2
	addps		xmm11, xmm5
	addps		xmm6, xmm7
	addps		xmm10, xmm3
	addps		xmm11, xmm6
	lea		ARG0, qword ptr 256[ARG0]
	lea		ARG1, qword ptr 128[ARG1]
	
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
	movaps		xmm3, xmm0
	movaps		xmm4, xmm0
	mulps		xmm0, XMMREG_SCALE
	cmpnleps	xmm3, XMMREG_MAX
	cmpltps		xmm4, XMMREG_MIN
	cvtps2dq	xmm0, xmm0
	xorps		xmm0, xmm3
	shufps		xmm1, xmm2, 0xdd
	movaps		xmm2, xmm0
	unpcklps	xmm0, xmm1
	unpckhps	xmm2, xmm1
	movups		[ARG2], xmm0
	movups		16[ARG2], xmm2
	
	psrld		xmm3, 31
	psrld		xmm4, 31
	paddd		xmm3, xmm4
	paddd		XMMREG_CLIP, xmm3
	
	lea		ARG2, qword ptr 32[ARG2]
	dec		ecx
	jnz			1b
	
	mov		ecx, 4
	
	ALIGN16
1:
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
	mulps		xmm9, -64[ARG1]
	mulps		xmm5, -48[ARG1]
	mulps		xmm6, -32[ARG1]
	mulps		xmm7, -16[ARG1]
	
	addps		xmm8, xmm1
	addps		xmm3, xmm2
	addps		xmm9, xmm5
	addps		xmm6, xmm7
	addps		xmm8, xmm3
	addps		xmm9, xmm6
	lea		ARG0, qword ptr 256[ARG0]
	lea		ARG1, qword ptr -128[ARG1]
	
	movups		xmm10, [ARG0]
	movups		xmm1, 16[ARG0]
	movups		xmm2, 32[ARG0]
	movups		xmm3, 48[ARG0]
	movups		xmm11, 128[ARG0]
	movups		xmm5, 144[ARG0]
	movups		xmm6, 160[ARG0]
	movups		xmm7, 176[ARG0]
	mulps		xmm10, [ARG1]
	mulps		xmm1, 16[ARG1]
	mulps		xmm2, 32[ARG1]
	mulps		xmm3, 48[ARG1]
	mulps		xmm11, -64[ARG1]
	mulps		xmm5, -48[ARG1]
	mulps		xmm6, -32[ARG1]
	mulps		xmm7, -16[ARG1]
	
	addps		xmm10, xmm1
	addps		xmm3, xmm2
	addps		xmm11, xmm5
	addps		xmm6, xmm7
	addps		xmm10, xmm3
	addps		xmm11, xmm6
	lea		ARG0, qword ptr 256[ARG0]
	lea		ARG1, qword ptr -128[ARG1]
	
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
	movaps		xmm3, xmm0
	movaps		xmm4, xmm0
	mulps		xmm0, XMMREG_SCALE
	cmpnleps	xmm3, XMMREG_MAX
	cmpltps		xmm4, XMMREG_MIN
	cvtps2dq	xmm0, xmm0
	xorps		xmm0, xmm3
	shufps		xmm1, xmm2, 0xdd
	movaps		xmm2, xmm0
	unpcklps	xmm0, xmm1
	unpckhps	xmm2, xmm1
	movups		[ARG2], xmm0
	movups		16[ARG2], xmm2
	
	psrld		xmm3, 31
	psrld		xmm4, 31
	paddd		xmm3, xmm4
	paddd		XMMREG_CLIP, xmm3
	
	lea		ARG2, qword ptr 32[ARG2]
	dec		ecx
	jnz			1b
	
	pshuflw		xmm0, XMMREG_CLIP, 0xee
	movhlps		xmm1, XMMREG_CLIP
	pshuflw		xmm2, xmm1, 0xee
	paddd		XMMREG_CLIP, xmm0
	paddd		XMMREG_CLIP, xmm1
	paddd		XMMREG_CLIP, xmm2
	
	movd		eax, XMMREG_CLIP
	
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
