#include "gas_nasm_compat.h"
/*
	dct64_sse: MMX/SSE optimized dct64

	copyright 1995-2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "mangle.h"

#define ARG(n) [ebp+8+n*4]
#define TEMP(n) [esp+4+n*16]
#define TEMP_BYTE(n) [esp+4+n]

/*
	void dct64_sse(short *out0, short *out1, real *samples);
*/

	RODATA
	ALIGN16
pnpn:
	DATA_LONG	0
	DATA_LONG	-2147483648
	DATA_LONG	0
	DATA_LONG	-2147483648
	ALIGN16
mask:
	DATA_LONG	-1
	DATA_LONG	-1
	DATA_LONG	-1
	DATA_LONG	0
	
	SECTION_TEXT
	ALIGN16
GLOBAL ASM_NAME(INT123_dct64_sse)
ASM_NAME(INT123_dct64_sse):
	push		ebp
	mov		ebp, esp
	
	and		esp, -16 /* align the stack at 16 bytes */
	sub		esp, 128 /* reserve space for temporal store */
	push		ebx

	GET_GOT

	mov		eax, DWORD_PTR ARG(2)
	
	MOVUAPS 	xmm7, [eax]
	MOVUAPS 	xmm6, [eax+16]
	MOVUAPS 	xmm0, [eax+112]
	MOVUAPS 	xmm1, [eax+96]
	shufps 		xmm0, xmm0, HEX(1b)
	shufps 		xmm1, xmm1, HEX(1b)
	movaps 		xmm4, xmm7
	movaps		xmm5, xmm6
	addps 		xmm4, xmm0
	addps 		xmm5, xmm1
	subps 		xmm7, xmm0
	subps 		xmm6, xmm1
	movaps		TEMP(0), xmm4
	movaps		TEMP(1), xmm5
	
	MOVUAPS 	xmm2, [eax+32]
	MOVUAPS 	xmm3, [eax+48]
	MOVUAPS 	xmm0, [eax+80]
	MOVUAPS 	xmm1, [eax+64]
	shufps 		xmm0, xmm0, HEX(1b)
	shufps 		xmm1, xmm1, HEX(1b)
	movaps 		xmm5, xmm2
	movaps		xmm4, xmm3
	addps 		xmm2, xmm0
	addps 		xmm3, xmm1
	subps 		xmm5, xmm0
	subps 		xmm4, xmm1
	
#ifdef PIC_GLOBAL_PTR
	mov			ecx, GLOBAL_VAR_PTR[INT123_costab_mmxsse]
#else
	lea			ecx, GLOBAL_VAR[INT123_costab_mmxsse]
#endif
	mulps		xmm7, [ecx]
	mulps		xmm6, [ecx+16]
	mulps		xmm5, [ecx+32]
	mulps		xmm4, [ecx+48]
	
	shufps		xmm2, xmm2, HEX(1b)
	shufps		xmm3, xmm3, HEX(1b)
	shufps		xmm4, xmm4, HEX(1b)
	shufps		xmm5, xmm5, HEX(1b)
	movaps		xmm0, TEMP(0)
	movaps		xmm1, TEMP(1)
	subps		xmm0, xmm3
	subps		xmm1, xmm2
	addps		xmm3, TEMP(0)
	addps		xmm2, TEMP(1)
	movaps		TEMP(0), xmm3
	movaps		TEMP(1), xmm2
	movaps		xmm2, xmm6
	movaps		xmm3, xmm7
	subps		xmm6, xmm5
	subps		xmm7, xmm4
	addps		xmm4, xmm3
	addps		xmm5, xmm2
	mulps		xmm0, [ecx+64]
	mulps		xmm1, [ecx+80]
	mulps		xmm6, [ecx+80]
	mulps		xmm7, [ecx+64]
	
	movaps		xmm2, TEMP(0)
	movaps		xmm3, TEMP(1)
	shufps		xmm3, xmm3, HEX(1b)
	shufps		xmm5, xmm5, HEX(1b)
	shufps		xmm1, xmm1, HEX(1b)
	shufps		xmm6, xmm6, HEX(1b)
	movaps		TEMP(1), xmm0
	subps		xmm2, xmm3
	subps		xmm0, xmm1
	addps		xmm3, TEMP(0)
	addps		xmm1, TEMP(1)
	movaps		TEMP(0), xmm3
	movaps		TEMP(2), xmm1
	movaps		xmm1, xmm5
	movaps		xmm5, xmm4
	movaps		xmm3, xmm7
	subps		xmm5, xmm1
	subps		xmm7, xmm6
	addps		xmm4, xmm1
	addps		xmm6, xmm3
	mulps		xmm2, 96[ecx]
	mulps		xmm0, 96[ecx]
	mulps		xmm5, 96[ecx]
	mulps		xmm7, 96[ecx]
	movaps		TEMP(1), xmm2
	movaps		TEMP(3), xmm0
	
	movaps		xmm2, xmm4
	movaps		xmm3, xmm5
	shufps		xmm2, xmm6, HEX(44)
	shufps		xmm5, xmm7, HEX(bb)
	shufps		xmm4, xmm6, HEX(bb)
	shufps		xmm3, xmm7, HEX(44)
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	subps		xmm2, xmm4
	subps		xmm3, xmm5
	addps		xmm4, xmm6
	addps		xmm5, xmm7
	movaps		xmm0, 112[ecx]
	movlhps		xmm0, xmm0
	mulps		xmm2, xmm0
	mulps		xmm3, xmm0
	movaps		TEMP(4), xmm0
	movaps		xmm6, xmm4
	movaps		xmm7, xmm5
	shufps		xmm4, xmm2, HEX(14)
	shufps		xmm6, xmm2, HEX(be)
	shufps		xmm5, xmm3, HEX(14)
	shufps		xmm7, xmm3, HEX(be)
	movaps		TEMP(5), xmm5
	movaps		TEMP(7), xmm7
	
	movaps		xmm0, TEMP(0)
	movaps		xmm1, TEMP(1)
	movaps		xmm2, xmm0
	movaps		xmm3, xmm1
	shufps		xmm2, TEMP(2), HEX(44)
	shufps		xmm1, TEMP(3), HEX(bb)
	shufps		xmm0, TEMP(2), HEX(bb)
	shufps		xmm3, TEMP(3), HEX(44)
	movaps		xmm5, xmm2
	movaps		xmm7, xmm3
	subps		xmm2, xmm0
	subps		xmm3, xmm1
	addps		xmm0, xmm5
	addps		xmm1, xmm7
	mulps		xmm2, TEMP(4)
	mulps		xmm3, TEMP(4)
	movaps		xmm5, xmm0
	movaps		xmm7, xmm1
	shufps		xmm0, xmm2, HEX(14)
	shufps		xmm5, xmm2, HEX(be)
	shufps		xmm1, xmm3, HEX(14)
	shufps		xmm7, xmm3, HEX(be)
	
	movaps		TEMP(0), xmm0
	movaps		TEMP(1), xmm1
	movaps		TEMP(2), xmm5
	movaps		TEMP(3), xmm7
	
	movss		xmm5, [ecx+120]
	shufps		xmm5, xmm5, HEX(00)
	xorps		xmm5, [LOCAL_VAR(pnpn)]
	
	movaps		xmm0, xmm4
	movaps		xmm1, xmm6
	unpcklps	xmm4, TEMP(5)
	unpckhps	xmm0, TEMP(5)
	unpcklps	xmm6, TEMP(7)
	unpckhps	xmm1, TEMP(7)
	movaps		xmm2, xmm4
	movaps		xmm3, xmm6
	unpcklps	xmm4, xmm0
	unpckhps	xmm2, xmm0
	unpcklps	xmm6, xmm1
	unpckhps	xmm3, xmm1
	movaps		xmm0, xmm4
	movaps		xmm1, xmm6
	subps		xmm0, xmm2
	subps		xmm1, xmm3
	addps		xmm4, xmm2
	addps		xmm6, xmm3
	mulps		xmm0, xmm5
	mulps		xmm1, xmm5
	movaps		TEMP(5), xmm5
	movaps		xmm5, xmm4
	movaps		xmm7, xmm6
	unpcklps	xmm4, xmm0
	unpckhps	xmm5, xmm0
	unpcklps	xmm6, xmm1
	unpckhps	xmm7, xmm1
	
	movaps		xmm0, TEMP(0)
	movaps		xmm2, TEMP(2)
	movaps		TEMP(4), xmm4
	movaps		TEMP(6), xmm6
	
	movaps		xmm4, xmm0
	movaps		xmm6, xmm2
	unpcklps	xmm0, TEMP(1)
	unpckhps	xmm4, TEMP(1)
	unpcklps	xmm2, TEMP(3)
	unpckhps	xmm6, TEMP(3)
	movaps		xmm1, xmm0
	movaps		xmm3, xmm2
	unpcklps	xmm0, xmm4
	unpckhps	xmm1, xmm4
	unpcklps	xmm2, xmm6
	unpckhps	xmm3, xmm6
	movaps		xmm4, xmm0
	movaps		xmm6, xmm2
	subps		xmm4, xmm1
	subps		xmm6, xmm3
	addps		xmm0, xmm1
	addps		xmm2, xmm3
	mulps		xmm4, TEMP(5)
	mulps		xmm6, TEMP(5)
	movaps		xmm1, xmm0
	movaps		xmm3, xmm2
	unpcklps	xmm0, xmm4
	unpckhps	xmm1, xmm4
	unpcklps	xmm2, xmm6
	unpckhps	xmm3, xmm6
	
	movaps		TEMP(0), xmm0
	movaps		TEMP(1), xmm1
	movaps		TEMP(2), xmm2
	movaps		TEMP(3), xmm3
	movaps		TEMP(5), xmm5
	movaps		TEMP(7), xmm7
	
	movss		xmm0, TEMP_BYTE(12)
	movss		xmm1, TEMP_BYTE(28)
	movss		xmm2, TEMP_BYTE(44)
	movss		xmm3, TEMP_BYTE(60)
	addss		xmm0, TEMP_BYTE(8)
	addss		xmm1, TEMP_BYTE(24)
	addss		xmm2, TEMP_BYTE(40)
	addss		xmm3, TEMP_BYTE(56)
	movss		TEMP_BYTE(8), xmm0
	movss		TEMP_BYTE(24), xmm1
	movss		TEMP_BYTE(40), xmm2
	movss		TEMP_BYTE(56), xmm3
	movss		xmm0, TEMP_BYTE(76)
	movss		xmm1, TEMP_BYTE(92)
	movss		xmm2, TEMP_BYTE(108)
	movss		xmm3, TEMP_BYTE(124)
	addss		xmm0, TEMP_BYTE(72)
	addss		xmm1, TEMP_BYTE(88)
	addss		xmm2, TEMP_BYTE(104)
	addss		xmm3, TEMP_BYTE(120)
	movss		TEMP_BYTE(72), xmm0
	movss		TEMP_BYTE(88), xmm1
	movss		TEMP_BYTE(104), xmm2
	movss		TEMP_BYTE(120), xmm3
	
	movaps		xmm1, TEMP_BYTE(16)
	movaps		xmm3, TEMP_BYTE(48)
	movaps		xmm5, TEMP_BYTE(80)
	movaps		xmm7, TEMP_BYTE(112)
	movaps		xmm0, xmm1
	movaps		xmm2, xmm3
	movaps		xmm4, xmm5
	movaps		xmm6, xmm7
	shufps		xmm0, xmm0, HEX(1e)
	shufps		xmm2, xmm2, HEX(1e)
	shufps		xmm4, xmm4, HEX(1e)
	shufps		xmm6, xmm6, HEX(1e)
	lea		ecx, DWORD_PTR LOCAL_VAR[mask]
	andps		xmm0, [ecx]
	andps		xmm2, [ecx]
	andps		xmm4, [ecx]
	andps		xmm6, [ecx]
	addps		xmm1, xmm0
	addps		xmm3, xmm2
	addps		xmm5, xmm4
	addps		xmm7, xmm6
	
	movaps		xmm2, TEMP_BYTE(32)
	movaps		xmm6, TEMP_BYTE(96)
	movaps		xmm0, xmm2
	movaps		xmm4, xmm6
	shufps		xmm0, xmm0, HEX(1e)
	shufps		xmm4, xmm4, HEX(1e)
	andps		xmm0, [ecx]
	andps		xmm4, [ecx]
	addps		xmm2, xmm3
	addps		xmm3, xmm0
	addps		xmm6, xmm7
	addps		xmm7, xmm4
	
	movaps		xmm0, TEMP_BYTE(0)
	movaps		xmm4, TEMP_BYTE(64)
	
	cvtps2pi	mm0, xmm0
	cvtps2pi	mm1, xmm1
	movhlps		xmm0, xmm0
	movhlps		xmm1, xmm1
	cvtps2pi	mm2, xmm0
	cvtps2pi	mm3, xmm1
	packssdw	mm0, mm2
	packssdw	mm1, mm3
	
	cvtps2pi	mm2, xmm2
	cvtps2pi	mm3, xmm3
	movhlps		xmm2, xmm2
	movhlps		xmm3, xmm3
	cvtps2pi	mm4, xmm2
	cvtps2pi	mm5, xmm3
	packssdw	mm2, mm4
	packssdw	mm3, mm5
	
	mov			TEMP_BYTE(0), ecx
	mov		ecx, DWORD_PTR ARG(0)
	mov		ebx, DWORD_PTR ARG(1)
	
	movd		eax, mm0
	movd		edx, mm1
	mov		word ptr 512[ecx], ax
	mov		word ptr 384[ecx], dx
	shr		eax, 16
	shr		edx, 16
	mov		word ptr [ecx], ax
	mov		word ptr [ebx], ax
	mov		word ptr 128[ebx], dx
	
	movd		eax, mm2
	movd		edx, mm3
	mov		word ptr 448[ecx], ax
	mov		word ptr 320[ecx], dx
	shr		eax, 16
	shr		edx, 16
	mov		word ptr 64[ebx], ax
	mov		word ptr 192[ebx], dx
	
	psrlq		mm0, 32
	psrlq		mm1, 32
	movd		eax, mm0
	movd		edx, mm1
	mov		word ptr 256[ecx], ax
	mov		word ptr 128[ecx], dx
	shr		eax, 16
	shr		edx, 16
	mov		word ptr 256[ebx], ax
	mov		word ptr 384[ebx], dx
	
	psrlq		mm2, 32
	psrlq		mm3, 32
	movd		eax, mm2
	movd		edx, mm3
	mov		word ptr 192[ecx], ax
	mov		word ptr 64[ecx], dx
	shr		eax, 16
	shr		edx, 16
	mov		word ptr 320[ebx], ax
	mov		word ptr 448[ebx], dx
	
	mov			eax, TEMP_BYTE(0)
	movaps		xmm0, xmm4
	shufps		xmm0, xmm0, HEX(1e)
	movaps		xmm1, xmm5
	andps		xmm0, [eax]
	
	addps		xmm4, xmm6
	addps		xmm5, xmm7
	addps		xmm6, xmm1
	addps		xmm7, xmm0
	
	cvtps2pi	mm0, xmm4
	cvtps2pi	mm1, xmm5
	movhlps		xmm4, xmm4
	movhlps		xmm5, xmm5
	cvtps2pi	mm2, xmm4
	cvtps2pi	mm3, xmm5
	packssdw	mm0, mm2
	packssdw	mm1, mm3
	
	cvtps2pi	mm2, xmm6
	cvtps2pi	mm3, xmm7
	movhlps		xmm6, xmm6
	movhlps		xmm7, xmm7
	cvtps2pi	mm4, xmm6
	cvtps2pi	mm5, xmm7
	packssdw	mm2, mm4
	packssdw	mm3, mm5
	
	movd		eax, mm0
	movd		edx, mm2
	mov		word ptr 480[ecx], ax
	mov		word ptr 416[ecx], dx
	shr		eax, 16
	shr		edx, 16
	mov		word ptr 32[ebx], ax
	mov		word ptr 96[ebx], dx
	
	psrlq		mm0, 32
	psrlq		mm2, 32
	movd		eax, mm0
	movd		edx, mm2
	mov		word ptr 224[ecx], ax
	mov		word ptr 160[ecx], dx
	shr		eax, 16
	shr		edx, 16
	mov		word ptr 288[ebx], ax
	mov		word ptr 352[ebx], dx
	
	movd		eax, mm1
	movd		edx, mm3
	mov		word ptr 352[ecx], ax
	mov		word ptr 288[ecx], dx
	shr		eax, 16
	shr		edx, 16
	mov		word ptr 160[ebx], ax
	mov		word ptr 224[ebx], dx
	
	psrlq		mm1, 32
	psrlq		mm3, 32
	movd		eax, mm1
	movd		edx, mm3
	mov		word ptr 96[ecx], ax
	mov		word ptr 32[ecx], dx
	shr		eax, 16
	shr		edx, 16
	mov		word ptr 416[ebx], ax
	mov		word ptr 480[ebx], dx

	pop		ebx
	mov		esp, ebp
	pop		ebp
	ret

#if defined(PIC) && defined(__APPLE__)
	SECTION	__IMPORT,__pointers,non_lazy_symbol_pointers
L_INT123_costab_mmxsse:
	.indirect_symbol	ASM_NAME(INT123_costab_mmxsse)
	DATA_LONG	0
#endif

NONEXEC_STACK
