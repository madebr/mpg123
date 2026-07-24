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
dct36_sse_tfcos36:
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
dct36_sse_mask:
	DATA_LONG 0,HEX(ffffffff),0,HEX(ffffffff)
	ALIGN16
dct36_sse_sign:
	DATA_LONG HEX(80000000),HEX(80000000),HEX(80000000),HEX(80000000)
	CPU_686
	XMM
	FLAT_MODEL
	SECTION_TEXT
	ALIGN_CODE(16)
	GLOBAL ASM_NAME(INT123_dct36_sse)
ASM_NAME(INT123_dct36_sse):
	push		ebp
	mov			ebp, esp
	and			esp, -16
	sub			esp, 80
	push		ebx
	push		esi
	push		edi
	lea			tmp, [esp+12]
	mov		in, DWORD_PTR [ebp+8]

	GET_GOT
	
	LEA(		eax, LOCAL_VAR(dct36_sse_COS9))
	LEA(		edx, LOCAL_VAR(dct36_sse_tfcos36))

	xorps		xmm0, xmm0
	xorps		xmm5, xmm5
	movlps		xmm5, QWORD_PTR [in+64]
	movups		xmm4, [in+48]
	movups		xmm3, [in+32]
	movups		xmm2, [in+16]
	movups		xmm1, [in]
	movaps		xmm6, xmm5
	shufps		xmm6, xmm6, HEX(e1)
	movaps		xmm7, xmm4
	shufps		xmm7, xmm7, HEX(93)
	movss		xmm6, xmm7
	addps		xmm5, xmm6
	movaps		xmm6, xmm3
	shufps		xmm6, xmm6, HEX(93)
	movss		xmm7, xmm6
	addps		xmm4, xmm7
	movaps		xmm7, xmm2
	shufps		xmm7, xmm7, HEX(93)
	movss		xmm6, xmm7
	addps		xmm3, xmm6
	movaps		xmm6, xmm1
	shufps		xmm6, xmm6, HEX(93)
	movss		xmm7, xmm6
	addps		xmm2, xmm7
	movss		xmm6, xmm0
	addps		xmm1, xmm6
	
	movaps		xmm0, XMMWORD_PTR [LOCAL_VAR(dct36_sse_mask)]
	movaps		xmm6, xmm4
	shufps		xmm4, xmm5, HEX(4e)
	movaps		xmm7, xmm3
	shufps		xmm3, xmm6, HEX(4e)
	andps		xmm6, xmm0
	addps		xmm4, xmm6
	movaps		xmm6, xmm2
	shufps		xmm2, xmm7, HEX(4e)
	andps		xmm7, xmm0
	addps		xmm3, xmm7
	movaps		xmm7, xmm1
	shufps		xmm1, xmm6, HEX(4e)
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
	shufps		xmm5, xmm3, HEX(e4)
	shufps		xmm3, xmm4, HEX(e4)
	shufps		xmm4, xmm2, HEX(e4)
	movaps		xmm2, xmm5

/*
xmm2 in[6,7,12,13]
xmm3 in[10,11,16,17]
xmm4 in[14,15,8,9]
*/

	mulps		xmm5, [eax]
	addps		xmm5, xmm0
	
	movaps		XMMWORD_PTR [tmp], xmm0
	movaps		XMMWORD_PTR [tmp+16], xmm2

/*
0(tmp) in[-,-,0,1]
xmm5 [ta33,tb33,ta66,tb66]
*/

	movaps		xmm6, xmm1
	subps		xmm6, xmm3
	subps		xmm6, xmm4
	xorps		xmm7, xmm7
	shufps		xmm7, xmm2, HEX(e0)
	mulps		xmm6, [eax]
	subps		xmm0, xmm7
	addps		xmm6, xmm0
	movaps		[tmp+48], xmm6
	
	movaps		xmm2, [eax+16]

	movaps		xmm0, xmm1
	movaps		xmm6, xmm3
	movaps		xmm7, xmm4
	mulps		xmm0, xmm2
	mulps		xmm6, [eax+32]
	mulps		xmm7, [eax+48]
	addps		xmm0, xmm5
	addps		xmm6, xmm7
	addps		xmm0, xmm6
	movaps		[tmp+32], xmm0
	
	movaps		xmm0, xmm1
	movaps		xmm6, xmm3
	movaps		xmm7, xmm4
	mulps		xmm0, [eax+32]
	mulps		xmm6, [eax+48]
	mulps		xmm7, xmm2
	subps		xmm0, xmm5
	subps		xmm7, xmm6
	addps		xmm0, xmm7
	movaps		[tmp+64], xmm0
	
	movaps		xmm6, xmm1
	movaps		xmm7, xmm4
	mulps		xmm6, [eax+48]
	mulps		xmm2, xmm3
	mulps		xmm7, [eax+32]
	subps		xmm6, xmm5
	subps		xmm2, xmm7
	addps		xmm6, xmm2
	
	movaps		xmm0, [tmp]
	movss		xmm5, DWORD_PTR [edx+32]
	subps		xmm0, xmm1
	subps		xmm4, [tmp+16]
	addps		xmm0, xmm3
	addps		xmm0, xmm4
	shufps		xmm0, xmm0, HEX(af)
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
	xorps		xmm5, XMMWORD_PTR [LOCAL_VAR(dct36_sse_sign)]

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

	mov		out1, DWORD_PTR 12[ebp]
	mov		out2, DWORD_PTR 16[ebp]
	mov		w, DWORD_PTR 20[ebp]
	mov		ts, DWORD_PTR 24[ebp]

	movaps		xmm0, xmm3
	movaps		xmm1, xmm5
	movups		xmm2, [w+108]
	movups		xmm3, [w+92]
	shufps		xmm3, xmm3, HEX(1b)
	movups		xmm4, [w+36]
	movups		xmm5, [w+20]
	shufps		xmm5, xmm5, HEX(1b)
	movaps		xmm6, xmm0
	addps		xmm0, xmm1
	subps		xmm6, xmm1
	mulps		xmm2, xmm0
	mulps		xmm0, xmm3
	mulps		xmm4, xmm6
	mulps		xmm6, xmm5
	movups		xmm1, [out1+36]
	movups		xmm3, [out1+20]
	shufps		xmm6, xmm6, HEX(1b)
	addps		xmm1, xmm4
	addps		xmm3, xmm6
	shufps		xmm0, xmm0, HEX(1b)
	movups		[out2+36], xmm2
	movups		[out2+20], xmm0
	movss		DWORD_PTR[ts+32*36], xmm1
	movss		DWORD_PTR[ts+32*20], xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		DWORD_PTR [ts+32*44], xmm2
	movss		DWORD_PTR [ts+32*28], xmm4
	shufps		xmm1, xmm1, HEX(b1)
	shufps		xmm3, xmm3, HEX(b1)
	movss		DWORD_PTR [ts+32*40], xmm1
	movss		DWORD_PTR [ts+32*24], xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		DWORD_PTR [ts+32*48], xmm2
	movss		DWORD_PTR [ts+32*32], xmm4
	
	movss		xmm0, DWORD_PTR [tmp+8]
	movss		xmm1, DWORD_PTR [tmp]
	movss		xmm2, DWORD_PTR [w+124]
	movss		xmm3, DWORD_PTR [w+88]
	movss		xmm4, DWORD_PTR [w+52]
	movss		xmm5, DWORD_PTR [w+16]
	movss		xmm6, xmm0
	addss		xmm0, xmm1
	subss		xmm6, xmm1
	mulss		xmm2, xmm0
	mulss		xmm0, xmm3
	mulss		xmm4, xmm6
	mulss		xmm6, xmm5
	addss		xmm4, DWORD_PTR [out1+52]
	addss		xmm6, DWORD_PTR [out1+16]
	movss		DWORD_PTR [out2+52], xmm2
	movss		DWORD_PTR [out2+16], xmm0
	movss		DWORD_PTR [ts+32*52], xmm4
	movss		DWORD_PTR [ts+32*16], xmm6
	
	movaps		xmm0, [tmp+16]
	movaps		xmm1, xmm7
	MOVUAPS		xmm2, [w+128]
	movups		xmm3, [w+72]
	shufps		xmm2, xmm2, HEX(1b)
	movlps		xmm4, QWORD_PTR [w+56]
	movhps		xmm4, QWORD_PTR [w+64]
	MOVUAPS		xmm5, [w]
	shufps		xmm4, xmm4, HEX(1b)
	movaps		xmm6, xmm0
	addps		xmm0, xmm1
	subps		xmm6, xmm1
	mulps		xmm2, xmm0
	mulps		xmm0, xmm3
	mulps		xmm4, xmm6
	mulps		xmm6, xmm5
	movlps		xmm1, QWORD_PTR [out1+56]
	movhps		xmm1, QWORD_PTR [out1+64]
	movups		xmm3, [out1]
	shufps		xmm4, xmm4, HEX(1b)
	addps		xmm3, xmm6
	addps		xmm1, xmm4
	shufps		xmm2, xmm2, HEX(1b)
	movups		[out2], xmm0
	movlps		QWORD_PTR [out2+56], xmm2
	movhps		QWORD_PTR [out2+64], xmm2
	movss		DWORD_PTR [ts+32*56], xmm1
	movss		DWORD_PTR [ts], xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		DWORD_PTR [ts+32*64], xmm2
	movss		DWORD_PTR [ts+32*8], xmm4
	shufps		xmm1, xmm1, HEX(b1)
	shufps		xmm3, xmm3, HEX(b1)
	movss		DWORD_PTR [ts+32*60], xmm1
	movss		DWORD_PTR [ts+32*4], xmm3
	movhlps		xmm2, xmm1
	movhlps		xmm4, xmm3
	movss		DWORD_PTR [ts+32*68], xmm2
	movss		DWORD_PTR [ts+32*12], xmm4
	
	pop			edi
	pop			esi
	pop			ebx
	mov			esp, ebp
	pop			ebp
	
	ret

NONEXEC_STACK

END_MODULE
