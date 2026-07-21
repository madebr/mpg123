#include "gas_nasm_compat.h"
/*
	dct36_sse: SSE optimized dct36

	copyright 1995-2013 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "mangle.h"

#define in edi
#define out1 edi
#define out2 edx
#define w  ecx
#define ts eax
#define tmp esi

/*
	void dct36_sse(real *inbuf,real *o1,real *o2,real *wintab,real *tsbuf);
*/
	
	RODATA
	ALIGN16
dct36_sse_COS9:
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
dct36_sse_tfcos36:
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
dct36_sse_mask:
	DATA_LONG 0,0xffffffff,0,0xffffffff
	ALIGN16
dct36_sse_sign:
	DATA_LONG 0x80000000,0x80000000,0x80000000,0x80000000
	SECTION_TEXT
	ALIGN16
	GLOBAL ASM_NAME(INT123_dct36_sse)
ASM_NAME(INT123_dct36_sse):
	push		ebp
	mov			ebp, esp
	and			esp, -16
	sub			esp, 80
	push		ebx
	push		esi
	push		edi
	lea			tmp, 12[esp]
	mov		in, dword ptr 8[ebp]

	GET_GOT
	
	lea			eax, LOCAL_VAR[dct36_sse_COS9]
	lea			edx, LOCAL_VAR[dct36_sse_tfcos36]

	xorps		xmm0, xmm0
	xorps		xmm5, xmm5
	movlps		xmm5, 64[in]
	movups		xmm4, 48[in]
	movups		xmm3, 32[in]
	movups		xmm2, 16[in]
	movups		xmm1, [in]
	movaps		xmm6, xmm5
	shufps		xmm6, xmm6, 0xe1
	movaps		xmm7, xmm4
	shufps		xmm7, xmm7, 0x93
	movss		xmm6, xmm7
	addps		xmm5, xmm6
	movaps		xmm6, xmm3
	shufps		xmm6, xmm6, 0x93
	movss		xmm7, xmm6
	addps		xmm4, xmm7
	movaps		xmm7, xmm2
	shufps		xmm7, xmm7, 0x93
	movss		xmm6, xmm7
	addps		xmm3, xmm6
	movaps		xmm6, xmm1
	shufps		xmm6, xmm6, 0x93
	movss		xmm7, xmm6
	addps		xmm2, xmm7
	movss		xmm6, xmm0
	addps		xmm1, xmm6
	
	movaps		xmm0, LOCAL_VAR[dct36_sse_mask]
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

	mulps		xmm5, [eax]
	addps		xmm5, xmm0
	
	movaps		[tmp], xmm0
	movaps		16[tmp], xmm2

/*
0(tmp) in[-,-,0,1]
xmm5 [ta33,tb33,ta66,tb66]
*/

	movaps		xmm6, xmm1
	subps		xmm6, xmm3
	subps		xmm6, xmm4
	xorps		xmm7, xmm7
	shufps		xmm7, xmm2, 0xe0
	mulps		xmm6, [eax]
	subps		xmm0, xmm7
	addps		xmm6, xmm0
	movaps		48[tmp], xmm6
	
	movaps		xmm2, 16[eax]

	movaps		xmm0, xmm1
	movaps		xmm6, xmm3
	movaps		xmm7, xmm4
	mulps		xmm0, xmm2
	mulps		xmm6, 32[eax]
	mulps		xmm7, 48[eax]
	addps		xmm0, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm6
	movaps		32[tmp], xmm0
	
	movaps		xmm0, xmm1
	movaps		xmm6, xmm3
	movaps		xmm7, xmm4
	mulps		xmm0, 32[eax]
	mulps		xmm6, 48[eax]
	mulps		xmm7, xmm2
	subps		xmm0, xmm5
	subps		xmm7, xmm6
	addps		xmm0, xmm7
	movaps		64[tmp], xmm0
	
	movaps		xmm6, xmm1
	movaps		xmm7, xmm4
	mulps		xmm6, 48[eax]
	mulps		xmm2, xmm3
	mulps		xmm7, 32[eax]
	subps		xmm6, xmm5
	subps		xmm2, xmm7
	addps		xmm6, xmm2
	
	movaps		xmm0, [tmp]
	movss		xmm5, 32[edx]
	subps		xmm0, xmm1
	subps		xmm4, 16[tmp]
	addps		xmm0, xmm3
	addps		xmm0, xmm4
	shufps		xmm0, xmm0, 0xaf
	mulss		xmm0, xmm5
	movaps		[tmp], xmm0
	
	movaps		xmm0, 32[tmp]
	movaps		xmm1, 48[tmp]
	movaps		xmm2, 64[tmp]

/*
xmm0 [1a-0,1b-0, 2a-0, 2b-0]
xmm1 [1a-1,1b-1, 2a-1, 2b-1]
xmm2 [1a-2,1b-2,-2a-2,-2b-2]
xmm6 [1a-3,1b-3,-2a-3,-2b-3]
*/

	movaps		xmm3, xmm0
	unpcklps	xmm0, xmm1
	unpckhps	xmm3, xmm1
	movaps		xmm5, xmm2
	unpcklps	xmm2, xmm6
	unpckhps	xmm5, xmm6
	xorps		xmm5, LOCAL_VAR[dct36_sse_sign]

/*
xmm0 [1a-0,1a-1,1b-0,1b-1]
xmm3 [2a-0,2a-1,2b-0,2b-1]
xmm2 [1a-2,1a-3,1b-2,1b-3]
xmm5 [2a-2,2a-3,2b-2,2b-3]
*/

	movaps		xmm1, xmm0
	movlhps		xmm0, xmm2
	movhlps		xmm2, xmm1
	movaps		xmm4, xmm3
	movlhps		xmm3, xmm5
	movhlps		xmm5, xmm4

/*
xmm0 tmp1a
xmm3 tmp2a
xmm2 tmp1b
xmm5 tmp2b
*/

	movaps		xmm6, [edx]
	movaps		xmm7, 16[edx]
	movaps		xmm1, xmm5
	addps		xmm5, xmm2
	subps		xmm1, xmm2
	movaps		xmm2, xmm3
	addps		xmm3, xmm0
	subps		xmm2, xmm0
	mulps		xmm5, xmm6
	mulps		xmm7, xmm1
	
	movaps		16[tmp], xmm2

/*
xmm3 tmp[0,1,2,3]
xmm5 tmp[17,16,15,14]
16[tmp] tmp[8,7,6,5]
xmm7 tmp[9,10,11,12]
0[tmp] tmp[13,-,4,-]
*/

	mov		out1, dword ptr 12[ebp]
	mov		out2, dword ptr 16[ebp]
	mov		w, dword ptr 20[ebp]
	mov		ts, dword ptr 24[ebp]

	movaps		xmm0, xmm3
	movaps		xmm1, xmm5
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
	
	movss		xmm0, 8[tmp]
	movss		xmm1, [tmp]
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
	
	movaps		xmm0, 16[tmp]
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
	
	pop			edi
	pop			esi
	pop			ebx
	mov			esp, ebp
	pop			ebp
	
	ret

NONEXEC_STACK
