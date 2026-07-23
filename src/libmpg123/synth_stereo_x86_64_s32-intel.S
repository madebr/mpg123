#include "gas_nasm_compat.h"
/*
	synth_stereo_x86_64_s32: SSE optimized synth for x86-64 (stereo specific, s32 output version)

	copyright 1995-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "x86_64_defs.h"
#include "mangle.h"

#ifdef IS_MSABI
/* short *window; */
#define WINDOW rsi
/* short *b0l; */
#define B0L rdx
/* short *b0r; */
#define B0R r8
/* short *samples; */
#define SAMPLES rdi
#else
/* real *window; */
#define WINDOW rdi
/* real *b0l; */
#define B0L rsi
/* real *b0r; */
#define B0R rdx
/* real *samples; */
#define SAMPLES r8
#endif

#define XMMREG_SCALE [r9]  /* {65536.0, 65536.0, 65536.0, 65536.0} */
#define XMMREG_MAX [r10]  /* {32767.999, 32767.999, 32767.999, 32767.999} */
#define XMMREG_MIN [r11]  /* {-32768.0, -32768.0, -32768.0, -32768.0} */
#define TEMP_CLIP [rsp]

/*
	int synth_1to1_s32_s_x86_64_asm(real *window, real *b0l, real *b0r, int32_t *samples, int bo1);
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
	SECTION_TEXT
	ALIGN16
GLOBAL ASM_NAME(INT123_synth_1to1_s32_s_x86_64_asm)
ASM_NAME(INT123_synth_1to1_s32_s_x86_64_asm):
#ifdef IS_MSABI /* should save xmm6-15 */
	mov		eax, dword ptr 40[rsp] /* 5th argument; placed after 32-byte shadow space */
	push		rsi
	push		rdi
	sub		rsp, 184 /* stack alignment + 10 xmm registers + temp */
	movaps		16[rsp], xmm6
	movaps		32[rsp], xmm7
	movaps		48[rsp], xmm8
	movaps		64[rsp], xmm9
	movaps		80[rsp], xmm10
	movaps		96[rsp], xmm11
	movaps		112[rsp], xmm12
	movaps		128[rsp], xmm13
	movaps		144[rsp], xmm14
	movaps		160[rsp], xmm15
#else
	sub		rsp, 24  /* stack alignment + temp */
#endif
	
#ifdef IS_MSABI
	shl		rax, 32
	shr		rax, 30
	mov		rsi, rcx
	mov		rdi, r9
#else
	mov		rax, r8
	shl		rax, 32
	shr		rax, 30
	mov		r8, rcx
#endif
	lea		WINDOW, 64[WINDOW]
	sub		WINDOW, rax
	
	lea		r9, ASM_NAME(INT123_scale_s32) RIP_REL
	lea		r10, ASM_NAME(INT123_maxmin_s32) RIP_REL
	lea		r11, 16[r10]
	xorps		xmm0, xmm0
	movaps		TEMP_CLIP, xmm0

	mov		ecx, 4
	
	ALIGN16
loop1:
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
	lea		WINDOW, 256[WINDOW]
	lea		B0L, 128[B0L]
	lea		B0R, 128[B0R]
	
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
	lea		WINDOW, 256[WINDOW]
	lea		B0L, 128[B0L]
	lea		B0R, 128[B0R]
	
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
	
	movaps		xmm2, xmm0
	movaps		xmm3, xmm0
	movaps		xmm5, xmm4
	movaps		xmm6, xmm4
	mulps		xmm0, XMMREG_SCALE
	mulps		xmm4, XMMREG_SCALE
	cmpnleps	xmm2, XMMREG_MAX
	cmpltps		xmm3, XMMREG_MIN
	cmpnleps	xmm5, XMMREG_MAX
	cmpltps		xmm6, XMMREG_MIN
	cvtps2dq	xmm0, xmm0
	cvtps2dq	xmm4, xmm4
	xorps		xmm0, xmm2
	xorps		xmm4, xmm5
	movaps		xmm1, xmm0
	unpcklps	xmm0, xmm4
	unpckhps	xmm1, xmm4
	movups		[SAMPLES], xmm0
	movups		16[SAMPLES], xmm1
	
	packssdw	xmm2, xmm5
	packssdw	xmm3, xmm6
	psrlw		xmm2, 15
	psrlw		xmm3, 15
	paddw		xmm2, xmm3
	paddw		xmm2, TEMP_CLIP
	movaps		TEMP_CLIP, xmm2
	
	lea		SAMPLES, 32[SAMPLES]
	dec		ecx
	jnz			loop1
	
	mov		ecx, 4
	
	ALIGN16
loop2:
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
	lea		WINDOW, 256[WINDOW]
	lea		B0L, -128[B0L]
	lea		B0R, -128[B0R]
	
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
	lea		WINDOW, 256[WINDOW]
	lea		B0L, -128[B0L]
	lea		B0R, -128[B0R]
	
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
	
	movaps		xmm2, xmm0
	movaps		xmm3, xmm0
	movaps		xmm5, xmm4
	movaps		xmm6, xmm4
	mulps		xmm0, XMMREG_SCALE
	mulps		xmm4, XMMREG_SCALE
	cmpnleps	xmm2, XMMREG_MAX
	cmpltps		xmm3, XMMREG_MIN
	cmpnleps	xmm5, XMMREG_MAX
	cmpltps		xmm6, XMMREG_MIN
	cvtps2dq	xmm0, xmm0
	cvtps2dq	xmm4, xmm4
	xorps		xmm0, xmm2
	xorps		xmm4, xmm5
	movaps		xmm1, xmm0
	unpcklps	xmm0, xmm4
	unpckhps	xmm1, xmm4
	movups		[SAMPLES], xmm0
	movups		16[SAMPLES], xmm1
	
	packssdw	xmm2, xmm5
	packssdw	xmm3, xmm6
	psrlw		xmm2, 15
	psrlw		xmm3, 15
	paddw		xmm2, xmm3
	paddw		xmm2, TEMP_CLIP
	movaps		TEMP_CLIP, xmm2
	
	lea		SAMPLES, 32[SAMPLES]
	dec		ecx
	jnz			loop2
	
	movaps		xmm4, TEMP_CLIP
	movhlps		xmm0, xmm4
	paddw		xmm0, xmm4
	pshuflw		xmm1, xmm0, HEX(55)
	pshuflw		xmm2, xmm0, HEX(aa)
	pshuflw		xmm3, xmm0, HEX(ff)
	paddw		xmm0, xmm1
	paddw		xmm0, xmm2
	paddw		xmm0, xmm3
	
	movd		eax, xmm0
	and		eax, HEX(ffff)
	
#ifdef IS_MSABI
	movaps		xmm6, 16[rsp]
	movaps		xmm7, 32[rsp]
	movaps		xmm8, 48[rsp]
	movaps		xmm9, 64[rsp]
	movaps		xmm10, 80[rsp]
	movaps		xmm11, 96[rsp]
	movaps		xmm12, 112[rsp]
	movaps		xmm13, 128[rsp]
	movaps		xmm14, 144[rsp]
	movaps		xmm15, 160[rsp]
	add		rsp, 184
	pop		rdi
	pop		rsi
#else
	add		rsp, 24
#endif
	ret

NONEXEC_STACK

END_MODULE

