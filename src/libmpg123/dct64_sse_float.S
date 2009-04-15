/*
	dct64_sse_float: SSE optimized dct64 (float output version)

	copyright 2006-2007 by Zuxy Meng <zuxy.meng@gmail.com> / the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by the mysterious higway for MMX (apparently)
	then developed into SSE opt by Zuxy Meng, also building on Romain Dolbeau's AltiVec
	Both have agreed to distribution under LGPL 2.1 .
	
	Modification for float output by Taihei Monma

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
nnnn:
	.long	-2147483648
	.long	-2147483648
	.long	-2147483648
	.long	-2147483648
	ALIGN16
ppnn:
	.long	0
	.long	0
	.long	-2147483648
	.long	-2147483648
	ALIGN16
pnpn:
	.long	0
	.long	-2147483648
	.long	0
	.long	-2147483648
	ALIGN16
mask:
	.long	-1
	.long	-1
	.long	-1
	.long	0
	ALIGN4
one.4748:
	.long	1065353216

	.text
	ALIGN16,,15
.globl ASM_NAME(dct64_real_sse)
ASM_NAME(dct64_real_sse):
	pushl	%ebp
	movl	%esp, %ebp
	/* stack from ebp: 0=ebp 4=back 8=arg0 12=arg1 16=arg2 */
#define ARG(n) (8+n*4)(%ebp)
	andl	$-16, %esp /* align the stack at 16 bytes */
	subl	$256, %esp /* reserve space for local b1 and b2 */
	pushl	%ebx
/* stack from esp: 0=ebx 4...131=b2 132...259=b1 */

/* If you change the value of B1OFF or B2OFF, 
   please replace the values in line 406 and 408 as well */
#define B1OFF 132
#define B2OFF 4
#define B1(n) (B1OFF+n)(%esp)
#define B2(n) (B2OFF+n)(%esp)

	movl	ARG(2), %eax
	movl	ARG(0), %ecx
/* APP */
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
	movaps    %xmm1, B1(0)
	subps     %xmm2, %xmm4
	mulps     %xmm3, %xmm4
	movaps    %xmm4, B1(112)
	
/* NO_APP */
	movl	ARG(1), %ebx
/* APP */
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
	movaps    %xmm1, B1(16)
	subps     %xmm2, %xmm4
	mulps     %xmm3, %xmm4
	movaps    %xmm4, B1(96)
	
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
	movaps    %xmm1, B1(32)
	subps     %xmm2, %xmm4
	mulps     %xmm3, %xmm4
	movaps    %xmm4, B1(80)
	
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
	movaps    %xmm1, B1(48)
	subps     %xmm2, %xmm4
	mulps     %xmm3, %xmm4
	movaps    %xmm4, B1(64)
	
	movaps    B1(0), %xmm1
	movaps    B1(16), %xmm3
	movaps    B1(32), %xmm4
	movaps    B1(48), %xmm6
	movaps    %xmm1, %xmm7
	shufps    $27, %xmm7, %xmm7
	movaps    %xmm3, %xmm5
	shufps    $27, %xmm5, %xmm5
	movaps    %xmm4, %xmm2
	shufps    $27, %xmm2, %xmm2
	movaps    %xmm6, %xmm0
	shufps    $27, %xmm0, %xmm0
	addps     %xmm0, %xmm1
	movaps    %xmm1, B2(0)
	addps     %xmm2, %xmm3
	movaps    %xmm3, B2(16)
	subps     %xmm4, %xmm5
	movaps    %xmm5, B2(32)
	subps     %xmm6, %xmm7
	movaps    %xmm7, B2(48)
	
	movaps    B1(64), %xmm1
	movaps    B1(80), %xmm3
	movaps    B1(96), %xmm4
	movaps    B1(112), %xmm6
	movaps    %xmm1, %xmm7
	shufps    $27, %xmm7, %xmm7
	movaps    %xmm3, %xmm5
	shufps    $27, %xmm5, %xmm5
	movaps    %xmm4, %xmm2
	shufps    $27, %xmm2, %xmm2
	movaps    %xmm6, %xmm0
	shufps    $27, %xmm0, %xmm0
	addps     %xmm0, %xmm1
	movaps    %xmm1, B2(64)
	addps     %xmm2, %xmm3
	movaps    %xmm3, B2(80)
	subps     %xmm4, %xmm5
	movaps    %xmm5, B2(96)
	subps     %xmm6, %xmm7
	movaps    %xmm7, B2(112)
	
	movaps    B2(32), %xmm0
	movaps    B2(48), %xmm1
	movaps    ASM_NAME(costab_mmxsse)+64, %xmm4
	xorps     %xmm6, %xmm6
	shufps    $27, %xmm4, %xmm4
	mulps     %xmm4, %xmm1
	movaps    ASM_NAME(costab_mmxsse)+80, %xmm2
	xorps     %xmm7, %xmm7
	shufps    $27, %xmm2, %xmm2
	mulps     %xmm2, %xmm0
	movaps    %xmm0, B2(32)
	movaps    %xmm1, B2(48)
	movaps    B2(96), %xmm3
	mulps     %xmm2, %xmm3
	subps     %xmm3, %xmm6
	movaps    %xmm6, B2(96)
	movaps    B2(112), %xmm5
	mulps     %xmm4, %xmm5
	subps     %xmm5, %xmm7
	movaps    %xmm7, B2(112)
	
	movaps    ASM_NAME(costab_mmxsse)+96, %xmm0
	shufps    $27, %xmm0, %xmm0
	movaps    nnnn, %xmm5
	movaps    %xmm5, %xmm6
	
	movaps    B2(0), %xmm2
	movaps    B2(16), %xmm3
	movaps    %xmm2, %xmm4
	xorps     %xmm5, %xmm6
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm3, %xmm1
	shufps    $27, %xmm1, %xmm1
	addps     %xmm1, %xmm2
	movaps    %xmm2, B1(0)
	subps     %xmm3, %xmm4
	xorps     %xmm6, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, B1(16)
	
	movaps    B2(32), %xmm2
	movaps    B2(48), %xmm3
	movaps    %xmm2, %xmm4
	xorps     %xmm5, %xmm6
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm3, %xmm1
	shufps    $27, %xmm1, %xmm1
	addps     %xmm1, %xmm2
	movaps    %xmm2, B1(32)
	subps     %xmm3, %xmm4
	xorps     %xmm6, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, B1(48)
	
	movaps    B2(64), %xmm2
	movaps    B2(80), %xmm3
	movaps    %xmm2, %xmm4
	xorps     %xmm5, %xmm6
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm3, %xmm1
	shufps    $27, %xmm1, %xmm1
	addps     %xmm1, %xmm2
	movaps    %xmm2, B1(64)
	subps     %xmm3, %xmm4
	xorps     %xmm6, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, B1(80)
	
	movaps    B2(96), %xmm2
	movaps    B2(112), %xmm3
	movaps    %xmm2, %xmm4
	xorps     %xmm5, %xmm6
	shufps    $27, %xmm4, %xmm4
	movaps    %xmm3, %xmm1
	shufps    $27, %xmm1, %xmm1
	addps     %xmm1, %xmm2
	movaps    %xmm2, B1(96)
	subps     %xmm3, %xmm4
	xorps     %xmm6, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, B1(112)
	
	movss     one.4748, %xmm1
	movss     ASM_NAME(costab_mmxsse)+112, %xmm0
	movaps    %xmm1, %xmm3
	unpcklps  %xmm0, %xmm3
	movss     ASM_NAME(costab_mmxsse)+116, %xmm2
	movaps    %xmm1, %xmm0
	unpcklps  %xmm2, %xmm0
	unpcklps  %xmm3, %xmm0
	movaps    ppnn, %xmm2
	
	movaps    B1(0), %xmm3
	movaps    %xmm3, %xmm4
	shufps    $20, %xmm4, %xmm4
	shufps    $235, %xmm3, %xmm3
	xorps     %xmm2, %xmm3
	addps     %xmm3, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, B2(0)
	movaps    B1(16), %xmm6
	movaps    %xmm6, %xmm5
	shufps    $27, %xmm5, %xmm5
	xorps     %xmm2, %xmm5
	addps     %xmm5, %xmm6
	mulps     %xmm0, %xmm6
	movaps    %xmm6, B2(16)
	
	movaps    B1(32), %xmm3
	movaps    %xmm3, %xmm4
	shufps    $20, %xmm4, %xmm4
	shufps    $235, %xmm3, %xmm3
	xorps     %xmm2, %xmm3
	addps     %xmm3, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, B2(32)
	movaps    B1(48), %xmm6
	movaps    %xmm6, %xmm5
	shufps    $27, %xmm5, %xmm5
	xorps     %xmm2, %xmm5
	addps     %xmm5, %xmm6
	mulps     %xmm0, %xmm6
	movaps    %xmm6, B2(48)
	
	movaps    B1(64), %xmm3
	movaps    %xmm3, %xmm4
	shufps    $20, %xmm4, %xmm4
	shufps    $235, %xmm3, %xmm3
	xorps     %xmm2, %xmm3
	addps     %xmm3, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, B2(64)
	movaps    B1(80), %xmm6
	movaps    %xmm6, %xmm5
	shufps    $27, %xmm5, %xmm5
	xorps     %xmm2, %xmm5
	addps     %xmm5, %xmm6
	mulps     %xmm0, %xmm6
	movaps    %xmm6, B2(80)
	
	movaps    B1(96), %xmm3
	movaps    %xmm3, %xmm4
	shufps    $20, %xmm4, %xmm4
	shufps    $235, %xmm3, %xmm3
	xorps     %xmm2, %xmm3
	addps     %xmm3, %xmm4
	mulps     %xmm0, %xmm4
	movaps    %xmm4, B2(96)
	movaps    B1(112), %xmm6
	movaps    %xmm6, %xmm5
	shufps    $27, %xmm5, %xmm5
	xorps     %xmm2, %xmm5
	addps     %xmm5, %xmm6
	mulps     %xmm0, %xmm6
	movaps    %xmm6, B2(112)
	
	movss     ASM_NAME(costab_mmxsse)+120, %xmm0
	movaps    %xmm1, %xmm2
	movaps    %xmm0, %xmm7
	unpcklps  %xmm1, %xmm2
	unpcklps  %xmm0, %xmm7
	movaps    pnpn, %xmm0
	unpcklps  %xmm7, %xmm2
	
	movaps    B2(0), %xmm1
	movaps    %xmm1, %xmm3
	shufps    $224, %xmm3, %xmm3
	shufps    $181, %xmm1, %xmm1
	xorps     %xmm0, %xmm1
	addps     %xmm1, %xmm3
	mulps     %xmm2, %xmm3
	movaps    %xmm3, B1(0)
	movaps    B2(16), %xmm4
	movaps    %xmm4, %xmm5
	shufps    $224, %xmm5, %xmm5
	shufps    $181, %xmm4, %xmm4
	xorps     %xmm0, %xmm4
	addps     %xmm4, %xmm5
	mulps     %xmm2, %xmm5
	movaps    %xmm5, B1(16)
	
	movaps    B2(32), %xmm1
	movaps    %xmm1, %xmm3
	shufps    $224, %xmm3, %xmm3
	shufps    $181, %xmm1, %xmm1
	xorps     %xmm0, %xmm1
	addps     %xmm1, %xmm3
	mulps     %xmm2, %xmm3
	movaps    %xmm3, B1(32)
	movaps    B2(48), %xmm4
	movaps    %xmm4, %xmm5
	shufps    $224, %xmm5, %xmm5
	shufps    $181, %xmm4, %xmm4
	xorps     %xmm0, %xmm4
	addps     %xmm4, %xmm5
	mulps     %xmm2, %xmm5
	movaps    %xmm5, B1(48)
	
	movaps    B2(64), %xmm1
	movaps    %xmm1, %xmm3
	shufps    $224, %xmm3, %xmm3
	shufps    $181, %xmm1, %xmm1
	xorps     %xmm0, %xmm1
	addps     %xmm1, %xmm3
	mulps     %xmm2, %xmm3
	movaps    %xmm3, B1(64)
	movaps    B2(80), %xmm4
	movaps    %xmm4, %xmm5
	shufps    $224, %xmm5, %xmm5
	shufps    $181, %xmm4, %xmm4
	xorps     %xmm0, %xmm4
	addps     %xmm4, %xmm5
	mulps     %xmm2, %xmm5
	movaps    %xmm5, B1(80)
	
	movaps    B2(96), %xmm1
	movaps    %xmm1, %xmm3
	shufps    $224, %xmm3, %xmm3
	shufps    $181, %xmm1, %xmm1
	xorps     %xmm0, %xmm1
	addps     %xmm1, %xmm3
	mulps     %xmm2, %xmm3
	movaps    %xmm3, B1(96)
	movaps    B2(112), %xmm4
	movaps    %xmm4, %xmm5
	shufps    $224, %xmm5, %xmm5
	shufps    $181, %xmm4, %xmm4
	xorps     %xmm0, %xmm4
	addps     %xmm4, %xmm5
	mulps     %xmm2, %xmm5
	movaps    %xmm5, B1(112)
	
	movss		B1(12), %xmm0
	movss		B1(28), %xmm1
	movss		B1(44), %xmm2
	movss		B1(60), %xmm3
	addss		B1(8), %xmm0
	addss		B1(24), %xmm1
	addss		B1(40), %xmm2
	addss		B1(56), %xmm3
	movss		%xmm0, B1(8)
	movss		%xmm1, B1(24)
	movss		%xmm2, B1(40)
	movss		%xmm3, B1(56)
	movss		B1(76), %xmm0
	movss		B1(92), %xmm1
	movss		B1(108), %xmm2
	movss		B1(124), %xmm3
	addss		B1(72), %xmm0
	addss		B1(88), %xmm1
	addss		B1(104), %xmm2
	addss		B1(120), %xmm3
	movss		%xmm0, B1(72)
	movss		%xmm1, B1(88)
	movss		%xmm2, B1(104)
	movss		%xmm3, B1(120)
	
	movaps		B1(16), %xmm1
	movaps		B1(48), %xmm3
	movaps		B1(80), %xmm5
	movaps		B1(112), %xmm7
	movaps		%xmm1, %xmm0
	movaps		%xmm3, %xmm2
	movaps		%xmm5, %xmm4
	movaps		%xmm7, %xmm6
	shufps		$0x1e, %xmm0, %xmm0
	shufps		$0x1e, %xmm2, %xmm2
	shufps		$0x1e, %xmm4, %xmm4
	shufps		$0x1e, %xmm6, %xmm6
	andps		mask, %xmm0
	andps		mask, %xmm2
	andps		mask, %xmm4
	andps		mask, %xmm6
	addps		%xmm0, %xmm1
	addps		%xmm2, %xmm3
	addps		%xmm4, %xmm5
	addps		%xmm6, %xmm7
	
	movaps		B1(32), %xmm2
	movaps		B1(96), %xmm6
	movaps		%xmm2, %xmm0
	movaps		%xmm6, %xmm4
	shufps		$0x1e, %xmm0, %xmm0
	shufps		$0x1e, %xmm4, %xmm4
	andps		mask, %xmm0
	andps		mask, %xmm4
	addps		%xmm3, %xmm2
	addps		%xmm0, %xmm3
	addps		%xmm7, %xmm6
	addps		%xmm4, %xmm7
	
	movaps		B1(0), %xmm0
	movaps		B1(64), %xmm4
	
	movss		%xmm0, 1024(%ecx)
	movss		%xmm2, 896(%ecx)
	movss		%xmm1, 768(%ecx)
	movss		%xmm3, 640(%ecx)
	
	shufps		$0xe1, %xmm0, %xmm0
	shufps		$0xe1, %xmm2, %xmm2
	shufps		$0xe1, %xmm1, %xmm1
	shufps		$0xe1, %xmm3, %xmm3
	movss		%xmm0, (%ecx)
	movss		%xmm0, (%ebx)
	movss		%xmm2, 128(%ebx)
	movss		%xmm1, 256(%ebx)
	movss		%xmm3, 384(%ebx)
	
	movhlps		%xmm0, %xmm0
	movhlps		%xmm2, %xmm2
	movhlps		%xmm1, %xmm1
	movhlps		%xmm3, %xmm3
	movss		%xmm0, 512(%ecx)
	movss		%xmm2, 384(%ecx)
	movss		%xmm1, 256(%ecx)
	movss		%xmm3, 128(%ecx)
	
	shufps		$0xe1, %xmm0, %xmm0
	shufps		$0xe1, %xmm2, %xmm2
	shufps		$0xe1, %xmm1, %xmm1
	shufps		$0xe1, %xmm3, %xmm3
	movss		%xmm0, 512(%ebx)
	movss		%xmm2, 640(%ebx)
	movss		%xmm1, 768(%ebx)
	movss		%xmm3, 896(%ebx)
	
	movaps		%xmm4, %xmm0
	shufps		$0x1e, %xmm0, %xmm0
	movaps		%xmm5, %xmm1
	andps		mask, %xmm0
	
	addps		%xmm6, %xmm4
	addps		%xmm7, %xmm5
	addps		%xmm1, %xmm6
	addps		%xmm0, %xmm7
	
	movss		%xmm4, 960(%ecx)
	movss		%xmm6, 832(%ecx)
	movss		%xmm5, 704(%ecx)
	movss		%xmm7, 576(%ecx)
	movhlps		%xmm4, %xmm0
	movhlps		%xmm6, %xmm1
	movhlps		%xmm5, %xmm2
	movhlps		%xmm7, %xmm3
	movss		%xmm0, 448(%ecx)
	movss		%xmm1, 320(%ecx)
	movss		%xmm2, 192(%ecx)
	movss		%xmm3, 64(%ecx)
	
	shufps		$0xe1, %xmm4, %xmm4
	shufps		$0xe1, %xmm6, %xmm6
	shufps		$0xe1, %xmm5, %xmm5
	shufps		$0xe1, %xmm7, %xmm7
	movss		%xmm4, 64(%ebx)
	movss		%xmm6, 192(%ebx)
	movss		%xmm5, 320(%ebx)
	movss		%xmm7, 448(%ebx)
	
	shufps		$0xe1, %xmm0, %xmm0
	shufps		$0xe1, %xmm1, %xmm1
	shufps		$0xe1, %xmm2, %xmm2
	shufps		$0xe1, %xmm3, %xmm3
	movss		%xmm0, 576(%ebx)
	movss		%xmm1, 704(%ebx)
	movss		%xmm2, 832(%ebx)
	movss		%xmm3, 960(%ebx)

	popl	%ebx
	movl	%ebp, %esp
	popl	%ebp
	ret

/* Mark non-executable stack. */
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
