#include "gas_nasm_compat.h"
/*
	synth_stereo_x86_64: SSE optimized synth for x86-64 (stereo specific version)

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
/* short *window; */
#define WINDOW rdi
/* short *b0l; */
#define B0L rsi
/* short *b0r; */
#define B0R rdx
/* short *samples; */
#define SAMPLES r9
#endif

#define XMMREG_CLIP %xmm15
#define XMMREG_MAX %xmm14 /* {32767, 32767, 32767, 32767} */
#define XMMREG_MIN %xmm13 /* {-32769, -32769, -32769, -32769} : not -32768 because SSE doesn't have "less than" comparison... */
#define XMMREG_FULL %xmm12 /* {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF} */

/*
	int synth_1to1_s_x86_64_asm(short *window, short *b0l, short *b0r, short *samples, int bo1);
	return value: number of clipped samples
*/

	RODATA
	ALIGN32
ASM_NAME(INT123_maxmin_x86_64):
	DATA_LONG   32767
	DATA_LONG   32767
	DATA_LONG   32767
	DATA_LONG   32767
	DATA_LONG   -32769
	DATA_LONG   -32769
	DATA_LONG   -32769
	DATA_LONG   -32769
	.text
	ALIGN16
.globl ASM_NAME(INT123_synth_1to1_s_x86_64_asm)
ASM_NAME(INT123_synth_1to1_s_x86_64_asm):
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
	
#ifdef IS_MSABI
	shl		rax, 32
	shr		rax, 31
	mov		r10, rcx
#else
	mov		rax, r8
	shl		rax, 32
	shr		rax, 31
	mov		r9, rcx
#endif
	lea		WINDOW, qword ptr 32[WINDOW]
	sub		WINDOW, rax
	
	lea		rax, qword ptr ASM_NAME(INT123_maxmin_x86_64)[rip]
	movaps		XMMREG_MAX, [rax]
	movaps		XMMREG_MIN, 16[rax]
	pxor		XMMREG_CLIP, XMMREG_CLIP
	pcmpeqd		XMMREG_FULL, XMMREG_FULL
	
	mov		ecx, 4
	
	ALIGN16
1:
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 64[WINDOW]
	movups		xmm3, 80[WINDOW]
	movups		xmm4, 128[WINDOW]
	movups		xmm5, 144[WINDOW]
	movups		xmm6, 192[WINDOW]
	movups		xmm7, 208[WINDOW]
	movaps		xmm8, xmm0
	movaps		xmm9, xmm1
	movaps		xmm10, xmm2
	movaps		xmm11, xmm3
	pmaddwd		xmm0, [B0L]
	pmaddwd		xmm1, 16[B0L]
	pmaddwd		xmm2, 32[B0L]
	pmaddwd		xmm3, 48[B0L]
	pmaddwd		xmm8, [B0R]
	pmaddwd		xmm9, 16[B0R]
	pmaddwd		xmm10, 32[B0R]
	pmaddwd		xmm11, 48[B0R]
	paddd		xmm0, xmm1
	paddd		xmm2, xmm3
	paddd		xmm8, xmm9
	paddd		xmm10, xmm11
	movaps		xmm1, xmm4
	movaps		xmm9, xmm5
	movaps		xmm3, xmm6
	movaps		xmm11, xmm7
	pmaddwd		xmm4, 64[B0L]
	pmaddwd		xmm5, 80[B0L]
	pmaddwd		xmm6, 96[B0L]
	pmaddwd		xmm7, 112[B0L]
	pmaddwd		xmm1, 64[B0R]
	pmaddwd		xmm9, 80[B0R]
	pmaddwd		xmm3, 96[B0R]
	pmaddwd		xmm11, 112[B0R]
	paddd		xmm4, xmm5
	paddd		xmm6, xmm7
	paddd		xmm9, xmm1
	paddd		xmm11, xmm3
	
	movaps		xmm1, xmm0
	movaps		xmm3, xmm4
	movaps		xmm5, xmm8
	movaps		xmm7, xmm9
	punpckldq	xmm0, xmm2
	punpckldq	xmm4, xmm6
	punpckhdq	xmm1, xmm2
	punpckhdq	xmm3, xmm6
	punpckldq	xmm8, xmm10
	punpckldq	xmm9, xmm11
	punpckhdq	xmm5, xmm10
	punpckhdq	xmm7, xmm11
	movaps		xmm2, xmm0
	movaps		xmm6, xmm1
	movaps		xmm10, xmm8
	movaps		xmm11, xmm5
	movlhps		xmm0, xmm4
	movhlps		xmm4, xmm2
	movlhps		xmm1, xmm3
	movhlps		xmm3, xmm6
	movlhps		xmm8, xmm9
	movhlps		xmm9, xmm10
	movlhps		xmm5, xmm7
	movhlps		xmm7, xmm11
	paddd		xmm0, xmm4
	paddd		xmm1, xmm3
	paddd		xmm8, xmm9
	paddd		xmm5, xmm7
	paddd		xmm0, xmm1
	paddd		xmm8, xmm5
	psrad		xmm0, 13
	psrad		xmm8, 13
	
	movaps		xmm1, xmm0
	movaps		xmm2, xmm0
	movaps		xmm3, xmm0
	movaps		xmm4, xmm8
	punpckldq	xmm0, xmm8
	punpckhdq	xmm1, xmm8
	packssdw	xmm0, xmm1
	movups		[SAMPLES], xmm0
	
	pcmpgtd		xmm2, XMMREG_MAX
	pcmpgtd		xmm3, XMMREG_MIN
	pcmpgtd		xmm4, XMMREG_MAX
	pcmpgtd		xmm8, XMMREG_MIN
	packssdw	xmm2, xmm4
	packssdw	xmm3, xmm8
	pxor		xmm3, XMMREG_FULL
	psrlw		xmm2, 15
	psrlw		xmm3, 15
	paddw		xmm2, xmm3
	paddw		XMMREG_CLIP, xmm2
	
	lea		WINDOW, qword ptr 256[WINDOW]
	lea		B0L, qword ptr 128[B0L]
	lea		B0R, qword ptr 128[B0R]
	lea		SAMPLES, qword ptr 16[SAMPLES]
	
	dec		ecx
	jnz			1b
	
	mov		ecx, 4
	
	ALIGN16
1:
	movups		xmm0, [WINDOW]
	movups		xmm1, 16[WINDOW]
	movups		xmm2, 64[WINDOW]
	movups		xmm3, 80[WINDOW]
	movups		xmm4, 128[WINDOW]
	movups		xmm5, 144[WINDOW]
	movups		xmm6, 192[WINDOW]
	movups		xmm7, 208[WINDOW]
	movaps		xmm8, xmm0
	movaps		xmm9, xmm1
	movaps		xmm10, xmm2
	movaps		xmm11, xmm3
	pmaddwd		xmm0, [B0L]
	pmaddwd		xmm1, 16[B0L]
	pmaddwd		xmm2, -32[B0L]
	pmaddwd		xmm3, -16[B0L]
	pmaddwd		xmm8, [B0R]
	pmaddwd		xmm9, 16[B0R]
	pmaddwd		xmm10, -32[B0R]
	pmaddwd		xmm11, -16[B0R]
	paddd		xmm0, xmm1
	paddd		xmm2, xmm3
	paddd		xmm8, xmm9
	paddd		xmm10, xmm11
	movaps		xmm1, xmm4
	movaps		xmm9, xmm5
	movaps		xmm3, xmm6
	movaps		xmm11, xmm7
	pmaddwd		xmm4, -64[B0L]
	pmaddwd		xmm5, -48[B0L]
	pmaddwd		xmm6, -96[B0L]
	pmaddwd		xmm7, -80[B0L]
	pmaddwd		xmm1, -64[B0R]
	pmaddwd		xmm9, -48[B0R]
	pmaddwd		xmm3, -96[B0R]
	pmaddwd		xmm11, -80[B0R]
	paddd		xmm4, xmm5
	paddd		xmm6, xmm7
	paddd		xmm9, xmm1
	paddd		xmm11, xmm3
	
	movaps		xmm1, xmm0
	movaps		xmm3, xmm4
	movaps		xmm5, xmm8
	movaps		xmm7, xmm9
	punpckldq	xmm0, xmm2
	punpckldq	xmm4, xmm6
	punpckhdq	xmm1, xmm2
	punpckhdq	xmm3, xmm6
	punpckldq	xmm8, xmm10
	punpckldq	xmm9, xmm11
	punpckhdq	xmm5, xmm10
	punpckhdq	xmm7, xmm11
	movaps		xmm2, xmm0
	movaps		xmm6, xmm1
	movaps		xmm10, xmm8
	movaps		xmm11, xmm5
	movlhps		xmm0, xmm4
	movhlps		xmm4, xmm2
	movlhps		xmm1, xmm3
	movhlps		xmm3, xmm6
	movlhps		xmm8, xmm9
	movhlps		xmm9, xmm10
	movlhps		xmm5, xmm7
	movhlps		xmm7, xmm11
	paddd		xmm0, xmm4
	paddd		xmm1, xmm3
	paddd		xmm8, xmm9
	paddd		xmm5, xmm7
	paddd		xmm0, xmm1
	paddd		xmm8, xmm5
	psrad		xmm0, 13
	psrad		xmm8, 13
	
	movaps		xmm1, xmm0
	movaps		xmm2, xmm0
	movaps		xmm3, xmm0
	movaps		xmm4, xmm8
	punpckldq	xmm0, xmm8
	punpckhdq	xmm1, xmm8
	packssdw	xmm0, xmm1
	movups		[SAMPLES], xmm0
	
	pcmpgtd		xmm2, XMMREG_MAX
	pcmpgtd		xmm3, XMMREG_MIN
	pcmpgtd		xmm4, XMMREG_MAX
	pcmpgtd		xmm8, XMMREG_MIN
	packssdw	xmm2, xmm4
	packssdw	xmm3, xmm8
	pxor		xmm3, XMMREG_FULL
	psrlw		xmm2, 15
	psrlw		xmm3, 15
	paddw		xmm2, xmm3
	paddw		XMMREG_CLIP, xmm2
	
	lea		WINDOW, qword ptr 256[WINDOW]
	lea		B0L, qword ptr -128[B0L]
	lea		B0R, qword ptr -128[B0R]
	lea		SAMPLES, qword ptr 16[SAMPLES]
	
	dec		ecx
	jnz			1b
	
	movhlps		xmm0, XMMREG_CLIP
	paddw		xmm0, XMMREG_CLIP
	pshuflw		xmm1, xmm0, 0x55
	pshuflw		xmm2, xmm0, 0xaa
	pshuflw		xmm3, xmm0, 0xff
	paddw		xmm0, xmm1
	paddw		xmm0, xmm2
	paddw		xmm0, xmm3
	
	movd		eax, xmm0
	and		eax, 0xffff
	
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
