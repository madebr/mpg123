#include "gas_nasm_compat.h"
/*
	dct64_sse_float: SSE optimized dct64 (float output version)

	copyright 1995-2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "mangle.h"

#define ARG(n) (8+n*4)(ebp)
#define TEMP(n) (4+n*16)(esp)
#define TEMP_BYTE(n) (4+n)(esp)

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
mask:
	DATA_LONG	-1
	DATA_LONG	-1
	DATA_LONG	-1
	DATA_LONG	0
	
	.text
	ALIGN16
.globl ASM_NAME(INT123_dct64_real_sse)
ASM_NAME(INT123_dct64_real_sse):
	push		ebp
	mov		ebp, esp
	
	and		esp, -16 /* align the stack at 16 bytes */
	sub		esp, 128 /* reserve space for temporal store */
	push		ebx

	GET_GOT
	
	mov		eax, dword ptr ARG[2]
	
	MOVUAPS 	xmm7, [eax]
	MOVUAPS 	xmm6, 16[eax]
	MOVUAPS 	xmm0, 112[eax]
	MOVUAPS 	xmm1, 96[eax]
	shufps 		xmm0, xmm0, 0x1b
	shufps 		xmm1, xmm1, 0x1b
	movaps 		xmm4, xmm7
	movaps		xmm5, xmm6
	addps 		xmm4, xmm0
	addps 		xmm5, xmm1
	subps 		xmm7, xmm0
	subps 		xmm6, xmm1
	movaps		TEMP[0], xmm4
	movaps		TEMP[1], xmm5
	
	MOVUAPS 	xmm2, 32[eax]
	MOVUAPS 	xmm3, 48[eax]
	MOVUAPS 	xmm0, 80[eax]
	MOVUAPS 	xmm1, 64[eax]
	shufps 		xmm0, xmm0, 0x1b
	shufps 		xmm1, xmm1, 0x1b
	movaps 		xmm5, xmm2
	movaps		xmm4, xmm3
	addps 		xmm2, xmm0
	addps 		xmm3, xmm1
	subps 		xmm5, xmm0
	subps 		xmm4, xmm1
	
#ifdef PIC_GLOBAL_PTR
	mov			eax, GLOBAL_VAR_PTR[INT123_costab_mmxsse]
#else
	lea			eax, GLOBAL_VAR[INT123_costab_mmxsse]
#endif
	mulps		xmm7, [eax]
	mulps		xmm6, 16[eax]
	mulps		xmm5, 32[eax]
	mulps		xmm4, 48[eax]
	
	shufps		xmm2, xmm2, 0x1b
	shufps		xmm3, xmm3, 0x1b
	shufps		xmm4, xmm4, 0x1b
	shufps		xmm5, xmm5, 0x1b
	movaps		xmm0, TEMP[0]
	movaps		xmm1, TEMP[1]
	subps		xmm0, xmm3
	subps		xmm1, xmm2
	addps		xmm3, TEMP[0]
	addps		xmm2, TEMP[1]
	movaps		TEMP[0], xmm3
	movaps		TEMP[1], xmm2
	movaps		xmm2, xmm6
	movaps		xmm3, xmm7
	subps		xmm6, xmm5
	subps		xmm7, xmm4
	addps		xmm4, xmm3
	addps		xmm5, xmm2
	mulps		xmm0, 64[eax]
	mulps		xmm1, 80[eax]
	mulps		xmm6, 80[eax]
	mulps		xmm7, 64[eax]
	
	movaps		xmm2, TEMP[0]
	movaps		xmm3, TEMP[1]
	shufps		xmm3, xmm3, 0x1b
	shufps		xmm5, xmm5, 0x1b
	shufps		xmm1, xmm1, 0x1b
	shufps		xmm6, xmm6, 0x1b
	movaps		TEMP[1], xmm0
	subps		xmm2, xmm3
	subps		xmm0, xmm1
	addps		xmm3, TEMP[0]
	addps		xmm1, TEMP[1]
	movaps		TEMP[0], xmm3
	movaps		TEMP[2], xmm1
	movaps		xmm1, xmm5
	movaps		xmm5, xmm4
	movaps		xmm3, xmm7
	subps		xmm5, xmm1
	subps		xmm7, xmm6
	addps		xmm4, xmm1
	addps		xmm6, xmm3
	mulps		xmm2, 96[eax]
	mulps		xmm0, 96[eax]
	mulps		xmm5, 96[eax]
	mulps		xmm7, 96[eax]
	movaps		TEMP[1], xmm2
	movaps		TEMP[3], xmm0
	
	movaps		xmm2, xmm4
	movaps		xmm3, xmm5
	shufps		xmm2, xmm6, 0x44
	shufps		xmm5, xmm7, 0xbb
	shufps		xmm4, xmm6, 0xbb
	shufps		xmm3, xmm7, 0x44
	movaps		xmm6, xmm2
	movaps		xmm7, xmm3
	subps		xmm2, xmm4
	subps		xmm3, xmm5
	addps		xmm4, xmm6
	addps		xmm5, xmm7
	movaps		xmm0, 112[eax]
	movlhps		xmm0, xmm0
	mulps		xmm2, xmm0
	mulps		xmm3, xmm0
	movaps		TEMP[4], xmm0
	movaps		xmm6, xmm4
	movaps		xmm7, xmm5
	shufps		xmm4, xmm2, 0x14
	shufps		xmm6, xmm2, 0xbe
	shufps		xmm5, xmm3, 0x14
	shufps		xmm7, xmm3, 0xbe
	movaps		TEMP[5], xmm5
	movaps		TEMP[7], xmm7
	
	movaps		xmm0, TEMP[0]
	movaps		xmm1, TEMP[1]
	movaps		xmm2, xmm0
	movaps		xmm3, xmm1
	shufps		xmm2, TEMP[2], 0x44
	shufps		xmm1, TEMP[3], 0xbb
	shufps		xmm0, TEMP[2], 0xbb
	shufps		xmm3, TEMP[3], 0x44
	movaps		xmm5, xmm2
	movaps		xmm7, xmm3
	subps		xmm2, xmm0
	subps		xmm3, xmm1
	addps		xmm0, xmm5
	addps		xmm1, xmm7
	mulps		xmm2, TEMP[4]
	mulps		xmm3, TEMP[4]
	movaps		xmm5, xmm0
	movaps		xmm7, xmm1
	shufps		xmm0, xmm2, 0x14
	shufps		xmm5, xmm2, 0xbe
	shufps		xmm1, xmm3, 0x14
	shufps		xmm7, xmm3, 0xbe
	
	movaps		TEMP[0], xmm0
	movaps		TEMP[1], xmm1
	movaps		TEMP[2], xmm5
	movaps		TEMP[3], xmm7
	
	movss		xmm5, 120[eax]
	shufps		xmm5, xmm5, 0x00
	xorps		xmm5, LOCAL_VAR[pnpn]
	
	movaps		xmm0, xmm4
	movaps		xmm1, xmm6
	unpcklps	xmm4, TEMP[5]
	unpckhps	xmm0, TEMP[5]
	unpcklps	xmm6, TEMP[7]
	unpckhps	xmm1, TEMP[7]
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
	movaps		TEMP[5], xmm5
	movaps		xmm5, xmm4
	movaps		xmm7, xmm6
	unpcklps	xmm4, xmm0
	unpckhps	xmm5, xmm0
	unpcklps	xmm6, xmm1
	unpckhps	xmm7, xmm1
	
	movaps		xmm0, TEMP[0]
	movaps		xmm2, TEMP[2]
	movaps		TEMP[4], xmm4
	movaps		TEMP[6], xmm6
	
	movaps		xmm4, xmm0
	movaps		xmm6, xmm2
	unpcklps	xmm0, TEMP[1]
	unpckhps	xmm4, TEMP[1]
	unpcklps	xmm2, TEMP[3]
	unpckhps	xmm6, TEMP[3]
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
	mulps		xmm4, TEMP[5]
	mulps		xmm6, TEMP[5]
	movaps		xmm1, xmm0
	movaps		xmm3, xmm2
	unpcklps	xmm0, xmm4
	unpckhps	xmm1, xmm4
	unpcklps	xmm2, xmm6
	unpckhps	xmm3, xmm6
	
	movaps		TEMP[0], xmm0
	movaps		TEMP[1], xmm1
	movaps		TEMP[2], xmm2
	movaps		TEMP[3], xmm3
	movaps		TEMP[5], xmm5
	movaps		TEMP[7], xmm7
	
	movss		xmm0, TEMP_BYTE[12]
	movss		xmm1, TEMP_BYTE[28]
	movss		xmm2, TEMP_BYTE[44]
	movss		xmm3, TEMP_BYTE[60]
	addss		xmm0, TEMP_BYTE[8]
	addss		xmm1, TEMP_BYTE[24]
	addss		xmm2, TEMP_BYTE[40]
	addss		xmm3, TEMP_BYTE[56]
	movss		TEMP_BYTE[8], xmm0
	movss		TEMP_BYTE[24], xmm1
	movss		TEMP_BYTE[40], xmm2
	movss		TEMP_BYTE[56], xmm3
	movss		xmm0, TEMP_BYTE[76]
	movss		xmm1, TEMP_BYTE[92]
	movss		xmm2, TEMP_BYTE[108]
	movss		xmm3, TEMP_BYTE[124]
	addss		xmm0, TEMP_BYTE[72]
	addss		xmm1, TEMP_BYTE[88]
	addss		xmm2, TEMP_BYTE[104]
	addss		xmm3, TEMP_BYTE[120]
	movss		TEMP_BYTE[72], xmm0
	movss		TEMP_BYTE[88], xmm1
	movss		TEMP_BYTE[104], xmm2
	movss		TEMP_BYTE[120], xmm3
	
	movaps		xmm1, TEMP_BYTE[16]
	movaps		xmm3, TEMP_BYTE[48]
	movaps		xmm5, TEMP_BYTE[80]
	movaps		xmm7, TEMP_BYTE[112]
	movaps		xmm0, xmm1
	movaps		xmm2, xmm3
	movaps		xmm4, xmm5
	movaps		xmm6, xmm7
	shufps		xmm0, xmm0, 0x1e
	shufps		xmm2, xmm2, 0x1e
	shufps		xmm4, xmm4, 0x1e
	shufps		xmm6, xmm6, 0x1e
	lea			eax, LOCAL_VAR[mask]
	andps		xmm0, [eax]
	andps		xmm2, [eax]
	andps		xmm4, [eax]
	andps		xmm6, [eax]
	addps		xmm1, xmm0
	addps		xmm3, xmm2
	addps		xmm5, xmm4
	addps		xmm7, xmm6
	
	movaps		xmm2, TEMP_BYTE[32]
	movaps		xmm6, TEMP_BYTE[96]
	movaps		xmm0, xmm2
	movaps		xmm4, xmm6
	shufps		xmm0, xmm0, 0x1e
	shufps		xmm4, xmm4, 0x1e
	andps		xmm0, [eax]
	andps		xmm4, [eax]
	addps		xmm2, xmm3
	addps		xmm3, xmm0
	addps		xmm6, xmm7
	addps		xmm7, xmm4
	
	movaps		xmm0, TEMP_BYTE[0]
	movaps		xmm4, TEMP_BYTE[64]
	
	mov		ecx, dword ptr ARG[0]
	mov		ebx, dword ptr ARG[1]
	
	movss		1024[ecx], xmm0
	movss		896[ecx], xmm2
	movss		768[ecx], xmm1
	movss		640[ecx], xmm3
	
	shufps		xmm0, xmm0, 0xe1
	shufps		xmm2, xmm2, 0xe1
	shufps		xmm1, xmm1, 0xe1
	shufps		xmm3, xmm3, 0xe1
	movss		[ecx], xmm0
	movss		[ebx], xmm0
	movss		128[ebx], xmm2
	movss		256[ebx], xmm1
	movss		384[ebx], xmm3
	
	movhlps		xmm0, xmm0
	movhlps		xmm2, xmm2
	movhlps		xmm1, xmm1
	movhlps		xmm3, xmm3
	movss		512[ecx], xmm0
	movss		384[ecx], xmm2
	movss		256[ecx], xmm1
	movss		128[ecx], xmm3
	
	shufps		xmm0, xmm0, 0xe1
	shufps		xmm2, xmm2, 0xe1
	shufps		xmm1, xmm1, 0xe1
	shufps		xmm3, xmm3, 0xe1
	movss		512[ebx], xmm0
	movss		640[ebx], xmm2
	movss		768[ebx], xmm1
	movss		896[ebx], xmm3
	
	movaps		xmm0, xmm4
	shufps		xmm0, xmm0, 0x1e
	movaps		xmm1, xmm5
	andps		xmm0, [eax]
	
	addps		xmm4, xmm6
	addps		xmm5, xmm7
	addps		xmm6, xmm1
	addps		xmm7, xmm0
	
	movss		960[ecx], xmm4
	movss		832[ecx], xmm6
	movss		704[ecx], xmm5
	movss		576[ecx], xmm7
	movhlps		xmm0, xmm4
	movhlps		xmm1, xmm6
	movhlps		xmm2, xmm5
	movhlps		xmm3, xmm7
	movss		448[ecx], xmm0
	movss		320[ecx], xmm1
	movss		192[ecx], xmm2
	movss		64[ecx], xmm3
	
	shufps		xmm4, xmm4, 0xe1
	shufps		xmm6, xmm6, 0xe1
	shufps		xmm5, xmm5, 0xe1
	shufps		xmm7, xmm7, 0xe1
	movss		64[ebx], xmm4
	movss		192[ebx], xmm6
	movss		320[ebx], xmm5
	movss		448[ebx], xmm7
	
	shufps		xmm0, xmm0, 0xe1
	shufps		xmm1, xmm1, 0xe1
	shufps		xmm2, xmm2, 0xe1
	shufps		xmm3, xmm3, 0xe1
	movss		576[ebx], xmm0
	movss		704[ebx], xmm1
	movss		832[ebx], xmm2
	movss		960[ebx], xmm3

	pop		ebx
	mov		esp, ebp
	pop		ebp
	ret

#if defined(PIC) && defined(__APPLE__)
	.section	__IMPORT,__pointers,non_lazy_symbol_pointers
L_INT123_costab_mmxsse:
	.indirect_symbol	ASM_NAME(INT123_costab_mmxsse)
	DATA_LONG	0
#endif

NONEXEC_STACK
