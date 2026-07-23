#include "gas_nasm_compat.h"
/*
	dct64_x86_64: SSE optimized dct64 for x86-64

	copyright 1995-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "x86_64_defs.h"
#include "mangle.h"

#ifdef IS_MSABI
/* short *out0 */
#define ARG0 r9
/* short *out1 */
#define ARG1 rdx
/* real *samples */
#define ARG2 r8
#else
/* short *out0 */
#define ARG0 rdi
/* short *out1 */
#define ARG1 rsi
/* real *samples */
#define ARG2 rdx
#endif

/*
	void dct64_x86_64(short *out0, short *out1, real *samples);
*/

	RODATA
	ALIGN32
ASM_NAME(INT123_costab_x86_64):
	DATA_LONG 1056974725
	DATA_LONG 1057056395
	DATA_LONG 1057223771
	DATA_LONG 1057485416
	DATA_LONG 1057855544
	DATA_LONG 1058356026
	DATA_LONG 1059019886
	DATA_LONG 1059897405
	DATA_LONG 1061067246
	DATA_LONG 1062657950
	DATA_LONG 1064892987
	DATA_LONG 1066774581
	DATA_LONG 1069414683
	DATA_LONG 1073984175
	DATA_LONG 1079645762
	DATA_LONG 1092815430
	DATA_LONG 1057005197
	DATA_LONG 1057342072
	DATA_LONG 1058087743
	DATA_LONG 1059427869
	DATA_LONG 1061799040
	DATA_LONG 1065862217
	DATA_LONG 1071413542
	DATA_LONG 1084439708
	DATA_LONG 1057128951
	DATA_LONG 1058664893
	DATA_LONG 1063675095
	DATA_LONG 1076102863
	DATA_LONG 1057655764
	DATA_LONG 1067924853
	DATA_LONG 1060439283
	DATA_LONG 0
	END_RODATA
	SECTION_TEXT
	ALIGN16
GLOBAL ASM_NAME(INT123_dct64_x86_64)
ASM_NAME(INT123_dct64_x86_64):
#ifdef IS_MSABI /* should save xmm6-15 */
	mov		ARG0, rcx
	sub		rsp, 168 /* stack alignment + 10 xmm registers */
	movaps		[rsp], xmm6
	movaps		[rsp+16], xmm7
	movaps		[rsp+32], xmm8
	movaps		[rsp+48], xmm9
	movaps		[rsp+64], xmm10
	movaps		[rsp+80], xmm11
	movaps		[rsp+96], xmm12
	movaps		[rsp+112], xmm13
	movaps		[rsp+128], xmm14
	movaps		144[rsp], xmm15
#endif

	lea		rcx, QWORD_PTR ASM_NAME(INT123_costab_x86_64) RIP_REL

	MOVUAPS 	xmm15, [ARG2]
	MOVUAPS 	xmm14, 16[ARG2]
	MOVUAPS 	xmm0, 112[ARG2]
	MOVUAPS 	xmm1, 96[ARG2]
	shufps 		xmm0, xmm0, HEX(1b)
	shufps 		xmm1, xmm1, HEX(1b)
	movaps 		xmm8, xmm15
	movaps		xmm9, xmm14
	addps 		xmm8, xmm0
	addps 		xmm9, xmm1
	subps 		xmm15, xmm0
	subps 		xmm14, xmm1
	
	MOVUAPS 	xmm13, 32[ARG2]
	MOVUAPS 	xmm12, 48[ARG2]
	MOVUAPS 	xmm0, 80[ARG2]
	MOVUAPS 	xmm1, 64[ARG2]
	shufps		xmm0, xmm0, HEX(1b)
	shufps		xmm1, xmm1, HEX(1b)
	movaps		xmm10, xmm13
	movaps		xmm11, xmm12
	addps		xmm10, xmm0
	addps		xmm11, xmm1
	subps		xmm13, xmm0
	subps		xmm12, xmm1
	
	movaps		xmm0, [rcx]
	movaps		xmm1, [rcx+16]
	movaps		xmm2, [rcx+32]
	movaps		xmm3, [rcx+48]
	mulps		xmm15, xmm0
	mulps		xmm14, xmm1
	mulps		xmm13, xmm2
	mulps		xmm12, xmm3
	
	movaps		xmm0, 64[rcx]
	movaps		xmm1, 80[rcx]
	
	pshufd		xmm2, xmm11, HEX(1b)
	pshufd		xmm3, xmm10, HEX(1b)
	shufps		xmm13, xmm13, HEX(1b)
	shufps		xmm12, xmm12, HEX(1b)
	movaps		xmm11, xmm8
	movaps		xmm10, xmm9
	movaps		xmm4, xmm14
	movaps		xmm5, xmm15
	subps		xmm11, xmm2
	subps		xmm10, xmm3
	subps		xmm14, xmm13
	subps		xmm15, xmm12
	addps		xmm8, xmm2
	addps		xmm9, xmm3
	addps		xmm12, xmm5
	addps		xmm13, xmm4
	mulps		xmm11, xmm0
	mulps		xmm10, xmm1
	mulps		xmm14, xmm1
	mulps		xmm15, xmm0
	
	movaps		xmm0, 96[rcx]
	
	pshufd		xmm1, xmm9, HEX(1b)
	pshufd		xmm2, xmm13, HEX(1b)
	shufps		xmm10, xmm10, HEX(1b)
	shufps		xmm14, xmm14, HEX(1b)
	movaps		xmm9, xmm8
	movaps		xmm13, xmm12
	movaps		xmm3, xmm11
	movaps		xmm4, xmm15
	subps		xmm9, xmm1
	subps		xmm13, xmm2
	subps		xmm11, xmm10
	subps		xmm15, xmm14
	addps		xmm8, xmm1
	addps		xmm12, xmm2
	addps		xmm10, xmm3
	addps		xmm14, xmm4
	mulps		xmm9, xmm0
	mulps		xmm13, xmm0
	mulps		xmm11, xmm0
	mulps		xmm15, xmm0
	
	movaps		xmm0, 112[rcx]
	movaps		xmm1, xmm0
	movlhps		xmm1, xmm1
	
	movaps		xmm2, xmm8
	movaps		xmm3, xmm9
	shufps		xmm2, xmm10, HEX(44)
	shufps		xmm9, xmm11, HEX(bb)
	shufps		xmm8, xmm10, HEX(bb)
	shufps		xmm3, xmm11, HEX(44)
	movaps		xmm4, xmm2
	movaps		xmm5, xmm3
	subps		xmm2, xmm8
	subps		xmm3, xmm9
	addps		xmm8, xmm4
	addps		xmm9, xmm5
	mulps		xmm2, xmm1
	mulps		xmm3, xmm1
	movaps		xmm10, xmm8
	movaps		xmm11, xmm9
	shufps		xmm8, xmm2, HEX(14)
	shufps		xmm10, xmm2, HEX(be)
	shufps		xmm9, xmm3, HEX(14)
	shufps		xmm11, xmm3, HEX(be)
	
	movaps		xmm2, xmm12
	movaps		xmm3, xmm13
	shufps		xmm2, xmm14, HEX(44)
	shufps		xmm13, xmm15, HEX(bb)
	shufps		xmm12, xmm14, HEX(bb)
	shufps		xmm3, xmm15, HEX(44)
	movaps		xmm4, xmm2
	movaps		xmm5, xmm3
	subps		xmm2, xmm12
	subps		xmm3, xmm13
	addps		xmm12, xmm4
	addps		xmm13, xmm5
	mulps		xmm2, xmm1
	mulps		xmm3, xmm1
	movaps		xmm14, xmm12
	movaps		xmm15, xmm13
	shufps		xmm12, xmm2, HEX(14)
	shufps		xmm14, xmm2, HEX(be)
	shufps		xmm13, xmm3, HEX(14)
	shufps		xmm15, xmm3, HEX(be)
	
	shufps		xmm0, xmm0, HEX(aa)
	pcmpeqd		xmm1, xmm1
	pslld		xmm1, 31
	psllq		xmm1, 32
	xorps		xmm0, xmm1
	
	movaps		xmm1, xmm8
	movaps		xmm2, xmm10
	unpcklps	xmm8, xmm9
	unpckhps	xmm1, xmm9
	unpcklps	xmm10, xmm11
	unpckhps	xmm2, xmm11
	movaps		xmm3, xmm8
	movaps		xmm4, xmm10
	unpcklps	xmm8, xmm1
	unpckhps	xmm3, xmm1
	unpcklps	xmm10, xmm2
	unpckhps	xmm4, xmm2
	movaps		xmm1, xmm8
	movaps		xmm2, xmm10
	subps		xmm1, xmm3
	subps		xmm2, xmm4
	addps		xmm8, xmm3
	addps		xmm10, xmm4
	mulps		xmm1, xmm0
	mulps		xmm2, xmm0
	movaps		xmm9, xmm8
	movaps		xmm11, xmm10
	unpcklps	xmm8, xmm1
	unpckhps	xmm9, xmm1
	unpcklps	xmm10, xmm2
	unpckhps	xmm11, xmm2
	
	movaps		xmm1, xmm12
	movaps		xmm2, xmm14
	unpcklps	xmm12, xmm13
	unpckhps	xmm1, xmm13
	unpcklps	xmm14, xmm15
	unpckhps	xmm2, xmm15
	movaps		xmm3, xmm12
	movaps		xmm4, xmm14
	unpcklps	xmm12, xmm1
	unpckhps	xmm3, xmm1
	unpcklps	xmm14, xmm2
	unpckhps	xmm4, xmm2
	movaps		xmm1, xmm12
	movaps		xmm2, xmm14
	subps		xmm1, xmm3
	subps		xmm2, xmm4
	addps		xmm12, xmm3
	addps		xmm14, xmm4
	mulps		xmm1, xmm0
	mulps		xmm2, xmm0
	movaps		xmm13, xmm12
	movaps		xmm15, xmm14
	unpcklps	xmm12, xmm1
	unpckhps	xmm13, xmm1
	unpcklps	xmm14, xmm2
	unpckhps	xmm15, xmm2
	
	
	xorps		xmm0, xmm0
	xorps		xmm1, xmm1
	shufpd		xmm0, xmm8, HEX(2)
	shufpd		xmm1, xmm9, HEX(2)
	psrlq		xmm0, 32
	psrlq		xmm1, 32
	addps		xmm8, xmm0
	addps		xmm9, xmm1
	
	xorps		xmm0, xmm0
	xorps		xmm1, xmm1
	shufpd		xmm0, xmm10, HEX(2)
	shufpd		xmm1, xmm11, HEX(2)
	psrlq		xmm0, 32
	psrlq		xmm1, 32
	addps		xmm10, xmm0
	addps		xmm11, xmm1
	
	xorps		xmm0, xmm0
	xorps		xmm1, xmm1
	shufpd		xmm0, xmm12, HEX(2)
	shufpd		xmm1, xmm13, HEX(2)
	psrlq		xmm0, 32
	psrlq		xmm1, 32
	addps		xmm12, xmm0
	addps		xmm13, xmm1
	
	xorps		xmm0, xmm0
	xorps		xmm1, xmm1
	shufpd		xmm0, xmm14, HEX(2)
	shufpd		xmm1, xmm15, HEX(2)
	psrlq		xmm0, 32
	psrlq		xmm1, 32
	addps		xmm14, xmm0
	addps		xmm15, xmm1
	
	pshufd		xmm0, xmm9, HEX(78)
	pshufd		xmm1, xmm11, HEX(78)
	pshufd		xmm2, xmm13, HEX(78)
	pshufd		xmm3, xmm15, HEX(78)
	psrldq		xmm0, 4
	psrldq		xmm1, 4
	psrldq		xmm2, 4
	psrldq		xmm3, 4
	addps		xmm9, xmm0
	addps		xmm11, xmm1
	addps		xmm13, xmm2
	addps		xmm15, xmm3
	
	pshufd		xmm0, xmm10, HEX(78)
	pshufd		xmm1, xmm14, HEX(78)
	psrldq		xmm0, 4
	psrldq		xmm1, 4
	addps		xmm10, xmm11
	addps		xmm14, xmm15
	addps		xmm11, xmm0
	addps		xmm15, xmm1
	
	cvtps2dq	xmm8, xmm8
	cvtps2dq	xmm9, xmm9
	cvtps2dq	xmm10, xmm10
	cvtps2dq	xmm11, xmm11
	packssdw	xmm8, xmm10
	packssdw	xmm9, xmm11
	
	movd		eax, xmm8
	movd		ecx, xmm9
	mov		word ptr 512[ARG0], ax
	mov		word ptr 384[ARG0], cx
	shr		eax, 16
	shr		ecx, 16
	mov		word ptr [ARG0], ax
	mov		word ptr [ARG1], ax
	mov		word ptr 128[ARG1], cx
	
	movhlps		xmm0, xmm8
	movhlps		xmm1, xmm9
	movd		eax, xmm0
	movd		ecx, xmm1
	mov		word ptr 448[ARG0], ax
	mov		word ptr 320[ARG0], cx
	shr		eax, 16
	shr		ecx, 16
	mov		word ptr 64[ARG1], ax
	mov		word ptr 192[ARG1], cx
	
	pshuflw		xmm2, xmm8, HEX(ee)
	pshuflw		xmm3, xmm9, HEX(ee)
	movd		eax, xmm2
	movd		ecx, xmm3
	mov		word ptr 256[ARG0], ax
	mov		word ptr 128[ARG0], cx
	shr		eax, 16
	shr		ecx, 16
	mov		word ptr 256[ARG1], ax
	mov		word ptr 384[ARG1], cx
	
	pshuflw		xmm0, xmm0, HEX(ee)
	pshuflw		xmm1, xmm1, HEX(ee)
	movd		eax, xmm0
	movd		ecx, xmm1
	mov		word ptr 192[ARG0], ax
	mov		word ptr 64[ARG0], cx
	shr		eax, 16
	shr		ecx, 16
	mov		word ptr 320[ARG1], ax
	mov		word ptr 448[ARG1], cx
	
	movaps		xmm0, xmm12
	movaps		xmm1, xmm13
	movaps		xmm2, xmm14
	movaps		xmm3, xmm15
	shufps		xmm0, xmm0, HEX(1e)
	pslldq		xmm0, 4
	psrldq		xmm0, 4
	addps		xmm12, xmm2
	addps		xmm13, xmm3
	addps		xmm14, xmm1
	addps		xmm15, xmm0
	
	cvtps2dq	xmm12, xmm12
	cvtps2dq	xmm13, xmm13
	cvtps2dq	xmm14, xmm14
	cvtps2dq	xmm15, xmm15
	packssdw	xmm12, xmm13
	packssdw	xmm14, xmm15
	
	movd		eax, xmm12
	movd		ecx, xmm14
	mov		word ptr 480[ARG0], ax
	mov		word ptr 416[ARG0], cx
	shr		eax, 16
	shr		ecx, 16
	mov		word ptr 32[ARG1], ax
	mov		word ptr 96[ARG1], cx
	
	pshuflw		xmm0, xmm12, HEX(ee)
	pshuflw		xmm1, xmm14, HEX(ee)
	movd		eax, xmm0
	movd		ecx, xmm1
	mov		word ptr 224[ARG0], ax
	mov		word ptr 160[ARG0], cx
	shr		eax, 16
	shr		ecx, 16
	mov		word ptr 288[ARG1], ax
	mov		word ptr 352[ARG1], cx
	
	movhlps		xmm0, xmm12
	movhlps		xmm1, xmm14
	movd		eax, xmm0
	movd		ecx, xmm1
	mov		word ptr 352[ARG0], ax
	mov		word ptr 288[ARG0], cx
	shr		eax, 16
	shr		ecx, 16
	mov		word ptr 160[ARG1], ax
	mov		word ptr 224[ARG1], cx
	
	pshuflw		xmm0, xmm0, HEX(ee)
	pshuflw		xmm1, xmm1, HEX(ee)
	movd		eax, xmm0
	movd		ecx, xmm1
	mov		word ptr 96[ARG0], ax
	mov		word ptr 32[ARG0], cx
	shr		eax, 16
	shr		ecx, 16
	mov		word ptr 416[ARG1], ax
	mov		word ptr 480[ARG1], cx
	
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
	add		rsp, 168
#endif
	ret

NONEXEC_STACK

END_MODULE
