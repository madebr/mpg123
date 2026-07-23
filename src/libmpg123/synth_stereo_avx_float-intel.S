#include "gas_nasm_compat.h"
/*
	synth_stereo_avx_float: AVX optimized synth for x86-64 (stereo specific, float output version)

	copyright 1995-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "x86_64_defs.h"
#include "mangle.h"

#ifdef IS_MSABI
/* real *window; */
#define WINDOW r10
/* real *b0l; */
#define B0L rdx
/* real *b0r; */
#define B0R r8
/* real *samples; */
#define SAMPLES r9
#else
/* real *window; */
#define WINDOW rdi
/* real *b0l; */
#define B0L rsi
/* real *b0r; */
#define B0R rdx
/* real *samples; */
#define SAMPLES r9
#endif

/*
	int synth_1to1_real_s_avx_asm(real *window, real *b0l, real *b0r, real *samples, int bo1);
	return value: number of clipped samples (0)
*/

	RODATA
	ALIGN16
scale_avx:
	DATA_LONG   939524096
	SECTION_TEXT
	ALIGN16
	GLOBAL ASM_NAME(INT123_synth_1to1_real_s_avx_asm)
ASM_NAME(INT123_synth_1to1_real_s_avx_asm):
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

	vbroadcastss	ymm14, DWORD_PTR RIP_REL_F(scale_avx)
	
#ifdef IS_MSABI
	shl			eax, 2
	mov			WINDOW, rcx
#else
	mov			eax, r8d
	shl			eax, 2
	mov			SAMPLES, rcx
#endif
	add			WINDOW, 64
	sub			WINDOW, rax

	mov			rax, 128
	mov			ecx, 4
	
	ALIGN16
loop1:
	vmovups		ymm8, [WINDOW]
	vmovups		ymm9, 32[WINDOW]
	vmovups		ymm10, [WINDOW+rax]
	vmovups		ymm11, 32[WINDOW+rax]
	vmulps		ymm0, ymm8, [B0L]
	vmulps		ymm1, ymm9, 32[B0L]
	vmulps		ymm2, ymm8, [B0R]
	vmulps		ymm3, ymm9, 32[B0R]
	vmulps		ymm4, ymm10, 64[B0L]
	vmulps		ymm5, ymm11, 96[B0L]
	vmulps		ymm6, ymm10, 64[B0R]
	vmulps		ymm7, ymm11, 96[B0R]
	vaddps		ymm8, ymm0, ymm1
	vaddps		ymm0, ymm2, ymm3
	vaddps		ymm9, ymm4, ymm5
	vaddps		ymm1, ymm6, ymm7
	lea			WINDOW, [WINDOW+rax*2]
	add			B0L, rax
	add			B0R, rax
	
	vmovups		ymm10, [WINDOW]
	vmovups		ymm11, 32[WINDOW]
	vmovups		ymm12, [WINDOW+rax]
	vmovups		ymm13, 32[WINDOW+rax]
	vmulps		ymm2, ymm10, [B0L]
	vmulps		ymm3, ymm11, 32[B0L]
	vmulps		ymm4, ymm10, [B0R]
	vmulps		ymm5, ymm11, 32[B0R]
	vmulps		ymm6, ymm12, 64[B0L]
	vmulps		ymm10, ymm13, 96[B0L]
	vmulps		ymm7, ymm12, 64[B0R]
	vmulps		ymm11, ymm13, 96[B0R]
	vaddps		ymm2, ymm2, ymm3
	vaddps		ymm3, ymm4, ymm5
	vaddps		ymm4, ymm10, ymm6
	vaddps		ymm5, ymm11, ymm7
	lea			WINDOW, [WINDOW+rax*2]
	add			B0L, rax
	add			B0R, rax
	
	vunpcklps	ymm6, ymm8, ymm0
	vunpckhps	ymm0, ymm8, ymm0
	vunpcklps	ymm7, ymm9, ymm1
	vunpckhps	ymm1, ymm9, ymm1
	vaddps		ymm0, ymm0, ymm6
	vaddps		ymm1, ymm1, ymm7
	vunpcklps	ymm6, ymm2, ymm3
	vunpckhps	ymm2, ymm2, ymm3
	vunpcklps	ymm7, ymm4, ymm5
	vunpckhps	ymm3, ymm4, ymm5
	vaddps		ymm2, ymm2, ymm6
	vaddps		ymm3, ymm3, ymm7
	
	vunpcklpd	ymm4, ymm0, ymm1
	vunpckhpd	ymm0, ymm0, ymm1
	vunpcklpd	ymm5, ymm2, ymm3
	vunpckhpd	ymm1, ymm2, ymm3
	vsubps		ymm0, ymm4, ymm0
	vsubps		ymm1, ymm5, ymm1
	vperm2f128	ymm2, ymm0, ymm1, HEX(20)
	vperm2f128	ymm3, ymm0, ymm1, HEX(31)
	vaddps		ymm0, ymm2, ymm3
	vmulps		ymm0, ymm0, ymm14
	
	vmovups		[SAMPLES], ymm0
	add			SAMPLES, 32
	dec			ecx
	jnz			loop1
	
	mov			ecx, 4
	
	ALIGN16
loop2:
	vmovups		ymm8, [WINDOW]
	vmovups		ymm9, 32[WINDOW]
	vmovups		ymm10, [WINDOW+rax]
	vmovups		ymm11, 32[WINDOW+rax]
	vmulps		ymm0, ymm8, [B0L]
	vmulps		ymm1, ymm9, 32[B0L]
	vmulps		ymm2, ymm8, [B0R]
	vmulps		ymm3, ymm9, 32[B0R]
	vmulps		ymm4, ymm10, [B0L-64]
	vmulps		ymm5, ymm11, [B0L-32]
	vmulps		ymm6, ymm10, -64[B0R]
	vmulps		ymm7, ymm11, -32[B0R]
	vaddps		ymm8, ymm0, ymm1
	vaddps		ymm0, ymm2, ymm3
	vaddps		ymm9, ymm4, ymm5
	vaddps		ymm1, ymm6, ymm7
	lea			WINDOW, [WINDOW+rax*2]
	sub			B0L, rax
	sub			B0R, rax
	
	vmovups		ymm10, [WINDOW]
	vmovups		ymm11, 32[WINDOW]
	vmovups		ymm12, [WINDOW+rax]
	vmovups		ymm13, 32[WINDOW+rax]
	vmulps		ymm2, ymm10, [B0L]
	vmulps		ymm3, ymm11, 32[B0L]
	vmulps		ymm4, ymm10, [B0R]
	vmulps		ymm5, ymm11, 32[B0R]
	vmulps		ymm6, ymm12, -64[B0L]
	vmulps		ymm10, ymm13, -32[B0L]
	vmulps		ymm7, ymm12, -64[B0R]
	vmulps		ymm11, ymm13, -32[B0R]
	vaddps		ymm2, ymm2, ymm3
	vaddps		ymm3, ymm4, ymm5
	vaddps		ymm4, ymm10, ymm6
	vaddps		ymm5, ymm11, ymm7
	lea			WINDOW, [WINDOW+rax*2]
	sub			B0L, rax
	sub			B0R, rax
	
	vunpcklps	ymm6, ymm8, ymm0
	vunpckhps	ymm0, ymm8, ymm0
	vunpcklps	ymm7, ymm9, ymm1
	vunpckhps	ymm1, ymm9, ymm1
	vaddps		ymm0, ymm0, ymm6
	vaddps		ymm1, ymm1, ymm7
	vunpcklps	ymm6, ymm2, ymm3
	vunpckhps	ymm2, ymm2, ymm3
	vunpcklps	ymm7, ymm4, ymm5
	vunpckhps	ymm3, ymm4, ymm5
	vaddps		ymm2, ymm2, ymm6
	vaddps		ymm3, ymm3, ymm7
	
	vunpcklpd	ymm4, ymm0, ymm1
	vunpckhpd	ymm0, ymm0, ymm1
	vunpcklpd	ymm5, ymm2, ymm3
	vunpckhpd	ymm1, ymm2, ymm3
	vaddps		ymm0, ymm4, ymm0
	vaddps		ymm1, ymm5, ymm1
	vperm2f128	ymm2, ymm0, ymm1, HEX(20)
	vperm2f128	ymm3, ymm0, ymm1, HEX(31)
	vaddps		ymm0, ymm2, ymm3
	vmulps		ymm0, ymm0, ymm14
	
	vmovups		[SAMPLES], ymm0
	add			SAMPLES, 32
	dec			ecx
	jnz			loop2
	
	vzeroupper
	
	xor			eax, eax
	
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
