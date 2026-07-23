#include "gas_nasm_compat.h"
/*
	dct36_sse: AVX optimized dct64 for x86-64

	copyright 1995-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "x86_64_defs.h"
#include "mangle.h"

#define samples rdx
#define costab rcx
#define out0 rdi
#define out1 rsi

/*
	void dct64_avx(short *out0, short *out1, real *samples);
*/

	RODATA
	ALIGN32
costab_avx:
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
GLOBAL ASM_NAME(INT123_dct64_avx)
ASM_NAME(INT123_dct64_avx):
#ifdef IS_MSABI
	push		rbp
	mov			rbp, rsp
	sub			rsp, 112
	movaps		[rsp], xmm6
	movaps		[rsp+16], xmm7
	movaps		[rsp+32], xmm8
	movaps		[rsp+48], xmm9
	movaps		[rsp+64], xmm10
	movaps		[rsp+80], xmm11
	movaps		[rsp+96], xmm12
	push		rdi
	push		rsi
	mov			rdi, rcx
	mov			rsi, rdx
	mov			rdx, r8
#endif
	lea		costab, QWORD_PTR costab_avx RIP_REL

	vmovups		ymm0, [samples]			# input[0,1,2,3,4,5,6,7]
	vmovups		ymm1, [samples+32]			# input[8,9,10,11,12,13,14,15]
	vperm2f128	ymm2, ymm2, [samples+64], HEX(23)
	vperm2f128	ymm3, ymm3, [samples+96], HEX(23)
	vshufps		ymm2, ymm2, ymm2, HEX(1b)	# input[23,22,21,20,19,18,17,16]
	vshufps		ymm3, ymm3, ymm3, HEX(1b)	# input[31,30,29,28,27,26,25,24]
	vsubps		ymm6, ymm1, ymm2
	vsubps		ymm7, ymm0, ymm3
	vaddps		ymm4, ymm3, ymm0			# bufs[0,1,2,3,4,5,6,7]
	vaddps		ymm5, ymm2, ymm1			# bufs[8,9,10,11,12,13,14,15]
	vmulps		ymm7, ymm7, [costab]		# bufs[31,30,29,28,27,26,25,24] cos64[0,1,2,3,4,5,6,7]
	vmulps		ymm6, ymm6, [costab+32]	# bufs[23,22,21,20,19,18,17,16] cos64[8,9,10,11,12,13,14,15]
	
	vmovaps		ymm8, [costab+64]			# cos32[0,1,2,3,4,5,6,7]
	
	vshufps		ymm5, ymm5, ymm5, HEX(1b)
	vshufps		ymm6, ymm6, ymm6, HEX(1b)
	vperm2f128	ymm5, ymm5, ymm5, HEX(01)	# bufs[15,14,13,12,11,10,9,8]
	vperm2f128	ymm6, ymm6, ymm6, HEX(01)	# bufs[16,17,18,19,20,21,22,23]
	vsubps		ymm1, ymm4, ymm5
	vsubps		ymm3, ymm7, ymm6
	vaddps		ymm0, ymm4, ymm5			# bufs[32,33,34,35,36,37,38,39]
	vaddps		ymm2, ymm7, ymm6			# bufs[48,49,50,51,52,53,54,55]
	vmulps		ymm1, ymm8, ymm1			# bufs[47,46,45,44,43,42,41,40]
	vmulps		ymm3, ymm8, ymm3			# bufs[63,62,61,60,59,58,57,56]
	
	vmovaps		ymm8, [costab+96]			# cos16[0,1,2,3]:cos8[0,1]:cos4[0]:-
	vperm2f128	ymm9, ymm8, ymm8, HEX(00)	# cos16[0,1,2,3,0,1,2,3]
	
	vperm2f128	ymm4, ymm0, ymm1, HEX(20)	# bufs[32,33,34,35,47,46,45,44]
	vperm2f128	ymm5, ymm0, ymm1, HEX(31)
	vshufps		ymm5, ymm5, ymm5, HEX(1b)	# bufs[39,38,37,36,40,41,42,43]
	vperm2f128	ymm6, ymm2, ymm3, HEX(20)	# bufs[48,49,50,51,63,62,61,60]
	vperm2f128	ymm7, ymm2, ymm3, HEX(31)
	vshufps		ymm7, ymm7, ymm7, HEX(1b)	# bufs[55,54,53,52,56,57,58,59]
	vsubps		ymm1, ymm4, ymm5
	vsubps		ymm3, ymm6, ymm7
	vaddps		ymm0, ymm4, ymm5			# bufs[0,1,2,3,8,9,10,11]
	vaddps		ymm2, ymm6, ymm7			# bufs[16,17,18,19,24,25,26,27]
	vmulps		ymm1, ymm9, ymm1			# bufs[7,6,5,4,15,14,13,12]
	vmulps		ymm3, ymm9, ymm3			# bufs[23,22,21,20,31,30,29,28]
	
	vperm2f128	ymm8, ymm8, ymm8, HEX(11)	# cos8[0,1]:cos4[0]:-:cos8[0,1]:cos4[0]:-
	vmovddup	ymm9, ymm8				# cos8[0,1,0,1,0,1,0,1]
	
	vunpcklps	ymm4, ymm0, ymm1			# bufs[0,7,1,6,8,15,9,14]
	vunpckhps	ymm5, ymm0, ymm1			# bufs[2,5,3,4,10,13,11,12]
	vunpcklps	ymm6, ymm2, ymm3			# bufs[16,23,17,22,24,31,25,30]
	vunpckhps	ymm7, ymm2, ymm3			# bufs[18,21,19,20,26,29,27,28]
	vshufps		ymm4, ymm4, ymm4, HEX(d8)	# bufs[0,1,7,6,8,9,15,14]
	vshufps		ymm5, ymm5, ymm5, HEX(72)	# bufs[3,2,4,5,11,10,12,13]
	vshufps		ymm6, ymm6, ymm6, HEX(d8)	# bufs[16,17,23,22,24,25,31,30]
	vshufps		ymm7, ymm7, ymm7, HEX(72)	# bufs[19,18,20,21,27,26,28,29]
	vsubps		ymm1, ymm4, ymm5
	vsubps		ymm3, ymm6, ymm7
	vaddps		ymm0, ymm4, ymm5			# bufs[32,33,36,37,40,41,44,45]
	vaddps		ymm2, ymm6, ymm7			# bufs[48,49,52,53,56,57,60,61]
	vmulps		ymm1, ymm9, ymm1			# bufs[35,34,39,38,43,42,47,46]
	vmulps		ymm3, ymm9, ymm3			# bufs[51,50,55,54,59,58,63,62]
	
	vpermilps	ymm8, ymm8, HEX(aa)			# cos4[0,0,0,0,0,0,0,0]
	
	vshufps		ymm0, ymm0, ymm0, HEX(d8)	# bufs[32,36,33,37,40,44,41,45]
	vshufps		ymm1, ymm1, ymm1, HEX(d8)	# bufs[35,39,34,38,43,47,42,46]
	vshufps		ymm2, ymm2, ymm2, HEX(d8)	# bufs[48,52,49,53,56,60,57,61]
	vshufps		ymm3, ymm3, ymm3, HEX(d8)	# bufs[51,55,50,54,59,63,58,62]
	vunpcklps	ymm4, ymm0, ymm1			# bufs[32,35,36,39,40,43,44,47]
	vunpckhps	ymm5, ymm0, ymm1			# bufs[33,34,37,38,41,42,45,46]
	vunpcklps	ymm6, ymm2, ymm3			# bufs[48,51,52,55,56,59,60,63]
	vunpckhps	ymm7, ymm2, ymm3			# bufs[49,50,53,54,57,58,61,62]
	vsubps		ymm1, ymm4, ymm5
	vsubps		ymm3, ymm6, ymm7
	vaddps		ymm0, ymm4, ymm5			# bufs[0,2,4,6,8,10,12,14]
	vaddps		ymm2, ymm6, ymm7			# bufs[16,18,20,22,24,26,28,30]
	vmulps		ymm1, ymm8, ymm1			# bufs[1,3,5,7,9,11,13,15]
	vmulps		ymm3, ymm8, ymm3			# bufs[17,19,21,23,25,27,29,31]
	
	vxorps		ymm8, ymm8, ymm8
	vblendps	ymm5, ymm8, ymm1, HEX(aa)
	vblendps	ymm6, ymm8, ymm3, HEX(aa)
	vaddps		ymm0, ymm0, ymm5
	vaddps		ymm2, ymm2, ymm6
	vunpcklps	ymm4, ymm0, ymm1			# bufs[0,1,2,3,8,9,10,11]
	vunpckhps	ymm5, ymm0, ymm1			# bufs[4,5,6,7,12,13,14,15]
	vunpcklps	ymm6, ymm2, ymm3			# bufs[16,17,18,19,24,25,26,27]
	vunpckhps	ymm7, ymm2, ymm3			# bufs[20,21,22,23,28,29,30,31]
	
	vextractf128	xmm0, ymm4, HEX(1)		# bufs[8,9,10,11]
	vextractf128	xmm1, ymm5, HEX(1)		# bufs[12,13,14,15]
	vextractf128	xmm2, ymm6, HEX(1)		# bufs[24,25,26,27]
	vextractf128	xmm3, ymm7, HEX(1)		# bufs[28,29,30,31]
	
	vshufps		xmm9, xmm5, xmm5, HEX(1e)	# bufs[6,7,5,4]
	vshufps		xmm10, xmm1, xmm1, HEX(1e)	# bufs[14,15,13,12]
	vshufps		xmm11, xmm7, xmm7, HEX(1e)	# bufs[22,23,21,20]
	vshufps		xmm12, xmm3, xmm3, HEX(1e)	# bufs[30,31,29,28]
	vblendps	xmm9, xmm8, xmm9, HEX(7)	# bufs[6,7,5,-]
	vblendps	xmm10, xmm8, xmm10, HEX(7) # bufs[14,15,13,-]
	vblendps	xmm11, xmm8, xmm11, HEX(7)	# bufs[22,23,21,-]
	vblendps	xmm12, xmm8, xmm12, HEX(7)	# bufs[30,31,29,-]
	vaddps		xmm5, xmm9, xmm5
	vaddps		xmm1, xmm10, xmm1
	vaddps		xmm7, xmm11, xmm7
	vaddps		xmm3, xmm12, xmm3
	
	prefetcht0	512[out0]
	
	vshufps		xmm9, xmm0, xmm0, HEX(1e)	# bufs[10,11,9,8]
	vshufps		xmm10, xmm2, xmm2, HEX(1e)	# bufs[26,27,25,24]
	vaddps		xmm0, xmm0, xmm1
	vaddps		xmm2, xmm2, xmm3
	vblendps	xmm9, xmm8, xmm9, HEX(7)	# bufs[10,11,9,-]
	vblendps	xmm10, xmm8, xmm10, HEX(7)	# bufs[26,27,25,-]
	vaddps		xmm1, xmm9, xmm1
	vaddps		xmm3, xmm10, xmm3
	
	vzeroupper
	prefetcht0	512[out1]
	
	cvtps2dq	xmm4, xmm4
	cvtps2dq	xmm0, xmm0
	cvtps2dq	xmm5, xmm5
	cvtps2dq	xmm1, xmm1
	packssdw	xmm4, xmm5
	packssdw	xmm0, xmm1
	mov		rcx, xmm4
	pshufd		xmm5, xmm4, HEX(4e)
	mov		rdx, xmm0
	pshufd		xmm1, xmm0, HEX(4e)
	mov		r8, xmm5
	mov		r9, xmm1
	
	add		out0, 512
	mov		rax, -64
	
	mov		word ptr [out0], cx
	mov		word ptr [out0+rax*1], dx
	mov		word ptr [out0+rax*2], r8w
	mov		word ptr [out0+rax*2-64], r9w
	lea		out0, QWORD_PTR [out0+rax*4]
	shr			rcx, 16
	shr			rdx, 16
	shr			r8, 16
	shr			r9, 16
	mov		word ptr [out0+rax*4], cx
	neg		rax
	mov		word ptr [out1], cx
	mov		word ptr [out1+rax*1], dx
	mov		word ptr [out1+rax*2], r8w
	mov		word ptr 64[out1+rax*2], r9w
	lea		out1, QWORD_PTR [out1+rax*4]
	shr			rcx, 16
	shr			rdx, 16
	shr			r8, 16
	shr			r9, 16
	neg		rax
	mov		word ptr [out0], cx
	mov		word ptr [out0+rax*1], dx
	mov		word ptr [out0+rax*2], r8w
	mov		word ptr [out0+rax*2-64], r9w
	shr			rcx, 16
	shr			rdx, 16
	shr			r8, 16
	shr			r9, 16
	neg		rax
	mov		word ptr [out1], cx
	mov		word ptr [out1+rax*1], dx
	mov		word ptr [out1+rax*2], r8w
	mov		word ptr 64[out1+rax*2], r9w
	
	lea		out0, QWORD_PTR [out0+rax*4-32]
	neg		rax
	lea		out1, QWORD_PTR 32[out1+rax*4]
	
	vshufps		xmm0, xmm6, xmm6, HEX(1e)
	vblendps	xmm0, xmm8, xmm0, HEX(7)
	addps		xmm6, xmm2
	addps		xmm2, xmm7
	addps		xmm7, xmm3
	addps		xmm3, xmm0
	cvtps2dq	xmm6, xmm6
	cvtps2dq	xmm2, xmm2
	cvtps2dq	xmm7, xmm7
	cvtps2dq	xmm3, xmm3
	packssdw	xmm6, xmm7
	packssdw	xmm2, xmm3
	mov		rcx, xmm6
	pshufd		xmm7, xmm6, HEX(4e)
	mov		rdx, xmm2
	pshufd		xmm3, xmm2, HEX(4e)
	mov		r8, xmm7
	mov		r9, xmm3
	
	mov		word ptr [out0], cx
	mov		word ptr [out0+rax*1], dx
	mov		word ptr [out0+rax*2], r8w
	mov		word ptr [out0+rax*2-64], r9w
	lea		out0, QWORD_PTR [out0+rax*4]
	shr			rcx, 16
	shr			rdx, 16
	shr			r8, 16
	shr			r9, 16
	neg		rax
	mov		word ptr [out1], cx
	mov		word ptr [out1+rax*1], dx
	mov		word ptr [out1+rax*2], r8w
	mov		word ptr 64[out1+rax*2], r9w
	lea		out1, QWORD_PTR [out1+rax*4]
	shr			rcx, 16
	shr			rdx, 16
	shr			r8, 16
	shr			r9, 16
	neg		rax
	mov		word ptr [out0], cx
	mov		word ptr [out0+rax*1], dx
	mov		word ptr [out0+rax*2], r8w
	mov		word ptr [out0+rax*2-64], r9w
	shr			rcx, 16
	shr			rdx, 16
	shr			r8, 16
	shr			r9, 16
	neg		rax
	mov		word ptr [out1], cx
	mov		word ptr [out1+rax*1], dx
	mov		word ptr [out1+rax*2], r8w
	mov		word ptr 64[out1+rax*2], r9w
	
#ifdef IS_MSABI
	pop			rsi
	pop			rdi
	movaps		xmm6, [rsp]
	movaps		xmm7, 16[rsp]
	movaps		xmm8, 32[rsp]
	movaps		xmm9, 48[rsp]
	movaps		xmm10, 64[rsp]
	movaps		xmm11, 80[rsp]
	movaps		xmm12, 96[rsp]
	mov			rsp, rbp
	pop			rbp
#endif
	ret

NONEXEC_STACK

END_MODULE
