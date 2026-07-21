#include "gas_nasm_compat.h"
/*
	synth_sse_float: SSE optimized synth (float output version)

	copyright 1995-2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "mangle.h"

/* real *window; */
#define WINDOW ebx
/* real *b0; */
#define B0 edx
/* real *samples; */
#define SAMPLES esi

/*
	int synth_1to1_real_sse_asm(real *window, real *b0, real *samples, int bo1);
	return value: number of clipped samples (0)
*/

	RODATA
	ALIGN32
scale_sse:
	DATA_LONG   939524096
	DATA_LONG   939524096
	DATA_LONG   939524096
	DATA_LONG   939524096
	SECTION_TEXT
	ALIGN16
GLOBAL ASM_NAME(INT123_synth_1to1_real_sse_asm)
ASM_NAME(INT123_synth_1to1_real_sse_asm):
	push		ebp
	mov		ebp, esp
	push		ebx
	push		esi
	
	mov		WINDOW, dword ptr 8[ebp]
	mov		B0, dword ptr 12[ebp]
	mov		SAMPLES, dword ptr 16[ebp]
	mov		eax, dword ptr 20[ebp]
	shl		eax, 2
	
	lea		WINDOW, dword ptr 64[WINDOW]
	sub		WINDOW, eax
	
	#undef _EBX_
	#define _EBX_ eax
	GET_GOT

	mov		ecx, 4
	
	ALIGN16
1:
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movups		xmm4, 128[WINDOW]
	movups		xmm5, 144[WINDOW]
	movups		xmm6, 160[WINDOW]
	movups		xmm7, 176[WINDOW]
	mulps		xmm0, 0[B0]
	mulps		xmm1, 16[B0]
	mulps		xmm2, 32[B0]
	mulps		xmm3, 48[B0]
	mulps		xmm4, 64[B0]
	mulps		xmm5, 80[B0]
	mulps		xmm6, 96[B0]
	mulps		xmm7, 112[B0]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		xmm5, xmm4
	movaps		xmm4, xmm0
	
	lea		WINDOW, dword ptr 256[WINDOW]
	lea		B0, dword ptr 128[B0]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movups		xmm6, 128[WINDOW]
	movups		xmm7, 144[WINDOW]
	mulps		xmm0, [B0]
	mulps		xmm1, 16[B0]
	mulps		xmm2, 32[B0]
	mulps		xmm3, 48[B0]
	mulps		xmm6, 64[B0]
	mulps		xmm7, 80[B0]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm6, xmm7
	movups		xmm1, 160[WINDOW]
	movups		xmm3, 176[WINDOW]
	mulps		xmm1, 96[B0]
	mulps		xmm3, 112[B0]
	addps		xmm0, xmm2
	addps		xmm1, xmm3
	addps		xmm6, xmm1
	movaps		xmm7, xmm6
	movaps		xmm6, xmm0
	
	lea		WINDOW, dword ptr 256[WINDOW]
	lea		B0, dword ptr 128[B0]
	
	movaps		xmm0, xmm4
	movaps		xmm1, xmm6
	unpcklps	xmm4, xmm5
	unpcklps	xmm6, xmm7
	unpckhps	xmm0, xmm5
	unpckhps	xmm1, xmm7
	movaps		xmm2, xmm4
	movaps		xmm3, xmm0
	movlhps		xmm4, xmm6
	movhlps		xmm6, xmm2
	movlhps		xmm0, xmm1
	movhlps		xmm1, xmm3
	subps		xmm4, xmm6
	subps		xmm0, xmm1
	addps		xmm0, xmm4
	
	movups		xmm1, [SAMPLES]
	movups		xmm2, 16[SAMPLES]
	mulps		xmm0, LOCAL_VAR[scale_sse]
	shufps		xmm1, xmm2, 0xdd
	movaps		xmm2, xmm0
	unpcklps	xmm0, xmm1
	unpckhps	xmm2, xmm1
	movups		[SAMPLES], xmm0
	movups		16[SAMPLES], xmm2
	
	lea		SAMPLES, dword ptr 32[SAMPLES]
	dec		ecx
	jnz			1b
	
	mov		ecx, 4
	
	ALIGN16
1:
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movups		xmm4, 128[WINDOW]
	movups		xmm5, 144[WINDOW]
	movups		xmm6, 160[WINDOW]
	movups		xmm7, 176[WINDOW]
	mulps		xmm0, 0[B0]
	mulps		xmm1, 16[B0]
	mulps		xmm2, 32[B0]
	mulps		xmm3, 48[B0]
	mulps		xmm4, -64[B0]
	mulps		xmm5, -48[B0]
	mulps		xmm6, -32[B0]
	mulps		xmm7, -16[B0]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		xmm5, xmm4
	movaps		xmm4, xmm0
	
	lea		WINDOW, dword ptr 256[WINDOW]
	lea		B0, dword ptr -128[B0]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movups		xmm6, 128[WINDOW]
	movups		xmm7, 144[WINDOW]
	mulps		xmm0, [B0]
	mulps		xmm1, 16[B0]
	mulps		xmm2, 32[B0]
	mulps		xmm3, 48[B0]
	mulps		xmm6, -64[B0]
	mulps		xmm7, -48[B0]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm6, xmm7
	movups		xmm1, 160[WINDOW]
	movups		xmm3, 176[WINDOW]
	mulps		xmm1, -32[B0]
	mulps		xmm3, -16[B0]
	addps		xmm0, xmm2
	addps		xmm1, xmm3
	addps		xmm6, xmm1
	movaps		xmm7, xmm6
	movaps		xmm6, xmm0
	
	lea		WINDOW, dword ptr 256[WINDOW]
	lea		B0, dword ptr -128[B0]
	
	movaps		xmm0, xmm4
	movaps		xmm1, xmm6
	unpcklps	xmm4, xmm5
	unpcklps	xmm6, xmm7
	unpckhps	xmm0, xmm5
	unpckhps	xmm1, xmm7
	movaps		xmm2, xmm4
	movaps		xmm3, xmm0
	movlhps		xmm4, xmm6
	movhlps		xmm6, xmm2
	movlhps		xmm0, xmm1
	movhlps		xmm1, xmm3
	addps		xmm4, xmm6
	addps		xmm0, xmm1
	addps		xmm0, xmm4
	
	movups		xmm1, [SAMPLES]
	movups		xmm2, 16[SAMPLES]
	mulps		xmm0, LOCAL_VAR[scale_sse]
	shufps		xmm1, xmm2, 0xdd
	movaps		xmm2, xmm0
	unpcklps	xmm0, xmm1
	unpckhps	xmm2, xmm1
	movups		[SAMPLES], xmm0
	movups		16[SAMPLES], xmm2
	
	lea		SAMPLES, dword ptr 32[SAMPLES]
	dec		ecx
	jnz			1b
	
	xor		eax, eax

	pop		esi
	pop		ebx
	mov		esp, ebp
	pop		ebp
	
	ret

NONEXEC_STACK
