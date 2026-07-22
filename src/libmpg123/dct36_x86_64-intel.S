#include "gas_nasm_compat.h"
/*
	dct36_x86_64: SSE optimized dct36 for x86-64

	copyright 1995-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "x86_64_defs.h"
#include "mangle.h"

#ifdef IS_MSABI
#define in rcx
#define out1 rdx
#define out2 r8
#define w  r9
#define ts r10
#define COS9_ rax
#define tfcos36_ r11
#else
#define in rdi
#define out1 rsi
#define out2 rdx
#define w  rcx
#define ts r8
#define COS9_ rax
#define tfcos36_ r9
#endif

/*
	void dct36_x86_64(real *inbuf,real *o1,real *o2,real *wintab,real *tsbuf);
*/
	
	RODATA
	ALIGN16
dct36_x86_64_COS9:
	DATA_LONG 0x3f5db3d7
	DATA_LONG 0x3f5db3d7
	DATA_LONG 0x3f000000
	DATA_LONG 0x3f000000
	DATA_LONG 0x3f7c1c5c
	DATA_LONG 0x3f7c1c5c
	DATA_LONG 0x3f708fb2
	DATA_LONG 0x3f708fb2
	DATA_LONG 0x3f248dbb
	DATA_LONG 0x3f248dbb
	DATA_LONG 0x3e31d0d4
	DATA_LONG 0x3e31d0d4
	DATA_LONG 0x3eaf1d44
	DATA_LONG 0x3eaf1d44
	DATA_LONG 0x3f441b7d
	DATA_LONG 0x3f441b7d
	ALIGN16
dct36_x86_64_tfcos36:
	DATA_LONG 0x3f007d2b
	DATA_LONG 0x3f0483ee
	DATA_LONG 0x3f0d3b7d
	DATA_LONG 0x3f1c4257
	DATA_LONG 0x40b79454
	DATA_LONG 0x3ff746ea
	DATA_LONG 0x3f976fd9
	DATA_LONG 0x3f5f2944
	DATA_LONG 0x3f3504f3
	ALIGN16
dct36_x86_64_mask:
	DATA_LONG 0,0xffffffff,0,0xffffffff
	ALIGN16
dct36_x86_64_sign:
	DATA_LONG 0x80000000,0x80000000,0x80000000,0x80000000
	SECTION_TEXT
	ALIGN16
	GLOBAL ASM_NAME(INT123_dct36_x86_64)
ASM_NAME(INT123_dct36_x86_64):
#ifdef IS_MSABI
	push		rbp
	mov			rbp, rsp
	sub			rsp, 160
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
	mov		ts, QWORD_PTR 48[rbp]
#endif
	lea			COS9_, dct36_x86_64_COS9 RIP_REL
	lea			tfcos36_, dct36_x86_64_tfcos36 RIP_REL
	
	xorps		xmm5, xmm5
	movups		xmm1, [in]
	movups		xmm2, 16[in]
	movups		xmm3, 32[in]
	movups		xmm4, 48[in]
	movlps		xmm5, 64[in]
	xorps		xmm6, xmm6
	movaps		xmm7, xmm1
	shufps		xmm7, xmm7, 0x93
	movaps		xmm8, xmm2
	shufps		xmm8, xmm8, 0x93
	movaps		xmm9, xmm3
	shufps		xmm9, xmm9, 0x93
	movaps		xmm10, xmm4
	shufps		xmm10, xmm10, 0x93
	movaps		xmm11, xmm5
	shufps		xmm11, xmm11, 0xe1
	movss		xmm11, xmm10
	addps		xmm5, xmm11
	movss		xmm10, xmm9
	addps		xmm4, xmm10
	movss		xmm9, xmm8
	addps		xmm3, xmm9
	movss		xmm8, xmm7
	addps		xmm2, xmm8
	movss		xmm7, xmm6
	addps		xmm1, xmm7
	
	movaps		xmm0, RIP_REL_F(dct36_x86_64_mask)
	movaps		xmm6, xmm4
	shufps		xmm4, xmm5, 0x4e
	movaps		xmm7, xmm3
	shufps		xmm3, xmm6, 0x4e
	andps		xmm6, xmm0
	addps		xmm4, xmm6
	movaps		xmm6, xmm2
	shufps		xmm2, xmm7, 0x4e
	andps		xmm7, xmm0
	addps		xmm3, xmm7
	movaps		xmm7, xmm1
	shufps		xmm1, xmm6, 0x4e
	andps		xmm6, xmm0
	addps		xmm2, xmm6
	movaps		xmm6, xmm7
	andps		xmm7, xmm0
	xorps		xmm0, xmm0
	addps		xmm1, xmm7
	movlhps		xmm0, xmm6

/*
xmm0 in[-,-,0,1]
xmm1 in[2,3,4,5]
xmm2 in[6,7,8,9]
xmm3 in[10,11,12,13]
xmm4 in[14,15,16,17]
*/
	
	movaps		xmm5, xmm2
	shufps		xmm5, xmm3, 0xe4
	shufps		xmm3, xmm4, 0xe4
	shufps		xmm4, xmm2, 0xe4
	movaps		xmm2, xmm5
/*
xmm2 in[6,7,12,13]
xmm3 in[10,11,16,17]
xmm4 in[14,15,8,9]
*/

	movaps		xmm15, [COS9_]
	movaps		xmm6, 16[COS9_]
	movaps		xmm7, 32[COS9_]
	movaps		xmm8, 48[COS9_]
	mulps		xmm5, xmm15
	addps		xmm5, xmm0
	
/*
xmm5 [ta33,tb33,ta66,tb66]
xmm6 COS9_[1,1,2,2]
xmm7 COS9_[5,5,8,8]
xmm8 COS9_[7,7,4,4]
xmm15 COS9_[3,3,6,6]
*/
	movaps		xmm9, xmm6
	movaps		xmm12, xmm7
	movaps		xmm13, xmm8
	mulps		xmm9, xmm1
	mulps		xmm12, xmm3
	mulps		xmm13, xmm4
	addps		xmm9, xmm5
	addps		xmm12, xmm13
	addps		xmm12, xmm9
	
	movaps		xmm13, xmm1
	subps		xmm13, xmm3
	movaps		xmm10, xmm0
	shufps		xmm10, xmm2, 0xe0
	movaps		xmm14, xmm0
	subps		xmm14, xmm10
	subps		xmm13, xmm4
	mulps		xmm13, xmm15
	addps		xmm13, xmm14
	
	movaps		xmm9, xmm7
	movaps		xmm15, xmm8
	movaps		xmm14, xmm6
	mulps		xmm9, xmm1
	mulps		xmm15, xmm3
	mulps		xmm14, xmm4
	subps		xmm9, xmm5
	subps		xmm14, xmm15
	addps		xmm14, xmm9
	
	mulps		xmm8, xmm1
	mulps		xmm6, xmm3
	mulps		xmm7, xmm4
	subps		xmm8, xmm5
	subps		xmm6, xmm7
	addps		xmm8, xmm6
	movaps		xmm15, xmm8
	
	movss		xmm5, 32[tfcos36_]
	subps		xmm0, xmm1
	subps		xmm4, xmm2
	addps		xmm0, xmm3
	addps		xmm0, xmm4
	shufps		xmm0, xmm0, 0xaf
	mulss		xmm0, xmm5
	movaps		xmm11, xmm0

/*
xmm12 [1a-0,1b-0, 2a-0, 2b-0]
xmm13 [1a-1,1b-1, 2a-1, 2b-1]
xmm14 [1a-2,1b-2,-2a-2,-2b-2]
xmm15 [1a-3,1b-3,-2a-3,-2b-3]
*/
	movaps		xmm5, xmm12
	unpckhps	xmm5, xmm13
	unpcklps	xmm12, xmm13
	movaps		xmm6, xmm14
	unpckhps	xmm6, xmm15
	unpcklps	xmm14, xmm15
	xorps		xmm6, RIP_REL_F(dct36_x86_64_sign)

/*
xmm12 [1a-0,1a-1,1b-0,1b-1]
xmm5  [2a-0,2a-1,2b-0,2b-1]
xmm14 [1a-2,1a-3,1b-2,1b-3]
xmm6 [2a-2,2a-3,2b-2,2b-3]
*/

	movaps		xmm0, xmm12
	movlhps		xmm12, xmm14
	movhlps		xmm14, xmm0
	movaps		xmm0, xmm5
	movlhps		xmm0, xmm6
	movhlps		xmm6, xmm5
	movaps		xmm15, xmm6

/*
xmm12 tmp1a
xmm0 tmp2a
xmm14 tmp1b
xmm15 tmp2b
*/

	movaps		xmm6, [tfcos36_]
	movaps		xmm7, 16[tfcos36_]
	movaps		xmm10, xmm15
	addps		xmm15, xmm14
	subps		xmm10, xmm14
	movaps		xmm14, xmm0
	addps		xmm0, xmm12
	subps		xmm14, xmm12
	mulps		xmm15, xmm6
	mulps		xmm7, xmm10

/*
xmm0  tmp[0,1,2,3]
xmm15 tmp[17,16,15,14]
xmm14 tmp[8,7,6,5]
xmm7  tmp[9,10,11,12]
xmm11 tmp[13,-,4,-]
*/

	movaps		xmm1, xmm15
	movups		xmm2, 108[w]
	movups		xmm3, 92[w]
	shufps		xmm3, xmm3, 0x1b
	movups		xmm4, 36[w]
	movups		xmm5, 20[w]
	shufps		xmm5, xmm5, 0x1b
	movaps		xmm6, xmm0
	addps		xmm0, xmm1
	subps		xmm6, xmm1
	mulps		xmm2, xmm0
	mulps		xmm0, xmm3
	mulps		xmm4, xmm6
	mulps		xmm6, xmm5
	movups		xmm1, 36[out1]
	movups		xmm3, 20[out1]
	shufps		xmm6, xmm6, 0x1b
	addps		xmm1, xmm4
	addps		xmm3, xmm6
	shufps		xmm0, xmm0, 0x1b
	movups		36[out2], xmm2
	movups		20[out2], xmm0
	movss		32*36[ts], xmm1
	movss		32*20[ts], xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		32*44[ts], xmm2
	movss		32*28[ts], xmm4
	shufps		xmm1, xmm1, 0xb1
	shufps		xmm3, xmm3, 0xb1
	movss		32*40[ts], xmm1
	movss		32*24[ts], xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		32*48[ts], xmm2
	movss		32*32[ts], xmm4
	
	movhlps		xmm0, xmm11
	movaps		xmm1, xmm11
	movss		xmm2, 124[w]
	movss		xmm3, 88[w]
	movss		xmm4, 52[w]
	movss		xmm5, 16[w]
	movss		xmm6, xmm0
	addss		xmm0, xmm1
	subss		xmm6, xmm1
	mulss		xmm2, xmm0
	mulss		xmm0, xmm3
	mulss		xmm4, xmm6
	mulss		xmm6, xmm5
	addss		xmm4, 52[out1]
	addss		xmm6, 16[out1]
	movss		52[out2], xmm2
	movss		16[out2], xmm0
	movss		32*52[ts], xmm4
	movss		32*16[ts], xmm6
	
	movaps		xmm0, xmm14
	movaps		xmm1, xmm7
	MOVUAPS		xmm2, 128[w]
	movups		xmm3, 72[w]
	shufps		xmm2, xmm2, 0x1b
	movlps		xmm4, 56[w]
	movhps		xmm4, 64[w]
	MOVUAPS		xmm5, [w]
	shufps		xmm4, xmm4, 0x1b
	movaps		xmm6, xmm0
	addps		xmm0, xmm1
	subps		xmm6, xmm1
	mulps		xmm2, xmm0
	mulps		xmm0, xmm3
	mulps		xmm4, xmm6
	mulps		xmm6, xmm5
	movlps		xmm1, 56[out1]
	movhps		xmm1, 64[out1]
	movups		xmm3, [out1]
	shufps		xmm4, xmm4, 0x1b
	addps		xmm3, xmm6
	addps		xmm1, xmm4
	shufps		xmm2, xmm2, 0x1b
	movups		[out2], xmm0
	movlps		56[out2], xmm2
	movhps		64[out2], xmm2
	movss		32*56[ts], xmm1
	movss		[ts], xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		32*64[ts], xmm2
	movss		32*8[ts], xmm4
	shufps		xmm1, xmm1, 0xb1
	shufps		xmm3, xmm3, 0xb1
	movss		32*60[ts], xmm1
	movss		32*4[ts], xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		32*68[ts], xmm2
	movss		32*12[ts], xmm4
	
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
	mov			rsp, rbp
	pop			rbp
#endif
	ret

NONEXEC_STACK
