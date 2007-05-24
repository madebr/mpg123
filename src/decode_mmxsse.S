/*
	decode_mmxsse: Synth for SSE and extended 3DNow (yeah, the name is a relic)

	copyright 2006-2007 by Zuxy Meng/the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by the mysterious higway for MMX (apparently)
	then developed into SSE opt by Zuxy Meng, also building on Romain Dolbeau's AltiVec
	Both have agreed to distribution under LGPL 2.1 .

	Transformed back into standalone asm, with help of
	gcc -S -DHAVE_CONFIG_H -I.  -march=pentium -O3 -Wall -pedantic -fno-strict-aliasing -DREAL_IS_FLOAT -c -o decode_mmxsse.{S,c}

	Original comment from MPlayer source follows:
*/

/*
 * this code comes under GPL
 * This code was taken from http://www.mpg123.org
 * See ChangeLog of mpg123-0.59s-pre.1 for detail
 * Applied to mplayer by Nick Kurshev <nickols_k@mail.ru>
 *
 * Local ChangeLog:
 * - Partial loops unrolling and removing MOVW insn from loops
*/

#include "mangle.h"

.globl ASM_NAME(costab_mmxsse)
	.data
	.align 16
	/* .type	ASM_NAME(costab_mmxsse), @object
	   .size	ASM_NAME(costab_mmxsse), 124 */
ASM_NAME(costab_mmxsse):
	.long	1056974725
	.long	1057056395
	.long	1057223771
	.long	1057485416
	.long	1057855544
	.long	1058356026
	.long	1059019886
	.long	1059897405
	.long	1061067246
	.long	1062657950
	.long	1064892987
	.long	1066774581
	.long	1069414683
	.long	1073984175
	.long	1079645762
	.long	1092815430
	.long	1057005197
	.long	1057342072
	.long	1058087743
	.long	1059427869
	.long	1061799040
	.long	1065862217
	.long	1071413542
	.long	1084439708
	.long	1057128951
	.long	1058664893
	.long	1063675095
	.long	1076102863
	.long	1057655764
	.long	1067924853
	.long	1060439283
	.align 8
	/* .type	one_null, @object
	   .size	one_null, 8 */
one_null:
	.long	-65536
	.long	-65536
	.align 8
	/* .type	null_one, @object
	.size	null_one, 8 */
null_one:
	.long	65535
	.long	65535
	/* .local	temp */
	COMM(temp,4,4)

	.text
	.p2align 4,,15
	/* void synth_1to1_sse_s(real *bandPtr, int channel, short *samples, short *buffs, int *bo) */
.globl ASM_NAME(synth_1to1_sse_s)
	/* .type	ASM_NAME(synth_1to1_sse_s), @function */
ASM_NAME(synth_1to1_sse_s):
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
#APP
	movl 12(%ebp),%ecx
	movl 16(%ebp),%edi
	movl $15,%ebx
	movl 24(%ebp),%edx
	leal (%edi,%ecx,2),%edi
	decl %ecx
	movl 20(%ebp),%esi
	movl (%edx),%eax
	jecxz .L01
	decl %eax
	andl %ebx,%eax
	leal 1088(%esi),%esi
	movl %eax,(%edx)
	.L01:
	leal (%esi,%eax,2),%edx
	movl %eax,temp
	incl %eax
	andl %ebx,%eax
	leal 544(%esi,%eax,2),%ecx
	incl %ebx
	testl $1, %eax
	jnz .L02
	xchgl %edx,%ecx
	incl temp
	leal 544(%esi),%esi
	.L02:
	emms
	pushl 8(%ebp)
	pushl %edx
	pushl %ecx
	call *ASM_NAME(mpl_dct64)
	addl $12, %esp
	leal 1(%ebx), %ecx
	subl temp,%ebx
	pushl %ecx
	leal ASM_NAME(decwins)(%ebx,%ebx,1), %edx
	shrl $1, %ecx
	.balign 16
	.L03:
	movq  (%edx),%mm0
	movq  64(%edx),%mm4
	pmaddwd (%esi),%mm0
	pmaddwd 32(%esi),%mm4
	movq  8(%edx),%mm1
	movq  72(%edx),%mm5
	pmaddwd 8(%esi),%mm1
	pmaddwd 40(%esi),%mm5
	movq  16(%edx),%mm2
	movq  80(%edx),%mm6
	pmaddwd 16(%esi),%mm2
	pmaddwd 48(%esi),%mm6
	movq  24(%edx),%mm3
	movq  88(%edx),%mm7
	pmaddwd 24(%esi),%mm3
	pmaddwd 56(%esi),%mm7
	paddd %mm1,%mm0
	paddd %mm5,%mm4
	paddd %mm2,%mm0
	paddd %mm6,%mm4
	paddd %mm3,%mm0
	paddd %mm7,%mm4
	movq  %mm0,%mm1
	movq  %mm4,%mm5
	psrlq $32,%mm1
	psrlq $32,%mm5
	paddd %mm1,%mm0
	paddd %mm5,%mm4
	psrad $13,%mm0
	psrad $13,%mm4
	packssdw %mm0,%mm0
	packssdw %mm4,%mm4
	movq	(%edi), %mm1
	punpckldq %mm4, %mm0
	pand   one_null, %mm1
	pand   null_one, %mm0
	por    %mm0, %mm1
	movq   %mm1,(%edi)
	leal 64(%esi),%esi
	leal 128(%edx),%edx
	leal 8(%edi),%edi
	decl %ecx
	jnz  .L03
	popl %ecx
	andl $1, %ecx
	jecxz .next_loop
	movq  (%edx),%mm0
	pmaddwd (%esi),%mm0
	movq  8(%edx),%mm1
	pmaddwd 8(%esi),%mm1
	movq  16(%edx),%mm2
	pmaddwd 16(%esi),%mm2
	movq  24(%edx),%mm3
	pmaddwd 24(%esi),%mm3
	paddd %mm1,%mm0
	paddd %mm2,%mm0
	paddd %mm3,%mm0
	movq  %mm0,%mm1
	psrlq $32,%mm1
	paddd %mm1,%mm0
	psrad $13,%mm0
	packssdw %mm0,%mm0
	movd %mm0,%eax
	movw %ax, (%edi)
	leal 32(%esi),%esi
	leal 64(%edx),%edx
	leal 4(%edi),%edi
	.next_loop:
	subl $64,%esi
	movl $7,%ecx
	.balign 16
	.L04:
	movq  (%edx),%mm0
	movq  64(%edx),%mm4
	pmaddwd (%esi),%mm0
	pmaddwd -32(%esi),%mm4
	movq  8(%edx),%mm1
	movq  72(%edx),%mm5
	pmaddwd 8(%esi),%mm1
	pmaddwd -24(%esi),%mm5
	movq  16(%edx),%mm2
	movq  80(%edx),%mm6
	pmaddwd 16(%esi),%mm2
	pmaddwd -16(%esi),%mm6
	movq  24(%edx),%mm3
	movq  88(%edx),%mm7
	pmaddwd 24(%esi),%mm3
	pmaddwd -8(%esi),%mm7
	paddd %mm1,%mm0
	paddd %mm5,%mm4
	paddd %mm2,%mm0
	paddd %mm6,%mm4
	paddd %mm3,%mm0
	paddd %mm7,%mm4
	movq  %mm0,%mm1
	movq  %mm4,%mm5
	psrlq $32,%mm1
	psrlq $32,%mm5
	paddd %mm0,%mm1
	paddd %mm4,%mm5
	psrad $13,%mm1
	psrad $13,%mm5
	packssdw %mm1,%mm1
	packssdw %mm5,%mm5
	psubd %mm0,%mm0
	psubd %mm4,%mm4
	psubsw %mm1,%mm0
	psubsw %mm5,%mm4
	movq	(%edi), %mm1
	punpckldq %mm4, %mm0
	pand   one_null, %mm1
	pand   null_one, %mm0
	por    %mm0, %mm1
	movq   %mm1,(%edi)
	subl $64,%esi
	addl $128,%edx
	leal 8(%edi),%edi
	decl %ecx
	jnz  .L04
	movq  (%edx),%mm0
	pmaddwd (%esi),%mm0
	movq  8(%edx),%mm1
	pmaddwd 8(%esi),%mm1
	movq  16(%edx),%mm2
	pmaddwd 16(%esi),%mm2
	movq  24(%edx),%mm3
	pmaddwd 24(%esi),%mm3
	paddd %mm1,%mm0
	paddd %mm2,%mm0
	paddd %mm3,%mm0
	movq  %mm0,%mm1
	psrlq $32,%mm1
	paddd %mm0,%mm1
	psrad $13,%mm1
	packssdw %mm1,%mm1
	psubd %mm0,%mm0
	psubsw %mm1,%mm0
	movd %mm0,%eax
	movw %ax,(%edi)
	emms
	
#NO_APP
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	/* .size	ASM_NAME(synth_1to1_sse_s), .-ASM_NAME(synth_1to1_sse_s) */
