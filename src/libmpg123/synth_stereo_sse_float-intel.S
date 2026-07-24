#include "gas_nasm_compat.h"
/*
	synth_sse_float: SSE optimized synth (stereo specific, float output version)

	copyright 1995-2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "mangle.h"

/* real *window; */
#define WINDOW ebx
/* real *b0l; */
#define B0L edx
/* real *b0r; */
#define B0R esi
/* real *samples; */
#define SAMPLES edi

#define TEMP(n) [esp+12+16*n]

/*
	int synth_1to1_real_s_sse_asm(real *window, real *b0l, real *b0r, real *samples, int bo1);
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
	ALIGN_CODE(16)
GLOBAL ASM_NAME(INT123_synth_1to1_real_s_sse_asm)
ASM_NAME(INT123_synth_1to1_real_s_sse_asm):
	push		ebp
	mov		ebp, esp
	and		esp, -16
	sub		esp, 128
	push		ebx
	push		esi
	push		edi
	
	mov		WINDOW, DWORD_PTR [ebp+8]
	mov		B0L, DWORD_PTR [ebp+12]
	mov		B0R, DWORD_PTR [ebp+16]
	mov		SAMPLES, DWORD_PTR [ebp+20]
	mov		eax, DWORD_PTR [ebp+24]
	shl		eax, 2
	
	lea		WINDOW, DWORD_PTR [WINDOW+64]
	sub		WINDOW, eax

	#undef _EBX_
	#define _EBX_ eax
	GET_GOT

	mov		ecx, 4
	
	ALIGN_CODE(16)
loop1:
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, [B0L+0]
	mulps		xmm1, [B0L+16]
	mulps		xmm2, [B0L+32]
	mulps		xmm3, [B0L+48]
	mulps		xmm4, [B0R+0]
	mulps		xmm5, [B0R+16]
	mulps		xmm6, [B0R+32]
	mulps		xmm7, [B0R+48]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP(0), xmm0
	movaps		TEMP(4), xmm4
	
	lea		WINDOW, DWORD_PTR [WINDOW+128]
	lea		B0L, DWORD_PTR [B0L+64]
	lea		B0R, DWORD_PTR [B0R+64]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, [B0L+0]
	mulps		xmm1, [B0L+16]
	mulps		xmm2, [B0L+32]
	mulps		xmm3, [B0L+48]
	mulps		xmm4, [B0R+0]
	mulps		xmm5, [B0R+16]
	mulps		xmm6, [B0R+32]
	mulps		xmm7, [B0R+48]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP(1), xmm0
	movaps		TEMP(5), xmm4
	
	lea		WINDOW, DWORD_PTR [WINDOW+128]
	lea		B0L, DWORD_PTR [B0L+64]
	lea		B0R, DWORD_PTR [B0R+64]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, [B0L+0]
	mulps		xmm1, [B0L+16]
	mulps		xmm2, [B0L+32]
	mulps		xmm3, [B0L+48]
	mulps		xmm4, [B0R+0]
	mulps		xmm5, [B0R+16]
	mulps		xmm6, [B0R+32]
	mulps		xmm7, [B0R+48]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP(2), xmm0
	movaps		TEMP(6), xmm4
	
	lea		WINDOW, DWORD_PTR [WINDOW+128]
	lea		B0L, DWORD_PTR [B0L+64]
	lea		B0R, DWORD_PTR [B0R+64]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, [B0L+0]
	mulps		xmm1, [B0L+16]
	mulps		xmm2, [B0L+32]
	mulps		xmm3, [B0L+48]
	mulps		xmm4, [B0R+0]
	mulps		xmm5, [B0R+16]
	mulps		xmm6, [B0R+32]
	mulps		xmm7, [B0R+48]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		xmm7, xmm0
	movaps		TEMP(7), xmm4
	
	lea		WINDOW, DWORD_PTR [WINDOW+128]
	lea		B0L, DWORD_PTR [B0L+64]
	lea		B0R, DWORD_PTR [B0R+64]
	
	movaps		xmm4, TEMP(0)
	movaps		xmm5, TEMP(1)
	movaps		xmm6, TEMP(2)
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
	movaps		xmm2, xmm0
	
	movaps		xmm4, TEMP(4)
	movaps		xmm5, TEMP(5)
	movaps		xmm6, TEMP(6)
	movaps		xmm7, TEMP(7)
	movaps		xmm0, xmm4
	movaps		xmm1, xmm6
	unpcklps	xmm4, xmm5
	unpcklps	xmm6, xmm7
	unpckhps	xmm0, xmm5
	unpckhps	xmm1, xmm7
	movaps		xmm5, xmm2
	movaps		xmm2, xmm4
	movaps		xmm3, xmm0
	movlhps		xmm4, xmm6
	movhlps		xmm6, xmm2
	movlhps		xmm0, xmm1
	movhlps		xmm1, xmm3
	subps		xmm4, xmm6
	subps		xmm0, xmm1
	addps		xmm0, xmm4
	
	mulps		xmm5, XMMWORD_PTR [LOCAL_VAR(scale_sse)]
	mulps		xmm0, XMMWORD_PTR [LOCAL_VAR(scale_sse)]
	movaps		xmm1, xmm5
	unpcklps	xmm5, xmm0
	unpckhps	xmm1, xmm0
	movups		[SAMPLES], xmm5
	movups		[SAMPLES+16], xmm1
	
	lea		SAMPLES, DWORD_PTR [SAMPLES+32]
	dec		ecx
	jnz			loop1
	
	mov		ecx, 4
	
	ALIGN_CODE(16)
loop2:
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, [B0L+0]
	mulps		xmm1, [B0L+16]
	mulps		xmm2, [B0L+32]
	mulps		xmm3, [B0L+48]
	mulps		xmm4, [B0R+0]
	mulps		xmm5, [B0R+16]
	mulps		xmm6, [B0R+32]
	mulps		xmm7, [B0R+48]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP(0), xmm0
	movaps		TEMP(4), xmm4
	
	lea		WINDOW, DWORD_PTR [WINDOW+128]
	lea		B0L, DWORD_PTR [B0L-64]
	lea		B0R, DWORD_PTR [B0R-64]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, [B0L+0]
	mulps		xmm1, [B0L+16]
	mulps		xmm2, [B0L+32]
	mulps		xmm3, [B0L+48]
	mulps		xmm4, [B0R+0]
	mulps		xmm5, [B0R+16]
	mulps		xmm6, [B0R+32]
	mulps		xmm7, [B0R+48]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP(1), xmm0
	movaps		TEMP(5), xmm4
	
	lea		WINDOW, DWORD_PTR [WINDOW+128]
	lea		B0L, DWORD_PTR [B0L-64]
	lea		B0R, DWORD_PTR [B0R-64]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, [B0L+0]
	mulps		xmm1, [B0L+16]
	mulps		xmm2, [B0L+32]
	mulps		xmm3, [B0L+48]
	mulps		xmm4, [B0R+0]
	mulps		xmm5, [B0R+16]
	mulps		xmm6, [B0R+32]
	mulps		xmm7, [B0R+48]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP(2), xmm0
	movaps		TEMP(6), xmm4
	
	lea		WINDOW, DWORD_PTR [WINDOW+128]
	lea		B0L, DWORD_PTR [B0L-64]
	lea		B0R, DWORD_PTR [B0R-64]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, [WINDOW+16]
	movups		xmm2, [WINDOW+32]
	movups		xmm3, [WINDOW+48]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, [B0L+0]
	mulps		xmm1, [B0L+16]
	mulps		xmm2, [B0L+32]
	mulps		xmm3, [B0L+48]
	mulps		xmm4, [B0R+0]
	mulps		xmm5, [B0R+16]
	mulps		xmm6, [B0R+32]
	mulps		xmm7, [B0R+48]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		xmm7, xmm0
	movaps		TEMP(7), xmm4
	
	lea		WINDOW, DWORD_PTR [WINDOW+128]
	lea		B0L, DWORD_PTR [B0L-64]
	lea		B0R, DWORD_PTR [B0R-64]
	
	movaps		xmm4, TEMP(0)
	movaps		xmm5, TEMP(1)
	movaps		xmm6, TEMP(2)
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
	movaps		xmm2, xmm0
	
	movaps		xmm4, TEMP(4)
	movaps		xmm5, TEMP(5)
	movaps		xmm6, TEMP(6)
	movaps		xmm7, TEMP(7)
	movaps		xmm0, xmm4
	movaps		xmm1, xmm6
	unpcklps	xmm4, xmm5
	unpcklps	xmm6, xmm7
	unpckhps	xmm0, xmm5
	unpckhps	xmm1, xmm7
	movaps		xmm5, xmm2
	movaps		xmm2, xmm4
	movaps		xmm3, xmm0
	movlhps		xmm4, xmm6
	movhlps		xmm6, xmm2
	movlhps		xmm0, xmm1
	movhlps		xmm1, xmm3
	addps		xmm4, xmm6
	addps		xmm0, xmm1
	addps		xmm0, xmm4
	
	mulps		xmm5, XMMWORD_PTR [LOCAL_VAR(scale_sse)]
	mulps		xmm0, XMMWORD_PTR [LOCAL_VAR(scale_sse)]
	movaps		xmm1, xmm5
	unpcklps	xmm5, xmm0
	unpckhps	xmm1, xmm0
	movups		[SAMPLES], xmm5
	movups		[SAMPLES+16], xmm1
	
	lea		SAMPLES, DWORD_PTR [SAMPLES+32]
	dec		ecx
	jnz			loop2
	
	xor		eax, eax
	
	pop		edi
	pop		esi
	pop		ebx
	mov		esp, ebp
	pop		ebp
	
	ret

NONEXEC_STACK

END_MODULE
