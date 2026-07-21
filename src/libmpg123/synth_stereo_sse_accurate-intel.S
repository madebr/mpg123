#include "gas_nasm_compat.h"
/*
	synth_stereo_sse_accurate: SSE optimized synth (stereo specific, MPEG-compliant 16bit output version)

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

#define TEMP(n) (12+16*n)(esp)
#define MMREG_CLIP mm7

/*
	int synth_1to1_s_sse_accurate_asm(real *window, real *b0l, real *b0r, short *samples, int bo1);
	return value: number of clipped samples
*/

	RODATA
	ALIGN32
maxmin_s16:
	DATA_LONG   1191181824 /* 32767.0 */
	DATA_LONG   1191181824
	DATA_LONG   1191181824
	DATA_LONG   1191181824
	DATA_LONG   -956301312 /* -32768.0 */
	DATA_LONG   -956301312
	DATA_LONG   -956301312
	DATA_LONG   -956301312
	SECTION_TEXT
	ALIGN16
GLOBAL ASM_NAME(INT123_synth_1to1_s_sse_accurate_asm)
ASM_NAME(INT123_synth_1to1_s_sse_accurate_asm):
	push		ebp
	mov		ebp, esp
	and		esp, -16
	sub		esp, 128
	push		ebx
	push		esi
	push		edi
	
	pxor		MMREG_CLIP, MMREG_CLIP
	
	mov		WINDOW, dword ptr 8[ebp]
	mov		B0L, dword ptr 12[ebp]
	mov		B0R, dword ptr 16[ebp]
	mov		SAMPLES, dword ptr 20[ebp]
	mov		eax, dword ptr 24[ebp]
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
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, 0[B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm4, 0[B0R]
	mulps		xmm5, 16[B0R]
	mulps		xmm6, 32[B0R]
	mulps		xmm7, 48[B0R]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP[0], xmm0
	movaps		TEMP[4], xmm4
	
	lea		WINDOW, dword ptr 128[WINDOW]
	lea		B0L, dword ptr 64[B0L]
	lea		B0R, dword ptr 64[B0R]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, 0[B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm4, 0[B0R]
	mulps		xmm5, 16[B0R]
	mulps		xmm6, 32[B0R]
	mulps		xmm7, 48[B0R]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP[1], xmm0
	movaps		TEMP[5], xmm4
	
	lea		WINDOW, dword ptr 128[WINDOW]
	lea		B0L, dword ptr 64[B0L]
	lea		B0R, dword ptr 64[B0R]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, 0[B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm4, 0[B0R]
	mulps		xmm5, 16[B0R]
	mulps		xmm6, 32[B0R]
	mulps		xmm7, 48[B0R]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP[2], xmm0
	movaps		TEMP[6], xmm4
	
	lea		WINDOW, dword ptr 128[WINDOW]
	lea		B0L, dword ptr 64[B0L]
	lea		B0R, dword ptr 64[B0R]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, 0[B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm4, 0[B0R]
	mulps		xmm5, 16[B0R]
	mulps		xmm6, 32[B0R]
	mulps		xmm7, 48[B0R]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		xmm7, xmm0
	movaps		TEMP[7], xmm4
	
	lea		WINDOW, dword ptr 128[WINDOW]
	lea		B0L, dword ptr 64[B0L]
	lea		B0R, dword ptr 64[B0R]
	
	movaps		xmm4, TEMP[0]
	movaps		xmm5, TEMP[1]
	movaps		xmm6, TEMP[2]
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
	
	movaps		xmm4, TEMP[4]
	movaps		xmm5, TEMP[5]
	movaps		xmm6, TEMP[6]
	movaps		xmm7, TEMP[7]
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
	
	movaps		xmm1, xmm5
	movaps		xmm2, xmm5
	movaps		xmm3, xmm0
	movaps		xmm4, xmm0
	cmpnleps	xmm1, LOCAL_VAR[maxmin_s16]
	cmpltps		xmm2, 16+LOCAL_VAR[maxmin_s16]
	cmpnleps	xmm3, LOCAL_VAR[maxmin_s16]
	cmpltps		xmm4, 16+LOCAL_VAR[maxmin_s16]
	cvtps2pi	mm0, xmm5
	cvtps2pi	mm1, xmm0
	movhlps		xmm5, xmm5
	movhlps		xmm0, xmm0
	cvtps2pi	mm2, xmm5
	cvtps2pi	mm3, xmm0
	packssdw	mm0, mm2
	packssdw	mm1, mm3
	mov		mm2, mm0
	punpcklwd	mm0, mm1
	punpckhwd	mm2, mm1
	mov		qword ptr [SAMPLES], mm0
	mov		qword ptr 8[SAMPLES], mm2
	
	cvtps2pi	mm2, xmm1
	cvtps2pi	mm3, xmm3
	movhlps		xmm1, xmm1
	movhlps		xmm3, xmm3
	cvtps2pi	mm4, xmm1
	cvtps2pi	mm5, xmm3
	packssdw	mm2, mm4
	packssdw	mm3, mm5
	psrlw		mm2, 15
	psrlw		mm3, 15
	cvtps2pi	mm0, xmm2
	cvtps2pi	mm1, xmm4
	movhlps		xmm2, xmm2
	movhlps		xmm4, xmm4
	cvtps2pi	mm4, xmm2
	cvtps2pi	mm5, xmm4
	packssdw	mm0, mm4
	packssdw	mm1, mm5
	psrlw		mm0, 15
	psrlw		mm1, 15
	paddw		mm2, mm3
	paddw		mm0, mm1
	paddw		mm0, mm2
	paddw		MMREG_CLIP, mm0
	
	lea		SAMPLES, dword ptr 16[SAMPLES]
	dec		ecx
	jnz			1b
	
	mov		ecx, 4
	
	ALIGN16
1:
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, 0[B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm4, 0[B0R]
	mulps		xmm5, 16[B0R]
	mulps		xmm6, 32[B0R]
	mulps		xmm7, 48[B0R]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP[0], xmm0
	movaps		TEMP[4], xmm4
	
	lea		WINDOW, dword ptr 128[WINDOW]
	lea		B0L, dword ptr -64[B0L]
	lea		B0R, dword ptr -64[B0R]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, 0[B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm4, 0[B0R]
	mulps		xmm5, 16[B0R]
	mulps		xmm6, 32[B0R]
	mulps		xmm7, 48[B0R]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP[1], xmm0
	movaps		TEMP[5], xmm4
	
	lea		WINDOW, dword ptr 128[WINDOW]
	lea		B0L, dword ptr -64[B0L]
	lea		B0R, dword ptr -64[B0R]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, 0[B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm4, 0[B0R]
	mulps		xmm5, 16[B0R]
	mulps		xmm6, 32[B0R]
	mulps		xmm7, 48[B0R]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		TEMP[2], xmm0
	movaps		TEMP[6], xmm4
	
	lea		WINDOW, dword ptr 128[WINDOW]
	lea		B0L, dword ptr -64[B0L]
	lea		B0R, dword ptr -64[B0R]
	
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 32[WINDOW]
	movups		xmm3, 48[WINDOW]
	movaps		xmm4, xmm0
	movaps		xmm5, xmm1
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	mulps		xmm0, 0[B0L]
	mulps		xmm1, 16[B0L]
	mulps		xmm2, 32[B0L]
	mulps		xmm3, 48[B0L]
	mulps		xmm4, 0[B0R]
	mulps		xmm5, 16[B0R]
	mulps		xmm6, 32[B0R]
	mulps		xmm7, 48[B0R]
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	addps		xmm4, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm2
	addps		xmm4, xmm6
	movaps		xmm7, xmm0
	movaps		TEMP[7], xmm4
	
	lea		WINDOW, dword ptr 128[WINDOW]
	lea		B0L, dword ptr -64[B0L]
	lea		B0R, dword ptr -64[B0R]
	
	movaps		xmm4, TEMP[0]
	movaps		xmm5, TEMP[1]
	movaps		xmm6, TEMP[2]
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
	
	movaps		xmm4, TEMP[4]
	movaps		xmm5, TEMP[5]
	movaps		xmm6, TEMP[6]
	movaps		xmm7, TEMP[7]
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
	
	movaps		xmm1, xmm5
	movaps		xmm2, xmm5
	movaps		xmm3, xmm0
	movaps		xmm4, xmm0
	cmpnleps	xmm1, LOCAL_VAR[maxmin_s16]
	cmpltps		xmm2, 16+LOCAL_VAR[maxmin_s16]
	cmpnleps	xmm3, LOCAL_VAR[maxmin_s16]
	cmpltps		xmm4, 16+LOCAL_VAR[maxmin_s16]
	cvtps2pi	mm0, xmm5
	cvtps2pi	mm1, xmm0
	movhlps		xmm5, xmm5
	movhlps		xmm0, xmm0
	cvtps2pi	mm2, xmm5
	cvtps2pi	mm3, xmm0
	packssdw	mm0, mm2
	packssdw	mm1, mm3
	mov		mm2, mm0
	punpcklwd	mm0, mm1
	punpckhwd	mm2, mm1
	mov		qword ptr [SAMPLES], mm0
	mov		qword ptr 8[SAMPLES], mm2
	
	cvtps2pi	mm2, xmm1
	cvtps2pi	mm3, xmm3
	movhlps		xmm1, xmm1
	movhlps		xmm3, xmm3
	cvtps2pi	mm4, xmm1
	cvtps2pi	mm5, xmm3
	packssdw	mm2, mm4
	packssdw	mm3, mm5
	psrlw		mm2, 15
	psrlw		mm3, 15
	cvtps2pi	mm0, xmm2
	cvtps2pi	mm1, xmm4
	movhlps		xmm2, xmm2
	movhlps		xmm4, xmm4
	cvtps2pi	mm4, xmm2
	cvtps2pi	mm5, xmm4
	packssdw	mm0, mm4
	packssdw	mm1, mm5
	psrlw		mm0, 15
	psrlw		mm1, 15
	paddw		mm2, mm3
	paddw		mm0, mm1
	paddw		mm0, mm2
	paddw		MMREG_CLIP, mm0
	
	lea		SAMPLES, dword ptr 16[SAMPLES]
	dec		ecx
	jnz			1b
	
	pshufw		mm0, MMREG_CLIP, 0xee
	paddw		mm0, MMREG_CLIP
	pshufw		mm1, mm0, 0x55
	paddw		mm0, mm1
	movd		eax, mm0
	and		eax, 0xffff
	
	pop		edi
	pop		esi
	pop		ebx
	mov		esp, ebp
	pop		ebp
	
	emms
	
	ret

NONEXEC_STACK
