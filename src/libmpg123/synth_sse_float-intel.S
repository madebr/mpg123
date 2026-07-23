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
	
	mov		WINDOW, DWORD_PTR 8[ebp]
	mov		B0, DWORD_PTR 12[ebp]
	mov		SAMPLES, DWORD_PTR 16[ebp]
	mov		eax, DWORD_PTR 20[ebp]
	shl		eax, 2
	
	lea		WINDOW, DWORD_PTR 64[WINDOW]
	sub		WINDOW, eax
	
	#undef _EBX_
	#define _EBX_ eax
	GET_GOT

	mov		ecx, 4
	
	ALIGN16
loop1:
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
	
	lea		WINDOW, DWORD_PTR 256[WINDOW]
	lea		B0, DWORD_PTR 128[B0]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movups		xmm6, [WINDOW+128]
	movups		xmm7, [WINDOW+144]
	mulps		xmm0, [B0]
	mulps		xmm1, [B0+16]
	mulps		xmm2, [B0+32]
	mulps		xmm3, [B0+48]
	mulps		xmm6, [B0+64]
	mulps		xmm7, [B0+80]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm6, xmm7
	movups		xmm1, [WINDOW+160]
	movups		xmm3, [WINDOW+176]
	mulps		xmm1, [B0+96]
	mulps		xmm3, [B0+112]
	addps		xmm0, xmm2
	addps		xmm1, xmm3
	addps		xmm6, xmm1
	movaps		xmm7, xmm6
	movaps		xmm6, xmm0
	
	lea		WINDOW, DWORD_PTR [WINDOW+256]
	lea		B0, DWORD_PTR [B0+128]
	
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
	movups		xmm2, [SAMPLES+16]
	mulps		xmm0, [LOCAL_VAR(scale_sse)]
	shufps		xmm1, xmm2, HEX(dd)
	movaps		xmm2, xmm0
	unpcklps	xmm0, xmm1
	unpckhps	xmm2, xmm1
	movups		[SAMPLES], xmm0
	movups		[SAMPLES+16], xmm2
	
	lea		SAMPLES, DWORD_PTR 32[SAMPLES]
	dec		ecx
	jnz			loop1
	
	mov		ecx, 4
	
	ALIGN16
loop2:
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movups		xmm4, [WINDOW+128]
	movups		xmm5, [WINDOW+144]
	movups		xmm6, [WINDOW+160]
	movups		xmm7, [WINDOW+176]
	mulps		xmm0, [B0+0]
	mulps		xmm1, [B0+16]
	mulps		xmm2, [B0+32]
	mulps		xmm3, [B0+48]
	mulps		xmm4, [B0-64]
	mulps		xmm5, [B0-48]
	mulps		xmm6, [B0-32]
	mulps		xmm7, [B0-16]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		xmm5, xmm4
	movaps		xmm4, xmm0
	
	lea		WINDOW, DWORD_PTR [WINDOW+256]
	lea		B0, DWORD_PTR [B0-128]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movups		xmm6, [WINDOW+128]
	movups		xmm7, [WINDOW+144]
	mulps		xmm0, [B0]
	mulps		xmm1, [B0+16]
	mulps		xmm2, [B0+32]
	mulps		xmm3, [B0+48]
	mulps		xmm6, [B0-64]
	mulps		xmm7, [B0-48]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm6, xmm7
	movups		xmm1, [WINDOW+160]
	movups		xmm3, [WINDOW+176]
	mulps		xmm1, [B0-32]
	mulps		xmm3, [B0-16]
	addps		xmm0, xmm2
	addps		xmm1, xmm3
	addps		xmm6, xmm1
	movaps		xmm7, xmm6
	movaps		xmm6, xmm0
	
	lea		WINDOW, DWORD_PTR [WINDOW+256]
	lea		B0, DWORD_PTR [B0-128]
	
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
	movups		xmm2, [SAMPLES+16]
	mulps		xmm0, [LOCAL_VAR(scale_sse)]
	shufps		xmm1, xmm2, HEX(dd)
	movaps		xmm2, xmm0
	unpcklps	xmm0, xmm1
	unpckhps	xmm2, xmm1
	movups		[SAMPLES], xmm0
	movups		16[SAMPLES], xmm2
	
	lea		SAMPLES, DWORD_PTR [SAMPLES+32]
	dec		ecx
	jnz			loop2
	
	xor		eax, eax

	pop		esi
	pop		ebx
	mov		esp, ebp
	pop		ebp
	
	ret

NONEXEC_STACK

END_MODULE
