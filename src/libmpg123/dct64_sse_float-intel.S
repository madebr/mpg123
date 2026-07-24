#include "gas_nasm_compat.h"
/*
	dct64_sse_float: SSE optimized dct64 (float output version)

	copyright 1995-2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "mangle.h"

#define ARG(n) [ebp+8+n*4]
#define TEMP(n) [esp+4+n*16]
#define TEMP_BYTE(n) [esp+4+n]

/*
	void dct64_real_sse(real *out0, real *out1, real *samples);
*/

	RODATA
	ALIGN16
pnpn:
	DATA_LONG	0
	DATA_LONG	-2147483648
	DATA_LONG	0
	DATA_LONG	-2147483648
	ALIGN16
masks:
	DATA_LONG	-1
	DATA_LONG	-1
	DATA_LONG	-1
	DATA_LONG	0

EXTERN(ASM_NAME(INT123_costab_mmxsse))

	SECTION_TEXT
	ALIGN 16
GLOBAL ASM_NAME(INT123_dct64_real_sse)
ASM_NAME(INT123_dct64_real_sse):
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
	mov			eax, [GLOBAL_VAR_PTR(INT123_costab_mmxsse)]
#else
	lea			eax, [GLOBAL_VAR(INT123_costab_mmxsse)]
#endif
	mulps		xmm7, [eax]
	mulps		xmm6, [eax+16]
	mulps		xmm5, [eax+32]
	mulps		xmm4, [eax+48]
	
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
	mulps		xmm0, [eax+64]
	mulps		xmm1, [eax+80]
	mulps		xmm6, [eax+80]
	mulps		xmm7, [eax+64]
	
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
	mulps		xmm2, [eax+96]
	mulps		xmm0, [eax+96]
	mulps		xmm5, [eax+96]
	mulps		xmm7, [eax+96]
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
	movaps		xmm0, [eax+112]
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
	
	movss		xmm5, DWORD_PTR [eax+120]
	shufps		xmm5, xmm5, HEX(00)
	xorps		xmm5, XMMWORD_PTR [LOCAL_VAR(pnpn)]
	
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
	
	movss		xmm0, DWORD_PTR TEMP_BYTE(12)
	movss		xmm1, DWORD_PTR TEMP_BYTE(28)
	movss		xmm2, DWORD_PTR TEMP_BYTE(44)
	movss		xmm3, DWORD_PTR TEMP_BYTE(60)
	addss		xmm0, DWORD_PTR TEMP_BYTE(8)
	addss		xmm1, DWORD_PTR TEMP_BYTE(24)
	addss		xmm2, DWORD_PTR TEMP_BYTE(40)
	addss		xmm3, DWORD_PTR TEMP_BYTE(56)
	movss		DWORD_PTR TEMP_BYTE(8), xmm0
	movss		DWORD_PTR TEMP_BYTE(24), xmm1
	movss		DWORD_PTR TEMP_BYTE(40), xmm2
	movss		DWORD_PTR TEMP_BYTE(56), xmm3
	movss		xmm0, DWORD_PTR TEMP_BYTE(76)
	movss		xmm1, DWORD_PTR TEMP_BYTE(92)
	movss		xmm2, DWORD_PTR TEMP_BYTE(108)
	movss		xmm3, DWORD_PTR TEMP_BYTE(124)
	addss		xmm0, DWORD_PTR TEMP_BYTE(72)
	addss		xmm1, DWORD_PTR TEMP_BYTE(88)
	addss		xmm2, DWORD_PTR TEMP_BYTE(104)
	addss		xmm3, DWORD_PTR TEMP_BYTE(120)
	movss		DWORD_PTR TEMP_BYTE(72), xmm0
	movss		DWORD_PTR TEMP_BYTE(88), xmm1
	movss		DWORD_PTR TEMP_BYTE(104), xmm2
	movss		DWORD_PTR TEMP_BYTE(120), xmm3
	
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
	LEA(		eax, LOCAL_VAR(masks))
	andps		xmm0, [eax]
	andps		xmm2, [eax]
	andps		xmm4, [eax]
	andps		xmm6, [eax]
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
	andps		xmm0, [eax]
	andps		xmm4, [eax]
	addps		xmm2, xmm3
	addps		xmm3, xmm0
	addps		xmm6, xmm7
	addps		xmm7, xmm4
	
	movaps		xmm0, TEMP_BYTE(0)
	movaps		xmm4, TEMP_BYTE(64)
	
	mov		ecx, DWORD_PTR ARG(0)
	mov		ebx, DWORD_PTR ARG(1)
	
	movss		DWORD_PTR [ecx+1024], xmm0
	movss		DWORD_PTR [ecx+896], xmm2
	movss		DWORD_PTR [ecx+768], xmm1
	movss		DWORD_PTR [ecx+640], xmm3
	
	shufps		xmm0, xmm0, HEX(e1)
	shufps		xmm2, xmm2, HEX(e1)
	shufps		xmm1, xmm1, HEX(e1)
	shufps		xmm3, xmm3, HEX(e1)
	movss		DWORD_PTR [ecx], xmm0
	movss		DWORD_PTR [ebx], xmm0
	movss		DWORD_PTR [ebx+128], xmm2
	movss		DWORD_PTR [ebx+256], xmm1
	movss		DWORD_PTR [ebx+384], xmm3
	
	movhlps		xmm0, xmm0
	movhlps		xmm2, xmm2
	movhlps		xmm1, xmm1
	movhlps		xmm3, xmm3
	movss		DWORD_PTR [ecx+512], xmm0
	movss		DWORD_PTR [ecx+384], xmm2
	movss		DWORD_PTR [ecx+256], xmm1
	movss		DWORD_PTR [ecx+128], xmm3
	
	shufps		xmm0, xmm0, HEX(e1)
	shufps		xmm2, xmm2, HEX(e1)
	shufps		xmm1, xmm1, HEX(e1)
	shufps		xmm3, xmm3, HEX(e1)
	movss		DWORD_PTR [ebx+512], xmm0
	movss		DWORD_PTR [ebx+640], xmm2
	movss		DWORD_PTR [ebx+768], xmm1
	movss		DWORD_PTR [ebx+896], xmm3
	
	movaps		xmm0, xmm4
	shufps		xmm0, xmm0, HEX(1e)
	movaps		xmm1, xmm5
	andps		xmm0, [eax]
	
	addps		xmm4, xmm6
	addps		xmm5, xmm7
	addps		xmm6, xmm1
	addps		xmm7, xmm0
	
	movss		DWORD_PTR [ecx+960], xmm4
	movss		DWORD_PTR [ecx+832], xmm6
	movss		DWORD_PTR [ecx+704], xmm5
	movss		DWORD_PTR [ecx+576], xmm7
	movhlps		xmm0, xmm4
	movhlps		xmm1, xmm6
	movhlps		xmm2, xmm5
	movhlps		xmm3, xmm7
	movss		DWORD_PTR [ecx+448], xmm0
	movss		DWORD_PTR [ecx+320], xmm1
	movss		DWORD_PTR [ecx+192], xmm2
	movss		DWORD_PTR [ecx+64], xmm3
	
	shufps		xmm4, xmm4, HEX(e1)
	shufps		xmm6, xmm6, HEX(e1)
	shufps		xmm5, xmm5, HEX(e1)
	shufps		xmm7, xmm7, HEX(e1)
	movss		DWORD_PTR [ebx+64], xmm4
	movss		DWORD_PTR [ebx+192], xmm6
	movss		DWORD_PTR [ebx+320], xmm5
	movss		DWORD_PTR [ebx+448], xmm7
	
	shufps		xmm0, xmm0, HEX(e1)
	shufps		xmm1, xmm1, HEX(e1)
	shufps		xmm2, xmm2, HEX(e1)
	shufps		xmm3, xmm3, HEX(e1)
	movss		DWORD_PTR [ebx+576], xmm0
	movss		DWORD_PTR [ebx+704], xmm1
	movss		DWORD_PTR [ebx+832], xmm2
	movss		DWORD_PTR [ebx+960], xmm3

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

END_MODULE
