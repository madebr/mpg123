#include "gas_nasm_compat.h"
/*
	synth_sse_s32: SSE optimized synth (s32 output version)

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

#define MMREG_CLIP mm7

/*
	int synth_1to1_s32_sse_asm(real *window, real *b0, int32_t *samples, int bo1);
	return value: number of clipped samples
*/

	RODATA
	ALIGN32
scale_s32:
	DATA_LONG   1199570944 /* 65536.0 */
	DATA_LONG   1199570944
	DATA_LONG   1199570944
	DATA_LONG   1199570944
	ALIGN16
maxmin_s32:
	DATA_LONG   1191182335 /* 32767.999 */
	DATA_LONG   1191182335
	DATA_LONG   1191182335
	DATA_LONG   1191182335
	DATA_LONG   -956301312 /* -32768.0 */
	DATA_LONG   -956301312
	DATA_LONG   -956301312
	DATA_LONG   -956301312
	MMX
	FLAT_MODEL
	SECTION_TEXT
	//ALIGN16
GLOBAL ASM_NAME(INT123_synth_1to1_s32_sse_asm)
ASM_NAME(INT123_synth_1to1_s32_sse_asm):
	push		ebp
	mov		ebp, esp
	push		ebx
	push		esi

	pxor		MMREG_CLIP, MMREG_CLIP
	
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
	
	//ALIGN16
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
	
	lea		WINDOW, DWORD_PTR 256[WINDOW]
	lea		B0, DWORD_PTR 128[B0]
	
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
	
	movaps		xmm1, xmm0
	movaps		xmm2, xmm0
	mulps		xmm0, XMMWORD_PTR [LOCAL_VAR(scale_s32)]
	cmpnleps	xmm1, XMMWORD_PTR [LOCAL_VAR(maxmin_s32)]
	cmpltps		xmm2, XMMWORD_PTR [16+LOCAL_VAR(maxmin_s32)]
	cvtps2pi	mm0, xmm0
	movhlps		xmm0, xmm0
	cvtps2pi	mm1, xmm0
	cvtps2pi	mm2, xmm1
	movhlps		xmm1, xmm1
	cvtps2pi	mm3, xmm1
	psrad		mm2, 31
	psrad		mm3, 31
	pxor		mm0, mm2
	pxor		mm1, mm3
	movd		DWORD_PTR [SAMPLES], mm0
	psrlq		mm0, 32
	movd		DWORD_PTR [SAMPLES+8], mm0
	movd		DWORD_PTR [SAMPLES+16], mm1
	psrlq		mm1, 32
	movd		DWORD_PTR [SAMPLES+24], mm1
	
	cvtps2pi	mm0, xmm2
	movhlps		xmm2, xmm2
	cvtps2pi	mm1, xmm2
	packssdw	mm2, mm3
	packssdw	mm0, mm1
	psrlw		mm2, 15
	psrlw		mm0, 15
	paddw		mm0, mm2
	paddw		MMREG_CLIP, mm0
	
	lea		SAMPLES, DWORD_PTR 32[SAMPLES]
	dec		ecx
	jnz			loop1
	
	mov		ecx, 4
	
	//ALIGN16
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
	
	movaps		xmm1, xmm0
	movaps		xmm2, xmm0
	mulps		xmm0, XMMWORD_PTR [LOCAL_VAR(scale_s32)]
	cmpnleps	xmm1, XMMWORD_PTR [LOCAL_VAR(maxmin_s32)]
	cmpltps		xmm2, XMMWORD_PTR [LOCAL_VAR(maxmin_s32)+16]
	cvtps2pi	mm0, xmm0
	movhlps		xmm0, xmm0
	cvtps2pi	mm1, xmm0
	cvtps2pi	mm2, xmm1
	movhlps		xmm1, xmm1
	cvtps2pi	mm3, xmm1
	psrad		mm2, 31
	psrad		mm3, 31
	pxor		mm0, mm2
	pxor		mm1, mm3
	movd		DWORD_PTR [SAMPLES], mm0
	psrlq		mm0, 32
	movd		DWORD_PTR [SAMPLES+8], mm0
	movd		DWORD_PTR [SAMPLES+16], mm1
	psrlq		mm1, 32
	movd		DWORD_PTR [SAMPLES+24], mm1
	
	cvtps2pi	mm0, xmm2
	movhlps		xmm2, xmm2
	cvtps2pi	mm1, xmm2
	packssdw	mm2, mm3
	packssdw	mm0, mm1
	psrlw		mm2, 15
	psrlw		mm0, 15
	paddw		mm0, mm2
	paddw		MMREG_CLIP, mm0
	
	lea		SAMPLES, DWORD_PTR 32[SAMPLES]
	dec		ecx
	jnz			loop2
	
	pshufw		mm0, MMREG_CLIP, HEX(ee)
	paddw		mm0, MMREG_CLIP
	pshufw		mm1, mm0, HEX(55)
	paddw		mm0, mm1
	movd		eax, mm0
	and		eax, HEX(ffff)

	pop		esi
	pop		ebx
	mov		esp, ebp
	pop		ebp
	
	emms
	
	ret

NONEXEC_STACK

END_MODULE
