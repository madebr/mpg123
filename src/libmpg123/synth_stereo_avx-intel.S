#include "gas_nasm_compat.h"
/*
	synth_stereo_avx: AVX optimized synth for x86-64 (stereo specific version)

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

/*
	int synth_1to1_s_avx_asm(short *window, short *b0l, short *b0r, short *samples, int bo1);
	return value: number of clipped samples
*/

	ALIGN16
GLOBAL ASM_NAME(INT123_synth_1to1_s_avx_asm)
ASM_NAME(INT123_synth_1to1_s_avx_asm):
#ifdef IS_MSABI /* should save xmm6-15 */
	push		rbp
	mov			rbp, rsp
	sub			rsp, 144
	movaps		[rsp], xmm6
	movaps		16[rsp], xmm7
	movaps		32[rsp], xmm8
	movaps		48[rsp], xmm9
	movaps		64[rsp], xmm10
	movaps		80[rsp], xmm11
	movaps		96[rsp], xmm12
	movaps		112[rsp], xmm13
	movaps		128[rsp], xmm14
	mov		eax, DWORD_PTR 48[rbp] /* 5th argument; placed after 32-byte shadow space */
#endif
	
#ifdef IS_MSABI
	shl			eax, 1
	mov			WINDOW, rcx
#else
	mov			eax, r8d
	shl			eax, 1
	mov		SAMPLES, rcx
#endif
	add			WINDOW, 32
	sub			WINDOW, rax
	
	mov			rax, 64
	mov		ecx, 4
	vpxor		xmm14, xmm14, xmm14
	
	ALIGN16
loop1:
	movups		xmm8, [WINDOW]
	movups		xmm9, 16[WINDOW]
	movups		xmm10, [WINDOW+rax]
	movups		xmm11, 16[WINDOW+rax]
	vpmaddwd	xmm0, xmm8, [B0L]
	vpmaddwd	xmm1, xmm9, 16[B0L]
	vpmaddwd	xmm2, xmm8, [B0R]
	vpmaddwd	xmm3, xmm9, 16[B0R]
	vpmaddwd	xmm4, xmm10, 32[B0L]
	vpmaddwd	xmm5, xmm11, 48[B0L]
	vpmaddwd	xmm6, xmm10, 32[B0R]
	vpmaddwd	xmm7, xmm11, 48[B0R]
	vpaddd		xmm8, xmm0, xmm1
	vpaddd		xmm0, xmm2, xmm3
	vpaddd		xmm9, xmm4, xmm5
	vpaddd		xmm1, xmm6, xmm7
	lea			WINDOW, [WINDOW+rax*2]
	add			B0L, rax
	add			B0R, rax
	
	movups		xmm10, [WINDOW]
	movups		xmm11, 16[WINDOW]
	movups		xmm12, [WINDOW+rax]
	movups		xmm13, 16[WINDOW+rax]
	vpmaddwd	xmm2, xmm10, [B0L]
	vpmaddwd	xmm3, xmm11, 16[B0L]
	vpmaddwd	xmm4, xmm10, [B0R]
	vpmaddwd	xmm5, xmm11, 16[B0R]
	vpmaddwd	xmm6, xmm12, 32[B0L]
	vpmaddwd	xmm10, xmm13, 48[B0L]
	vpmaddwd	xmm7, xmm12, 32[B0R]
	vpmaddwd	xmm11, xmm13, 48[B0R]
	vpaddd		xmm2, xmm2, xmm3
	vpaddd		xmm3, xmm4, xmm5
	vpaddd		xmm4, xmm10, xmm6
	vpaddd		xmm5, xmm11, xmm7
	lea			WINDOW, [WINDOW+rax*2]
	add			B0L, rax
	add			B0R, rax
	
	vpunpckldq	xmm6, xmm8, xmm0
	vpunpckhdq	xmm0, xmm8, xmm0
	vpunpckldq	xmm7, xmm9, xmm1
	vpunpckhdq	xmm1, xmm9, xmm1
	vpaddd		xmm0, xmm0, xmm6
	vpaddd		xmm1, xmm1, xmm7
	vpunpckldq	xmm6, xmm2, xmm3
	vpunpckhdq	xmm2, xmm2, xmm3
	vpunpckldq	xmm7, xmm4, xmm5
	vpunpckhdq	xmm3, xmm4, xmm5
	vpaddd		xmm2, xmm2, xmm6
	vpaddd		xmm3, xmm3, xmm7
	
	vpunpcklqdq	xmm4, xmm0, xmm1
	vpunpckhqdq	xmm0, xmm0, xmm1
	vpunpcklqdq	xmm5, xmm2, xmm3
	vpunpckhqdq	xmm1, xmm2, xmm3
	vpaddd		xmm0, xmm4, xmm0
	vpaddd		xmm1, xmm5, xmm1
	vpsrad		xmm0, xmm0, 13
	vpsrad		xmm1, xmm1, 13
	vpackssdw	xmm2, xmm0, xmm1
	vpcmpeqd	xmm3, xmm3, xmm3
	vpslld		xmm0, xmm0, 16
	vpslld		xmm1, xmm1, 16
	vpsrld		xmm0, xmm0, 16
	vpsrld		xmm1, xmm1, 16
	vpackusdw	xmm0, xmm0, xmm1
	vpcmpeqw	xmm0, xmm0, xmm2
	vpxor		xmm0, xmm0, xmm3
	vpaddw		xmm14, xmm14, xmm0
	
	movups		[SAMPLES], xmm2
	add			SAMPLES, 16
	dec			ecx
	jnz			loop1
	
	mov		ecx, 4
	
	ALIGN16
loop2:
	movups		xmm8, [WINDOW]
	movups		xmm9, 16[WINDOW]
	movups		xmm10, [WINDOW+rax]
	movups		xmm11, 16[WINDOW+rax]
	vpmaddwd	xmm0, xmm8, [B0L]
	vpmaddwd	xmm1, xmm9, 16[B0L]
	vpmaddwd	xmm2, xmm8, [B0R]
	vpmaddwd	xmm3, xmm9, 16[B0R]
	vpmaddwd	xmm4, xmm10, [B0L-32]
	vpmaddwd	xmm5, xmm11, [B0L-16]
	vpmaddwd	xmm6, xmm10, [B0R-32]
	vpmaddwd	xmm7, xmm11, [B0R-16]
	vpaddd		xmm8, xmm0, xmm1
	vpaddd		xmm0, xmm2, xmm3
	vpaddd		xmm9, xmm4, xmm5
	vpaddd		xmm1, xmm6, xmm7
	lea			WINDOW, [WINDOW+rax*2]
	sub			B0L, rax
	sub			B0R, rax
	
	movups		xmm10, [WINDOW]
	movups		xmm11, 16[WINDOW]
	movups		xmm12, [WINDOW+rax]
	movups		xmm13, 16[WINDOW+rax]
	vpmaddwd	xmm2, xmm10, [B0L]
	vpmaddwd	xmm3, xmm11, 16[B0L]
	vpmaddwd	xmm4, xmm10, [B0R]
	vpmaddwd	xmm5, xmm11, 16[B0R]
	vpmaddwd	xmm6, xmm12, [B0L-32]
	vpmaddwd	xmm10, xmm13, [B0L-16]
	vpmaddwd	xmm7, xmm12, [B0R-32]
	vpmaddwd	xmm11, xmm13, [B0R-16]
	vpaddd		xmm2, xmm2, xmm3
	vpaddd		xmm3, xmm4, xmm5
	vpaddd		xmm4, xmm10, xmm6
	vpaddd		xmm5, xmm11, xmm7
	lea			WINDOW, [WINDOW+rax*2]
	sub			B0L, rax
	sub			B0R, rax
	
	vpunpckldq	xmm6, xmm8, xmm0
	vpunpckhdq	xmm0, xmm8, xmm0
	vpunpckldq	xmm7, xmm9, xmm1
	vpunpckhdq	xmm1, xmm9, xmm1
	vpaddd		xmm0, xmm0, xmm6
	vpaddd		xmm1, xmm1, xmm7
	vpunpckldq	xmm6, xmm2, xmm3
	vpunpckhdq	xmm2, xmm2, xmm3
	vpunpckldq	xmm7, xmm4, xmm5
	vpunpckhdq	xmm3, xmm4, xmm5
	vpaddd		xmm2, xmm2, xmm6
	vpaddd		xmm3, xmm3, xmm7
	
	vpunpcklqdq	xmm4, xmm0, xmm1
	vpunpckhqdq	xmm0, xmm0, xmm1
	vpunpcklqdq	xmm5, xmm2, xmm3
	vpunpckhqdq	xmm1, xmm2, xmm3
	vpaddd		xmm0, xmm4, xmm0
	vpaddd		xmm1, xmm5, xmm1
	vpsrad		xmm0, xmm0, 13
	vpsrad		xmm1, xmm1, 13
	vpackssdw	xmm2, xmm0, xmm1
	vpcmpeqd	xmm3, xmm3, xmm3
	vpslld		xmm0, xmm0, 16
	vpslld		xmm1, xmm1, 16
	vpsrld		xmm0, xmm0, 16
	vpsrld		xmm1, xmm1, 16
	vpackusdw	xmm0, xmm0, xmm1
	vpcmpeqw	xmm0, xmm0, xmm2
	vpxor		xmm0, xmm0, xmm3
	vpaddw		xmm14, xmm14, xmm0
	
	movups		[SAMPLES], xmm2
	add			SAMPLES, 16
	dec			ecx
	jnz			loop2
	
	pxor		xmm1, xmm1
	psubw		xmm1, xmm14
	pshufd		xmm0, xmm1, HEX(4e)
	paddw		xmm0, xmm1
	pshuflw		xmm1, xmm0, HEX(4e)
	paddw		xmm0, xmm1
	pshuflw		xmm1, xmm0, HEX(11)
	paddw		xmm0, xmm1
	movd		eax, xmm0
	and			eax, HEX(7f)
	
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
	mov			rsp, rbp
	pop			rbp
#endif
	ret

NONEXEC_STACK

END_MODULE

