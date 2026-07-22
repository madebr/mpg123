#include "gas_nasm_compat.h"
/*
	dct36_avx: AVX optimized dct36 for x86-64

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
	void dct36_avx(real *inbuf,real *o1,real *o2,real *wintab,real *tsbuf);
*/
	
	RODATA
	ALIGN16
dct36_avx_COS9:
	DATA_LONG HEX(3f5db3d7)
	DATA_LONG HEX(3f5db3d7)
	DATA_LONG HEX(3f000000)
	DATA_LONG HEX(3f000000)
	DATA_LONG HEX(3f7c1c5c)
	DATA_LONG HEX(3f7c1c5c)
	DATA_LONG HEX(3f708fb2)
	DATA_LONG HEX(3f708fb2)
	DATA_LONG HEX(3f248dbb)
	DATA_LONG HEX(3f248dbb)
	DATA_LONG HEX(3e31d0d4)
	DATA_LONG HEX(3e31d0d4)
	DATA_LONG HEX(3eaf1d44)
	DATA_LONG HEX(3eaf1d44)
	DATA_LONG HEX(3f441b7d)
	DATA_LONG HEX(3f441b7d)
	ALIGN16
dct36_avx_tfcos36:
	DATA_LONG HEX(3f007d2b)
	DATA_LONG HEX(3f0483ee)
	DATA_LONG HEX(3f0d3b7d)
	DATA_LONG HEX(3f1c4257)
	DATA_LONG HEX(40b79454)
	DATA_LONG HEX(3ff746ea)
	DATA_LONG HEX(3f976fd9)
	DATA_LONG HEX(3f5f2944)
	DATA_LONG HEX(3f3504f3)
	ALIGN16
dct36_avx_sign:
	DATA_LONG HEX(80000000),HEX(80000000),HEX(80000000),HEX(80000000)
	SECTION_TEXT
	ALIGN16
	GLOBAL ASM_NAME(INT123_dct36_avx)
ASM_NAME(INT123_dct36_avx):
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
	lea			COS9_, dct36_avx_COS9 RIP_REL
	lea			tfcos36_, dct36_avx_tfcos36 RIP_REL
	
	xorps		xmm4, xmm4
	movups		xmm0, [in]
	movups		xmm1, MEM_OFFSET(in, 16)
	movups		xmm2, MEM_OFFSET(in, 32)
	movups		xmm3, MEM_OFFSET(in, 48)
	movlps		xmm4, QWORD_PTR MEM_OFFSET(in, 64)
	vshufps		xmm5, xmm0, xmm0, HEX(93)
	vshufps		xmm6, xmm1, xmm1, HEX(93)
	vshufps		xmm7, xmm2, xmm2, HEX(93)
	vshufps		xmm8, xmm3, xmm3, HEX(93)
	vshufps		xmm9, xmm4, xmm4, HEX(e1)
	movss		xmm9, xmm8 COMMENT [fg--]
	addps		xmm4, xmm9 COMMENT [gh--]
	movss		xmm8, xmm7
	addps		xmm3, xmm8 COMMENT [cdef]
	movss		xmm7, xmm6
	addps		xmm2, xmm7 COMMENT [89ab]
	movss		xmm6, xmm5
	addps		xmm1, xmm6 COMMENT [4567]
	xorps		xmm6, xmm6
	movss		xmm5, xmm6
	addps		xmm0, xmm5 COMMENT [0123]
	
	vblendps	xmm7, xmm3, xmm6, HEX(5)
	vshufps		xmm4, xmm3, xmm4, HEX(4e)
	addps		xmm4, xmm7
	vblendps	xmm7, xmm2, xmm6, HEX(5)
	vshufps		xmm3, xmm2, xmm3, HEX(4e)
	addps		xmm3, xmm7
	vblendps	xmm7, xmm1, xmm6, HEX(5)
	vshufps		xmm2, xmm1, xmm2, HEX(4e)
	addps		xmm2, xmm7
	vblendps	xmm7, xmm0, xmm6, HEX(5)
	vshufps		xmm1, xmm0, xmm1, HEX(4e)
	addps		xmm1, xmm7
	vmovlhps	xmm0, xmm6, xmm0

/*
xmm0 in[-,-,0,1]
xmm1 in[2,3,4,5]
xmm2 in[6,7,8,9]
xmm3 in[10,11,12,13]
xmm4 in[14,15,16,17]
*/

	vblendps	xmm5, xmm2, xmm3, HEX(c)
	blendps		xmm3, xmm4, HEX(c)
	blendps		xmm4, xmm2, HEX(c)
	movaps		xmm2, xmm5

/*
xmm2 in[6,7,12,13]
xmm3 in[10,11,16,17]
xmm4 in[14,15,8,9]
*/

	movaps		xmm15, [COS9_]
	movaps		xmm6, MEM_OFFSET(COS9_, 16)
	movaps		xmm7, MEM_OFFSET(COS9_, 32)
	movaps		xmm8, MEM_OFFSET(COS9_, 48)
	vmulps		xmm5, xmm15, xmm2
	addps		xmm5, xmm0
	
/*
xmm5 [ta33,tb33,ta66,tb66]
xmm6 COS9_[1,1,2,2]
xmm7 COS9_[5,5,8,8]
xmm8 COS9_[7,7,4,4]
xmm15 COS9_[3,3,6,6]
*/
	
	vmulps		xmm9, xmm6, xmm1
	vmulps		xmm12, xmm7, xmm3
	vmulps		xmm13, xmm8, xmm4
	addps		xmm9, xmm5
	addps		xmm12, xmm13
	addps		xmm12, xmm9
	
	vsubps		xmm13, xmm1, xmm3
	vshufps		xmm14, xmm0, xmm2, HEX(e0)
	vsubps		xmm14, xmm0, xmm14
	subps		xmm13, xmm4
	mulps		xmm13, xmm15
	addps		xmm13, xmm14
	
	vmulps		xmm9, xmm7, xmm1
	vmulps		xmm15, xmm8, xmm3
	vmulps		xmm14, xmm6, xmm4
	subps		xmm9, xmm5
	subps		xmm14, xmm15
	addps		xmm14, xmm9
	
	mulps		xmm8, xmm1
	mulps		xmm6, xmm3
	mulps		xmm7, xmm4
	subps		xmm8, xmm5
	subps		xmm6, xmm7
	vaddps		xmm15, xmm8, xmm6
	
	movss		xmm5, DWORD_PTR MEM_OFFSET(tfcos36_, 32)
	subps		xmm0, xmm1
	subps		xmm4, xmm2
	addps		xmm0, xmm3
	addps		xmm0, xmm4
	shufps		xmm0, xmm0, HEX(af)
	vmulss		xmm11, xmm0, xmm5

/*
xmm12 [1a-0,1b-0, 2a-0, 2b-0]
xmm13 [1a-1,1b-1, 2a-1, 2b-1]
xmm14 [1a-2,1b-2,-2a-2,-2b-2]
xmm15 [1a-3,1b-3,-2a-3,-2b-3]
*/
	vunpckhps	xmm5, xmm12, xmm13
	vunpcklps	xmm12, xmm12, xmm13
	vunpckhps	xmm6, xmm14, xmm15
	vunpcklps	xmm14, xmm14, xmm15
	xorps		xmm6, XMMWORD_PTR RIP_REL_F(dct36_avx_sign)

/*
xmm12 [1a-0,1a-1,1b-0,1b-1]
xmm5  [2a-0,2a-1,2b-0,2b-1]
xmm14 [1a-2,1a-3,1b-2,1b-3]
xmm6 [2a-2,2a-3,2b-2,2b-3]
*/

	vmovlhps	xmm0, xmm12, xmm14
	movhlps		xmm14, xmm12
	vmovlhps	xmm1, xmm5, xmm6
	vmovhlps	xmm15, xmm6, xmm5

/*
xmm0 tmp1a
xmm1 tmp2a
xmm14 tmp1b
xmm15 tmp2b
*/

	movaps		xmm6, [tfcos36_]
	movaps		xmm7, MEM_OFFSET(tfcos36_, 16)
	vsubps		xmm10, xmm15, xmm14
	addps		xmm15, xmm14
	vsubps		xmm14, xmm1, xmm0
	addps		xmm0, xmm1
	vmulps		xmm1, xmm15, xmm6
	mulps		xmm7, xmm10

/*
xmm0 tmp[0,1,2,3]
xmm1 tmp[17,16,15,14]
xmm14 tmp[8,7,6,5]
xmm7 tmp[9,10,11,12]
xmm11 tmp[13,-,4,-]
*/

	movups		xmm2, MEM_OFFSET(w, 108)
	movups		xmm3, MEM_OFFSET(w, 92)
	shufps		xmm3, xmm3, HEX(1b)
	movups		xmm4, MEM_OFFSET(w, 36)
	movups		xmm5, MEM_OFFSET(w, 20)
	shufps		xmm5, xmm5, HEX(1b)
	vsubps		xmm6, xmm0, xmm1
	addps		xmm0, xmm1
	mulps		xmm2, xmm0
	mulps		xmm0, xmm3
	mulps		xmm4, xmm6
	mulps		xmm6, xmm5
	movups		xmm1, MEM_OFFSET(out1, 36)
	movups		xmm3, MEM_OFFSET(out1, 20)
	shufps		xmm6, xmm6, HEX(1b)
	addps		xmm1, xmm4
	addps		xmm3, xmm6
	shufps		xmm0, xmm0, HEX(1b)
	movups		MEM_OFFSET(out2, 36), xmm2
	movups		MEM_OFFSET(out2, 20), xmm0
	movss		DWORD_PTR MEM_OFFSET(ts, 32*36), xmm1
	movss		DWORD_PTR MEM_OFFSET(ts, 32*20), xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		DWORD_PTR MEM_OFFSET(ts, 32*44), xmm2
	movss		DWORD_PTR MEM_OFFSET(ts, 32*28), xmm4
	shufps		xmm1, xmm1, HEX(b1)
	shufps		xmm3, xmm3, HEX(b1)
	movss		DWORD_PTR MEM_OFFSET(ts, 32*40), xmm1
	movss		DWORD_PTR MEM_OFFSET(ts, 32*24), xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		DWORD_PTR MEM_OFFSET(ts, 32*48), xmm2
	movss		DWORD_PTR MEM_OFFSET(ts, 32*32), xmm4
	
	movhlps		xmm0, xmm11
	movss		xmm2, DWORD_PTR MEM_OFFSET(w, 124)
	movss		xmm3, DWORD_PTR MEM_OFFSET(w, 88)
	movss		xmm4, DWORD_PTR MEM_OFFSET(w, 52)
	movss		xmm5, DWORD_PTR MEM_OFFSET(w, 16)
	movss		xmm6, xmm0
	addss		xmm0, xmm11
	subss		xmm6, xmm11
	mulss		xmm2, xmm0
	mulss		xmm0, xmm3
	mulss		xmm4, xmm6
	mulss		xmm6, xmm5
	addss		xmm4, DWORD_PTR MEM_OFFSET(out1, 52)
	addss		xmm6, DWORD_PTR MEM_OFFSET(out1, 16)
	movss		DWORD_PTR MEM_OFFSET(out2, 52), xmm2
	movss		DWORD_PTR MEM_OFFSET(out2, 16), xmm0
	movss		DWORD_PTR MEM_OFFSET(ts, 32*52), xmm4
	movss		DWORD_PTR MEM_OFFSET(ts, 32*16), xmm6
	
	movaps		xmm0, xmm14
	movaps		xmm1, xmm7
	MOVUAPS		xmm2, MEM_OFFSET(w, 128)
	movups		xmm3, MEM_OFFSET(w, 72)
	shufps		xmm2, xmm2, HEX(1b)
	movlps		xmm4, QWORD_PTR MEM_OFFSET(w,56)
	movhps		xmm4, QWORD_PTR MEM_OFFSET(w,64)
	MOVUAPS		xmm5, [w]
	shufps		xmm4, xmm4, HEX(1b)
	vsubps		xmm6, xmm0, xmm1
	addps		xmm0, xmm1
	mulps		xmm2, xmm0
	mulps		xmm0, xmm3
	mulps		xmm4, xmm6
	mulps		xmm6, xmm5
	movlps		xmm1, QWORD_PTR MEM_OFFSET(out1, 56)
	movhps		xmm1, QWORD_PTR MEM_OFFSET(out1, 64)
	movups		xmm3, [out1]
	shufps		xmm4, xmm4, HEX(1b)
	addps		xmm3, xmm6
	addps		xmm1, xmm4
	shufps		xmm2, xmm2, HEX(1b)
	movups		[out2], xmm0
	movlps		QWORD_PTR MEM_OFFSET(out2, 56), xmm2
	movhps		QWORD_PTR MEM_OFFSET(out2, 64), xmm2
	movss		DWORD_PTR MEM_OFFSET(ts, 32*56), xmm1
	movss		DWORD_PTR [ts], xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		DWORD_PTR MEM_OFFSET(ts, 32*64), xmm2
	movss		DWORD_PTR MEM_OFFSET(ts, 32*8), xmm4
	shufps		xmm1, xmm1, HEX(b1)
	shufps		xmm3, xmm3, HEX(b1)
	movss		DWORD_PTR MEM_OFFSET(ts, 32*60), xmm1
	movss		DWORD_PTR MEM_OFFSET(ts, 32*4), xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		DWORD_PTR MEM_OFFSET(ts, 32*68), xmm2
	movss		DWORD_PTR MEM_OFFSET(ts, 32*12), xmm4
	
#ifdef IS_MSABI
	movaps		xmm6, [rsp]
	movaps		xmm7, MEM_OFFSET(rsp, 16)
	movaps		xmm8, MEM_OFFSET(rsp, 32)
	movaps		xmm9, MEM_OFFSET(rsp, 48)
	movaps		xmm10, MEM_OFFSET(rsp, 64)
	movaps		xmm11, MEM_OFFSET(rsp, 80)
	movaps		xmm12, MEM_OFFSET(rsp, 96)
	movaps		xmm13, MEM_OFFSET(rsp, 112)
	movaps		xmm14, MEM_OFFSET(rsp, 128)
	movaps		xmm15, MEM_OFFSET(rsp, 144)
	mov			rsp, rbp
	pop			rbp
#endif
	ret

NONEXEC_STACK

END_MODULE
