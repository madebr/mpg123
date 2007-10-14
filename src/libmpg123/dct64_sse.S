/*
	dct64_sse: MMX/SSE optimized dct64

	copyright 2006-2007 by Zuxy Meng <zuxy.meng@gmail.com> / the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by the mysterious higway for MMX (apparently)
	then developed into SSE opt by Zuxy Meng, also building on Romain Dolbeau's AltiVec
	Both have agreed to distribution under LGPL 2.1 .

	Transformed back into standalone asm, with help of
	gcc -S -DHAVE_CONFIG_H -I.  -march=pentium3 -O3 -Wall -pedantic -fno-strict-aliasing -DREAL_IS_FLOAT -c -o dct64_sse.{S,c}

	Original comment from MPlayer source follows:
*/

/*
 * Discrete Cosine Tansform (DCT) for SSE
 * based upon code from mp3lib/dct64.c, mp3lib/dct64_altivec.c
 * and mp3lib/dct64_MMX.c
 */

#include "mangle.h"

#ifndef __APPLE__
	.section	.rodata
#else
	.data
#endif
	ALIGN16
	/* .type	nnnn, @object
	   .size	nnnn, 16 */
nnnn:
	.long	-2147483648
	.long	-2147483648
	.long	-2147483648
	.long	-2147483648
	ALIGN16
	/* .type	ppnn, @object
	   .size	ppnn, 16 */
ppnn:
	.long	0
	.long	0
	.long	-2147483648
	.long	-2147483648
	ALIGN16
	/* .type	pnpn, @object
	   .size	pnpn, 16 */
pnpn:
	.long	0
	.long	-2147483648
	.long	0
	.long	-2147483648
	ALIGN4
	/* .type	one.4748, @object
	   .size	one.4748, 4 */
one.4748:
	.long	1065353216

	/* no .data ? */
	/* .local	b2.4747 */
	ALIGN16
	COMM(b2.4747,128,16)
	/* .local	b1.4746 */
	ALIGN16
	COMM(b1.4746,128,16)

	.text
	ALIGN16,,15
.globl ASM_NAME(dct64_sse)
	/* .type	ASM_NAME(dct64_sse), @function */
ASM_NAME(dct64_sse):
	pushl	%ebp
	movl	%esp, %ebp
	movl	16(%ebp), %eax
	pushl	%ebx
	movl	8(%ebp), %ecx
#APP
/* for (i = 0; i < 0x20 / 2; i += 4) cycle 1 */
	movaps    ASM_NAME(costab_mmxsse), %xmm3
	shufps    $27, %xmm3, %xmm3
	MOVUAPS    (%eax), %xmm1
	movaps    %xmm1, %xmm4
	MOVUAPS    112(%eax), %xmm2
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm2, %xmm0
	shufps    $27, %xmm0, %xmm0
	addps     %xmm0, %xmm1
	movaps    %xmm1, b1.4746
	subps     %xmm2, %xmm4
	mulps     %xmm3, %xmm4
	movaps    %xmm4, b1.4746+112
	
#NO_APP
	movl	12(%ebp), %ebx
#APP
/* for (i = 0; i < 0x20 / 2; i += 4) cycle 2 */
	movaps    ASM_NAME(costab_mmxsse)+16, %xmm3
	shufps    $27, %xmm3, %xmm3
	MOVUAPS    16(%eax), %xmm1
	movaps    %xmm1, %xmm4
	MOVUAPS    96(%eax), %xmm2
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm2, %xmm0
	shufps    $27, %xmm0, %xmm0
	addps     %xmm0, %xmm1
	movaps    %xmm1, b1.4746+16
	subps     %xmm2, %xmm4
	mulps     %xmm3, %xmm4
	movaps    %xmm4, b1.4746+96
	
/* for (i = 0; i < 0x20 / 2; i += 4) cycle 3 */
	movaps    ASM_NAME(costab_mmxsse)+32, %xmm3
	shufps    $27, %xmm3, %xmm3
	MOVUAPS    32(%eax), %xmm1
	movaps    %xmm1, %xmm4
	MOVUAPS    80(%eax), %xmm2
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm2, %xmm0
	shufps    $27, %xmm0, %xmm0
	addps     %xmm0, %xmm1
	movaps    %xmm1, b1.4746+32
	subps     %xmm2, %xmm4
	mulps     %xmm3, %xmm4
	movaps    %xmm4, b1.4746+80
	
/* for (i = 0; i < 0x20 / 2; i += 4) cycle 4 */
	movaps    ASM_NAME(costab_mmxsse)+48, %xmm3
	shufps    $27, %xmm3, %xmm3
	MOVUAPS    48(%eax), %xmm1
	movaps    %xmm1, %xmm4
	MOVUAPS    64(%eax), %xmm2
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm2, %xmm0
	shufps    $27, %xmm0, %xmm0
	addps     %xmm0, %xmm1
	movaps    %xmm1, b1.4746+48
	subps     %xmm2, %xmm4
	mulps     %xmm3, %xmm4
	movaps    %xmm4, b1.4746+64
	
	movaps    b1.4746, %xmm1
	movaps    b1.4746+16, %xmm3
	movaps    b1.4746+32, %xmm4
	movaps    b1.4746+48, %xmm6
	movaps    %xmm1, %xmm7
	shufps    $27, %xmm7, %xmm7
	movaps    %xmm3, %xmm5
	shufps    $27, %xmm5, %xmm5
	movaps    %xmm4, %xmm2
	shufps    $27, %xmm2, %xmm2
	movaps    %xmm6, %xmm0
	shufps    $27, %xmm0, %xmm0
	addps     %xmm0, %xmm1
	movaps    %xmm1, b2.4747
	addps     %xmm2, %xmm3
	movaps    %xmm3, b2.4747+16
	subps     %xmm4, %xmm5
	movaps    %xmm5, b2.4747+32
	subps     %xmm6, %xmm7
	movaps    %xmm7, b2.4747+48
	
	movaps    b1.4746+64, %xmm1
	movaps    b1.4746+80, %xmm3
	movaps    b1.4746+96, %xmm4
	movaps    b1.4746+112, %xmm6
	movaps    %xmm1, %xmm7
	shufps    $27, %xmm7, %xmm7
	movaps    %xmm3, %xmm5
	shufps    $27, %xmm5, %xmm5
	movaps    %xmm4, %xmm2
	shufps    $27, %xmm2, %xmm2
	movaps    %xmm6, %xmm0
	shufps    $27, %xmm0, %xmm0
	addps     %xmm0, %xmm1
	movaps    %xmm1, b2.4747+64
	addps     %xmm2, %xmm3
	movaps    %xmm3, b2.4747+80
	subps     %xmm4, %xmm5
	movaps    %xmm5, b2.4747+96
	subps     %xmm6, %xmm7
	movaps    %xmm7, b2.4747+112
	
	movaps    b2.4747+32, %xmm0
	movaps    b2.4747+48, %xmm1
	movaps    ASM_NAME(costab_mmxsse)+64, %xmm4
	xorps     %xmm6, %xmm6
	shufps    $27, %xmm4, %xmm4
	mulps     %xmm4, %xmm1
	movaps    ASM_NAME(costab_mmxsse)+80, %xmm2
	xorps     %xmm7, %xmm7
	shufps    $27, %xmm2, %xmm2
	mulps     %xmm2, %xmm0
	movaps    %xmm0, b2.4747+32
	movaps    %xmm1, b2.4747+48
	movaps    b2.4747+96, %xmm3
	mulps     %xmm2, %xmm3
	subps     %xmm3, %xmm6
	movaps    %xmm6, b2.4747+96
	movaps    b2.4747+112, %xmm5
	mulps     %xmm4, %xmm5
	subps     %xmm5, %xmm7
	movaps    %xmm7, b2.4747+112
	
	movaps    ASM_NAME(costab_mmxsse)+96, %xmm0
	shufps    $27, %xmm0, %xmm0
	movaps    nnnn, %xmm5
	movaps    %xmm5, %xmm6
	
	movaps    b2.4747, %xmm2
	movaps    b2.4747+16, %xmm3
	movaps    %xmm2, %xmm4
	xorps     %xmm5, %xmm6
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm3, %xmm1
	shufps    $27, %xmm1, %xmm1
	addps     %xmm1, %xmm2
	movaps    %xmm2, b1.4746
	subps     %xmm3, %xmm4
	xorps     %xmm6, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, b1.4746+16
	
	movaps    b2.4747+32, %xmm2
	movaps    b2.4747+48, %xmm3
	movaps    %xmm2, %xmm4
	xorps     %xmm5, %xmm6
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm3, %xmm1
	shufps    $27, %xmm1, %xmm1
	addps     %xmm1, %xmm2
	movaps    %xmm2, b1.4746+32
	subps     %xmm3, %xmm4
	xorps     %xmm6, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, b1.4746+48
	
	movaps    b2.4747+64, %xmm2
	movaps    b2.4747+80, %xmm3
	movaps    %xmm2, %xmm4
	xorps     %xmm5, %xmm6
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm3, %xmm1
	shufps    $27, %xmm1, %xmm1
	addps     %xmm1, %xmm2
	movaps    %xmm2, b1.4746+64
	subps     %xmm3, %xmm4
	xorps     %xmm6, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, b1.4746+80
	
	movaps    b2.4747+96, %xmm2
	movaps    b2.4747+112, %xmm3
	movaps    %xmm2, %xmm4
	xorps     %xmm5, %xmm6
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm3, %xmm1
	shufps    $27, %xmm1, %xmm1
	addps     %xmm1, %xmm2
	movaps    %xmm2, b1.4746+96
	subps     %xmm3, %xmm4
	xorps     %xmm6, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, b1.4746+112
	
	movss     one.4748, %xmm1
	movss     ASM_NAME(costab_mmxsse)+112, %xmm0
	movaps    %xmm1, %xmm3
	unpcklps  %xmm0, %xmm3
	movss     ASM_NAME(costab_mmxsse)+116, %xmm2
	movaps    %xmm1, %xmm0
	unpcklps  %xmm2, %xmm0
	unpcklps  %xmm3, %xmm0
	movaps    ppnn, %xmm2
	
	movaps    b1.4746, %xmm3
	movaps    %xmm3, %xmm4
	shufps    $20, %xmm4, %xmm4
	shufps    $235, %xmm3, %xmm3
	xorps     %xmm2, %xmm3
	addps     %xmm3, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, b2.4747
	movaps    b1.4746+16, %xmm6
	movaps    %xmm6, %xmm5
	shufps    $27, %xmm5, %xmm5
	xorps     %xmm2, %xmm5
	addps     %xmm5, %xmm6
	mulps     %xmm0, %xmm6
	movaps    %xmm6, b2.4747+16
	
	movaps    b1.4746+32, %xmm3
	movaps    %xmm3, %xmm4
	shufps    $20, %xmm4, %xmm4
	shufps    $235, %xmm3, %xmm3
	xorps     %xmm2, %xmm3
	addps     %xmm3, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, b2.4747+32
	movaps    b1.4746+48, %xmm6
	movaps    %xmm6, %xmm5
	shufps    $27, %xmm5, %xmm5
	xorps     %xmm2, %xmm5
	addps     %xmm5, %xmm6
	mulps     %xmm0, %xmm6
	movaps    %xmm6, b2.4747+48
	
	movaps    b1.4746+64, %xmm3
	movaps    %xmm3, %xmm4
	shufps    $20, %xmm4, %xmm4
	shufps    $235, %xmm3, %xmm3
	xorps     %xmm2, %xmm3
	addps     %xmm3, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, b2.4747+64
	movaps    b1.4746+80, %xmm6
	movaps    %xmm6, %xmm5
	shufps    $27, %xmm5, %xmm5
	xorps     %xmm2, %xmm5
	addps     %xmm5, %xmm6
	mulps     %xmm0, %xmm6
	movaps    %xmm6, b2.4747+80
	
	movaps    b1.4746+96, %xmm3
	movaps    %xmm3, %xmm4
	shufps    $20, %xmm4, %xmm4
	shufps    $235, %xmm3, %xmm3
	xorps     %xmm2, %xmm3
	addps     %xmm3, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, b2.4747+96
	movaps    b1.4746+112, %xmm6
	movaps    %xmm6, %xmm5
	shufps    $27, %xmm5, %xmm5
	xorps     %xmm2, %xmm5
	addps     %xmm5, %xmm6
	mulps     %xmm0, %xmm6
	movaps    %xmm6, b2.4747+112
	
	movss     ASM_NAME(costab_mmxsse)+120, %xmm0
	movaps    %xmm1, %xmm2
	movaps    %xmm0, %xmm7
	unpcklps  %xmm1, %xmm2
	unpcklps  %xmm0, %xmm7
	movaps    pnpn, %xmm0
	unpcklps  %xmm7, %xmm2
	
	movaps    b2.4747+32, %xmm1
	movaps    %xmm1, %xmm3
	shufps    $224, %xmm3, %xmm3
	shufps    $181, %xmm1, %xmm1
	xorps     %xmm0, %xmm1
	addps     %xmm1, %xmm3
	mulps     %xmm2, %xmm3
	movaps    %xmm3, b1.4746+32
	movaps    b2.4747+48, %xmm4
	movaps    %xmm4, %xmm5
	shufps    $224, %xmm5, %xmm5
	shufps    $181, %xmm4, %xmm4
	xorps     %xmm0, %xmm4
	addps     %xmm4, %xmm5
	mulps     %xmm2, %xmm5
	movaps    %xmm5, b1.4746+48
	
	movaps    b2.4747+64, %xmm1
	movaps    %xmm1, %xmm3
	shufps    $224, %xmm3, %xmm3
	shufps    $181, %xmm1, %xmm1
	xorps     %xmm0, %xmm1
	addps     %xmm1, %xmm3
	mulps     %xmm2, %xmm3
	movaps    %xmm3, b1.4746+64
	movaps    b2.4747+80, %xmm4
	movaps    %xmm4, %xmm5
	shufps    $224, %xmm5, %xmm5
	shufps    $181, %xmm4, %xmm4
	xorps     %xmm0, %xmm4
	addps     %xmm4, %xmm5
	mulps     %xmm2, %xmm5
	movaps    %xmm5, b1.4746+80
	
	movaps    b2.4747+96, %xmm1
	movaps    %xmm1, %xmm3
	shufps    $224, %xmm3, %xmm3
	shufps    $181, %xmm1, %xmm1
	xorps     %xmm0, %xmm1
	addps     %xmm1, %xmm3
	mulps     %xmm2, %xmm3
	movaps    %xmm3, b1.4746+96
	movaps    b2.4747+112, %xmm4
	movaps    %xmm4, %xmm5
	shufps    $224, %xmm5, %xmm5
	shufps    $181, %xmm4, %xmm4
	xorps     %xmm0, %xmm4
	addps     %xmm4, %xmm5
	mulps     %xmm2, %xmm5
	movaps    %xmm5, b1.4746+112
	
#NO_APP
	flds	b1.4746+40
	movl	$b1.4746, %edx
	movl	$b2.4747, %eax
	fadds	b1.4746+44
	fstps	b1.4746+40
	flds	b1.4746+56
	fadds	b1.4746+60
	flds	b1.4746+48
	fadd	%st(1), %st
	fstps	b1.4746+48
	fadds	b1.4746+52
	fstps	b1.4746+56
	flds	b1.4746+52
	fadds	b1.4746+60
	fstps	b1.4746+52
	flds	b1.4746+72
	fadds	b1.4746+76
	fstps	b1.4746+72
	flds	b1.4746+88
	fadds	b1.4746+92
	flds	b1.4746+80
	fadd	%st(1), %st
	fstps	b1.4746+80
	fadds	b1.4746+84
	fstps	b1.4746+88
	flds	b1.4746+84
	fadds	b1.4746+92
	fstps	b1.4746+84
	flds	b1.4746+104
	fadds	b1.4746+108
	fstps	b1.4746+104
	flds	b1.4746+120
	fadds	b1.4746+124
	flds	b1.4746+112
	fadd	%st(1), %st
	fstps	b1.4746+112
	fadds	b1.4746+116
	fstps	b1.4746+120
	flds	b1.4746+116
	fadds	b1.4746+124
	fstps	b1.4746+116
#APP
	flds       ASM_NAME(costab_mmxsse)+120
	flds     (%eax)
	fadds   4(%eax)
	fistp 512(%ecx)
	flds     (%eax)
	fsubs   4(%eax)
	fmul  %st(1)
	fistp    (%ecx)
	flds   12(%eax)
	fsubs   8(%eax)
	fmul  %st(1)
	fist  256(%ebx)
	fadds  12(%eax)
	fadds   8(%eax)
	fistp 256(%ecx)
	flds   16(%eax)
	fsubs  20(%eax)
	fmul  %st(1)
	flds   28(%eax)
	fsubs  24(%eax)
	fmul  %st(2)
	fist  384(%ebx)
	fld   %st(0)
	fadds  24(%eax)
	fadds  28(%eax)
	fld   %st(0)
	fadds  16(%eax)
	fadds  20(%eax)
	fistp 384(%ecx)
	fadd  %st(2)
	fistp 128(%ecx)
	faddp %st(1)
	fistp 128(%ebx)
	flds   32(%edx)
	fadds  48(%edx)
	fistp 448(%ecx)
	flds   48(%edx)
	fadds  40(%edx)
	fistp 320(%ecx)
	flds   40(%edx)
	fadds  56(%edx)
	fistp 192(%ecx)
	flds   56(%edx)
	fadds  36(%edx)
	fistp  64(%ecx)
	flds   36(%edx)
	fadds  52(%edx)
	fistp  64(%ebx)
	flds   52(%edx)
	fadds  44(%edx)
	fistp 192(%ebx)
	flds   60(%edx)
	fist  448(%ebx)
	fadds  44(%edx)
	fistp 320(%ebx)
	flds   96(%edx)
	fadds 112(%edx)
	fld   %st(0)
	fadds  64(%edx)
	fistp 480(%ecx)
	fadds  80(%edx)
	fistp 416(%ecx)
	flds  112(%edx)
	fadds 104(%edx)
	fld   %st(0)
	fadds  80(%edx)
	fistp 352(%ecx)
	fadds  72(%edx)
	fistp 288(%ecx)
	flds  104(%edx)
	fadds 120(%edx)
	fld   %st(0)
	fadds  72(%edx)
	fistp 224(%ecx)
	fadds  88(%edx)
	fistp 160(%ecx)
	flds  120(%edx)
	fadds 100(%edx)
	fld   %st(0)
	fadds  88(%edx)
	fistp  96(%ecx)
	fadds  68(%edx)
	fistp  32(%ecx)
	flds  100(%edx)
	fadds 116(%edx)
	fld   %st(0)
	fadds  68(%edx)
	fistp  32(%ebx)
	fadds  84(%edx)
	fistp  96(%ebx)
	flds  116(%edx)
	fadds 108(%edx)
	fld   %st(0)
	fadds  84(%edx)
	fistp 160(%ebx)
	fadds  76(%edx)
	fistp 224(%ebx)
	flds  108(%edx)
	fadds 124(%edx)
	fld   %st(0)
	fadds  76(%edx)
	fistp 288(%ebx)
	fadds  92(%edx)
	fistp 352(%ebx)
	flds  124(%edx)
	fist  480(%ebx)
	fadds  92(%edx)
	fistp 416(%ebx)
	ffreep %st(0)
	
#NO_APP
	movzwl	(%ecx), %eax
	movw	%ax, (%ebx)
	popl	%ebx
	popl	%ebp
	ret
	/* .size	ASM_NAME(dct64_sse), .-ASM_NAME(dct64_sse) */
