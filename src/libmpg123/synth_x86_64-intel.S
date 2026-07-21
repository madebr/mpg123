#include "gas_nasm_compat.h"
/*
	synth_x86_64: SSE optimized synth for x86-64

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
/* short *window; */
#define ARG0 rdi
/* short *b0; */
#define ARG1 rsi
/* short *samples; */
#define ARG2 rdx
/* int bo1; */
#define ARG3 rcx
#endif

#define XMMREG_CLIP xmm15
#define XMMREG_MAX xmm14 /* {32767, 32767, 32767, 32767} */
#define XMMREG_MIN xmm13 /* {-32769, -32769, -32769, -32769} : not -32768 because SSE doesn't have "less than" comparison... */
#define XMMREG_FULL xmm12 /* {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF} */

/*
	int synth_1to1_x86_64_asm(short *window, short *b0, short *samples, int bo1);
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
	SECTION_TEXT
	ALIGN16
GLOBAL ASM_NAME(INT123_synth_1to1_x86_64_asm)
ASM_NAME(INT123_synth_1to1_x86_64_asm):
#ifdef IS_MSABI /* should save xmm6-15 */
	mov		ARG0, rcx
	sub		rsp, 104 /* stack alignment + 6 xmm registers */
	movaps		[rsp], xmm6
	movaps		16[rsp], xmm7
	movaps		32[rsp], xmm12
	movaps		48[rsp], xmm13
	movaps		64[rsp], xmm14
	movaps		80[rsp], xmm15
#endif

	lea		rax, qword ptr ASM_NAME(INT123_maxmin_x86_64) RIP_REL
	movaps		XMMREG_MAX, [rax]
	movaps		XMMREG_MIN, 16[rax]
	pxor		XMMREG_CLIP, XMMREG_CLIP
	pcmpeqd		XMMREG_FULL, XMMREG_FULL
	
	and		ARG3, 0xf
	shl		ARG3, 1
	lea		ARG0, qword ptr 32[ARG0]
	sub		ARG0, ARG3
	
	mov		ecx, 4
	
	ALIGN16
1:
	movups		xmm0, [ARG0]
	movups		xmm1, 16[ARG0]
	movups		xmm2, 64[ARG0]
	movups		xmm3, 80[ARG0]
	movups		xmm4, 128[ARG0]
	movups		xmm5, 144[ARG0]
	movups		xmm6, 192[ARG0]
	movups		xmm7, 208[ARG0]
	pmaddwd		xmm0, [ARG1]
	pmaddwd		xmm1, 16[ARG1]
	pmaddwd		xmm2, 32[ARG1]
	pmaddwd		xmm3, 48[ARG1]
	pmaddwd		xmm4, 64[ARG1]
	pmaddwd		xmm5, 80[ARG1]
	pmaddwd		xmm6, 96[ARG1]
	pmaddwd		xmm7, 112[ARG1]
	
	paddd		xmm0, xmm1
	paddd		xmm2, xmm3
	paddd		xmm4, xmm5
	paddd		xmm6, xmm7
	
	movaps		xmm1, xmm0
	movaps		xmm3, xmm4
	punpckldq	xmm0, xmm2
	punpckldq	xmm4, xmm6
	punpckhdq	xmm1, xmm2
	punpckhdq	xmm3, xmm6
	movaps		xmm5, xmm0
	movaps		xmm7, xmm1
	movlhps		xmm0, xmm4
	movhlps		xmm4, xmm5
	movlhps		xmm1, xmm3
	movhlps		xmm3, xmm7
	paddd		xmm0, xmm4
	paddd		xmm1, xmm3
	paddd		xmm0, xmm1
	psrad		xmm0, 13
	
	movups		xmm3, [ARG2]
	movaps		xmm1, xmm0
	movaps		xmm2, xmm0
	packssdw	xmm0, xmm0
	pcmpgtd		xmm1, XMMREG_MAX
	pcmpgtd		xmm2, XMMREG_MIN
	movhlps		xmm4, xmm3
	pshuflw		xmm3, xmm3, 0xdd
	pshuflw		xmm4, xmm4, 0xdd
	psrlq		xmm3, 32
	psllq		xmm4, 32
	por			xmm3, xmm4
	punpcklwd	xmm0, xmm3
	movups		[ARG2], xmm0
	
	pxor		xmm2, XMMREG_FULL
	psrld		xmm1, 31
	psrld		xmm2, 31
	paddd		xmm1, xmm2
	paddd		XMMREG_CLIP, xmm1
	
	lea		ARG0, qword ptr 256[ARG0]
	lea		ARG1, qword ptr 128[ARG1]
	lea		ARG2, qword ptr 16[ARG2]
	
	dec		ecx
	jnz			1b
	
	mov		ecx, 4
	
	ALIGN16
1:
	movups		xmm0, [ARG0]
	movups		xmm1, 16[ARG0]
	movups		xmm2, 64[ARG0]
	movups		xmm3, 80[ARG0]
	movups		xmm4, 128[ARG0]
	movups		xmm5, 144[ARG0]
	movups		xmm6, 192[ARG0]
	movups		xmm7, 208[ARG0]
	pmaddwd		xmm0, [ARG1]
	pmaddwd		xmm1, 16[ARG1]
	pmaddwd		xmm2, -32[ARG1]
	pmaddwd		xmm3, -16[ARG1]
	pmaddwd		xmm4, -64[ARG1]
	pmaddwd		xmm5, -48[ARG1]
	pmaddwd		xmm6, -96[ARG1]
	pmaddwd		xmm7, -80[ARG1]
	
	paddd		xmm0, xmm1
	paddd		xmm2, xmm3
	paddd		xmm4, xmm5
	paddd		xmm6, xmm7
	
	movaps		xmm1, xmm0
	movaps		xmm3, xmm4
	punpckldq	xmm0, xmm2
	punpckldq	xmm4, xmm6
	punpckhdq	xmm1, xmm2
	punpckhdq	xmm3, xmm6
	movaps		xmm5, xmm0
	movaps		xmm7, xmm1
	movlhps		xmm0, xmm4
	movhlps		xmm4, xmm5
	movlhps		xmm1, xmm3
	movhlps		xmm3, xmm7
	paddd		xmm0, xmm4
	paddd		xmm1, xmm3
	paddd		xmm0, xmm1
	psrad		xmm0, 13
	
	movups		xmm3, [ARG2]
	movaps		xmm1, xmm0
	movaps		xmm2, xmm0
	packssdw	xmm0, xmm0
	pcmpgtd		xmm1, XMMREG_MAX
	pcmpgtd		xmm2, XMMREG_MIN
	movhlps		xmm4, xmm3
	pshuflw		xmm3, xmm3, 0xdd
	pshuflw		xmm4, xmm4, 0xdd
	psrlq		xmm3, 32
	psllq		xmm4, 32
	por			xmm3, xmm4
	punpcklwd	xmm0, xmm3
	movups		[ARG2], xmm0
	
	pxor		xmm2, XMMREG_FULL
	psrld		xmm1, 31
	psrld		xmm2, 31
	paddd		xmm1, xmm2
	paddd		XMMREG_CLIP, xmm1
	
	lea		ARG0, qword ptr 256[ARG0]
	lea		ARG1, qword ptr -128[ARG1]
	lea		ARG2, qword ptr 16[ARG2]
	
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
	movaps		xmm12, 32[rsp]
	movaps		xmm13, 48[rsp]
	movaps		xmm14, 64[rsp]
	movaps		xmm15, 80[rsp]
	add		rsp, 104
#endif
	ret

NONEXEC_STACK
