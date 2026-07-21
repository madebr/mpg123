#include "gas_nasm_compat.h"
/*
	dct64_x86_64_float: SSE optimized dct64 for x86-64 (float output version)

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
/* real *out0 */
#define ARG0 rdi
/* real *out1 */
#define ARG1 rsi
/* real *samples */
#define ARG2 rdx
#endif

/*
	void dct64_real_x86_64(real *out0, real *out1, real *samples);
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
	SECTION_TEXT
	ALIGN16
GLOBAL ASM_NAME(INT123_dct64_real_x86_64)
ASM_NAME(INT123_dct64_real_x86_64):
#ifdef IS_MSABI /* should save xmm6-15 */
	mov		ARG0, rcx
	sub		rsp, 168 /* stack alignment + 10 xmm registers */
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
#endif

	lea		rcx, ASM_NAME(INT123_costab_x86_64) RIP_REL

	MOVUAPS 	xmm15, [ARG2]
	MOVUAPS 	xmm14, 16[ARG2]
	MOVUAPS 	xmm0, 112[ARG2]
	MOVUAPS 	xmm1, 96[ARG2]
	shufps 		xmm0, xmm0, 0x1b
	shufps 		xmm1, xmm1, 0x1b
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
	shufps		xmm0, xmm0, 0x1b
	shufps		xmm1, xmm1, 0x1b
	movaps		xmm10, xmm13
	movaps		xmm11, xmm12
	addps		xmm10, xmm0
	addps		xmm11, xmm1
	subps		xmm13, xmm0
	subps		xmm12, xmm1
	
	movaps		xmm0, [rcx]
	movaps		xmm1, 16[rcx]
	movaps		xmm2, 32[rcx]
	movaps		xmm3, 48[rcx]
	mulps		xmm15, xmm0
	mulps		xmm14, xmm1
	mulps		xmm13, xmm2
	mulps		xmm12, xmm3
	
	movaps		xmm0, 64[rcx]
	movaps		xmm1, 80[rcx]
	
	pshufd		xmm2, xmm11, 0x1b
	pshufd		xmm3, xmm10, 0x1b
	shufps		xmm13, xmm13, 0x1b
	shufps		xmm12, xmm12, 0x1b
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
	
	pshufd		xmm1, xmm9, 0x1b
	pshufd		xmm2, xmm13, 0x1b
	shufps		xmm10, xmm10, 0x1b
	shufps		xmm14, xmm14, 0x1b
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
	shufps		xmm2, xmm10, 0x44
	shufps		xmm9, xmm11, 0xbb
	shufps		xmm8, xmm10, 0xbb
	shufps		xmm3, xmm11, 0x44
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
	shufps		xmm8, xmm2, 0x14
	shufps		xmm10, xmm2, 0xbe
	shufps		xmm9, xmm3, 0x14
	shufps		xmm11, xmm3, 0xbe
	
	movaps		xmm2, xmm12
	movaps		xmm3, xmm13
	shufps		xmm2, xmm14, 0x44
	shufps		xmm13, xmm15, 0xbb
	shufps		xmm12, xmm14, 0xbb
	shufps		xmm3, xmm15, 0x44
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
	shufps		xmm12, xmm2, 0x14
	shufps		xmm14, xmm2, 0xbe
	shufps		xmm13, xmm3, 0x14
	shufps		xmm15, xmm3, 0xbe
	
	shufps		xmm0, xmm0, 0xaa
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
	shufpd		xmm0, xmm8, 0x2
	shufpd		xmm1, xmm9, 0x2
	psrlq		xmm0, 32
	psrlq		xmm1, 32
	addps		xmm8, xmm0
	addps		xmm9, xmm1
	
	xorps		xmm0, xmm0
	xorps		xmm1, xmm1
	shufpd		xmm0, xmm10, 0x2
	shufpd		xmm1, xmm11, 0x2
	psrlq		xmm0, 32
	psrlq		xmm1, 32
	addps		xmm10, xmm0
	addps		xmm11, xmm1
	
	xorps		xmm0, xmm0
	xorps		xmm1, xmm1
	shufpd		xmm0, xmm12, 0x2
	shufpd		xmm1, xmm13, 0x2
	psrlq		xmm0, 32
	psrlq		xmm1, 32
	addps		xmm12, xmm0
	addps		xmm13, xmm1
	
	xorps		xmm0, xmm0
	xorps		xmm1, xmm1
	shufpd		xmm0, xmm14, 0x2
	shufpd		xmm1, xmm15, 0x2
	psrlq		xmm0, 32
	psrlq		xmm1, 32
	addps		xmm14, xmm0
	addps		xmm15, xmm1
	
	pshufd		xmm0, xmm9, 0x78
	pshufd		xmm1, xmm11, 0x78
	pshufd		xmm2, xmm13, 0x78
	pshufd		xmm3, xmm15, 0x78
	psrldq		xmm0, 4
	psrldq		xmm1, 4
	psrldq		xmm2, 4
	psrldq		xmm3, 4
	addps		xmm9, xmm0
	addps		xmm11, xmm1
	addps		xmm13, xmm2
	addps		xmm15, xmm3
	
	pshufd		xmm0, xmm10, 0x78
	pshufd		xmm1, xmm14, 0x78
	psrldq		xmm0, 4
	psrldq		xmm1, 4
	addps		xmm10, xmm11
	addps		xmm14, xmm15
	addps		xmm11, xmm0
	addps		xmm15, xmm1
	
	
	movss		1024[ARG0], xmm8
	movss		896[ARG0], xmm10
	movss		768[ARG0], xmm9
	movss		640[ARG0], xmm11
	movhlps		xmm0, xmm8
	movhlps		xmm1, xmm10
	movhlps		xmm2, xmm9
	movhlps		xmm3, xmm11
	movss		512[ARG0], xmm0
	movss		384[ARG0], xmm1
	movss		256[ARG0], xmm2
	movss		128[ARG0], xmm3
	
	pshuflw		xmm4, xmm8, 0xee
	pshuflw		xmm5, xmm10, 0xee
	pshuflw		xmm6, xmm9, 0xee
	pshuflw		xmm7, xmm11, 0xee
	movss		[ARG0], xmm4
	movss		[ARG1], xmm4
	movss		128[ARG1], xmm5
	movss		256[ARG1], xmm6
	movss		384[ARG1], xmm7
	
	pshuflw		xmm0, xmm0, 0xee
	pshuflw		xmm1, xmm1, 0xee
	pshuflw		xmm2, xmm2, 0xee
	pshuflw		xmm3, xmm3, 0xee
	movss		512[ARG1], xmm0
	movss		640[ARG1], xmm1
	movss		768[ARG1], xmm2
	movss		896[ARG1], xmm3
	
	pshufd		xmm0, xmm12, 0x78
	movaps		xmm1, xmm13
	psrldq		xmm0, 4
	
	addps		xmm12, xmm14
	addps		xmm13, xmm15
	addps		xmm14, xmm1
	addps		xmm15, xmm0
	
	movss		960[ARG0], xmm12
	movss		832[ARG0], xmm14
	movss		704[ARG0], xmm13
	movss		576[ARG0], xmm15
	movhlps		xmm0, xmm12
	movhlps		xmm1, xmm14
	movhlps		xmm2, xmm13
	movhlps		xmm3, xmm15
	movss		448[ARG0], xmm0
	movss		320[ARG0], xmm1
	movss		192[ARG0], xmm2
	movss		64[ARG0], xmm3
	
	pshuflw		xmm4, xmm12, 0xee
	pshuflw		xmm5, xmm14, 0xee
	pshuflw		xmm6, xmm13, 0xee
	pshuflw		xmm7, xmm15, 0xee
	movss		64[ARG1], xmm4
	movss		192[ARG1], xmm5
	movss		320[ARG1], xmm6
	movss		448[ARG1], xmm7
	
	pshuflw		xmm0, xmm0, 0xee
	pshuflw		xmm1, xmm1, 0xee
	pshuflw		xmm2, xmm2, 0xee
	pshuflw		xmm3, xmm3, 0xee
	movss		576[ARG1], xmm0
	movss		704[ARG1], xmm1
	movss		832[ARG1], xmm2
	movss		960[ARG1], xmm3
	
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
