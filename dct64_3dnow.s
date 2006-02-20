/ 3D-Now DCT64. 
/
/ Copyrights 1999 by Michael Hipp
/
/ Not really optimized. Just using 3dnow instead of
/ standard FPU instructions enhances performance a lot.
/

.text

.globl dct64

	.align 4
	.type	 dct64,@function

dct64:
	pushl %ebp
	movl %esp,%ebp
	subl $256,%esp 	/ tmp-buff
	pushl %ebx
	pushl %edi
	movl 16(%ebp),%eax

	femms

	leal -128(%ebp),%ecx
	leal -256(%ebp),%edx

	movl pnts,%ebx

	movd 124(%eax),%mm1
	movd 120(%eax),%mm2
	movq    (%eax),%mm0
	psllq      $32,%mm2
        movd 116(%eax),%mm5
	pfadd     %mm2,%mm1
        movd 112(%eax),%mm6
	pfadd     %mm0,%mm1
        movq   8(%eax),%mm4
        psllq      $32,%mm6
	movq      %mm1,%mm3
        pfadd     %mm6,%mm5
        movq      %mm1,(%edx)
        movq      %mm5,%mm7
	movq    (%ebx),%mm2
	pfsub     %mm3,%mm0
        movq      %mm5,8(%edx)
        pfadd     %mm4,%mm5
        movq   8(%ebx),%mm6
	pfmul     %mm2,%mm0
        pfsub     %mm7,%mm4

	movd %mm0,124(%edx)
        movd 108(%eax),%mm1
        pfmul     %mm6,%mm4
	psrlq $32,%mm0
        movd 104(%eax),%mm2
	movd %mm0,120(%edx)
        movd %mm4,116(%edx)
        movq  16(%eax),%mm0
        psrlq $32,%mm4
        psllq      $32,%mm2
        movd %mm4,112(%edx)

        movd 100(%eax),%mm5
        pfadd     %mm2,%mm1
        movd  96(%eax),%mm6
        movq      %mm1,%mm3
        movq  24(%eax),%mm4
        pfadd     %mm0,%mm1
        psllq      $32,%mm6
        movq      %mm1,16(%edx)
        pfadd     %mm6,%mm5

        pfsub     %mm3,%mm0
        movq      %mm5,%mm7
        movq  16(%ebx),%mm2
        pfadd     %mm4,%mm5
        pfmul     %mm2,%mm0
        movq      %mm5,24(%edx)

        movd %mm0,108(%edx)
        pfsub     %mm7,%mm4
        psrlq $32,%mm0
        movq  24(%ebx),%mm6
        movd %mm0,104(%edx)
        pfmul     %mm6,%mm4

        movd  92(%eax),%mm1
        movd %mm4,100(%edx)
        movd  88(%eax),%mm2
        psrlq $32,%mm4
        movq  32(%eax),%mm0
        psllq      $32,%mm2
        movd %mm4,96(%edx)
        pfadd     %mm2,%mm1

        movd  84(%eax),%mm5
        movq      %mm1,%mm3
        movd  80(%eax),%mm6
        pfadd     %mm0,%mm1
        movq  40(%eax),%mm4
        movq      %mm1,32(%edx)
        psllq      $32,%mm6

        pfsub     %mm3,%mm0
        movq  32(%ebx),%mm2
        pfadd     %mm6,%mm5
        pfmul     %mm2,%mm0
        movq      %mm5,%mm7

        movd %mm0,92(%edx)
        pfadd     %mm4,%mm5
        psrlq $32,%mm0
        movq      %mm5,40(%edx)
        pfsub     %mm7,%mm4
        movd %mm0,88(%edx)
        movq  40(%ebx),%mm6

        movd  76(%eax),%mm1
        pfmul     %mm6,%mm4
        movd  72(%eax),%mm2

        movd %mm4,84(%edx)
        movq  48(%eax),%mm0
        psrlq $32,%mm4
        psllq      $32,%mm2
        movd %mm4,80(%edx)
        pfadd     %mm2,%mm1

        movd  68(%eax),%mm5
        movq      %mm1,%mm3
        movd  64(%eax),%mm6
        pfadd     %mm0,%mm1
        movq  56(%eax),%mm4
        movq      %mm1,48(%edx)
        psllq      $32,%mm6

        pfsub     %mm3,%mm0
        movq  48(%ebx),%mm2
        pfadd     %mm6,%mm5
        pfmul     %mm2,%mm0
        movq      %mm5,%mm7

        movd %mm0,76(%edx)
        pfadd     %mm4,%mm5
        psrlq $32,%mm0
        pfsub     %mm7,%mm4
        movq      %mm5,56(%edx)
        movq  56(%ebx),%mm6
        movd %mm0,72(%edx)

        / 8

        pfmul     %mm6,%mm4
	movl $2,%eax
        movd %mm4,68(%edx)
        psrlq $32,%mm4
	movl pnts+4,%ebx
        movd %mm4,64(%edx)

/ end part 1


part2:
        / 1
        movd  60(%edx),%mm1
        movd  56(%edx),%mm2
        movq    (%edx),%mm0
        psllq      $32,%mm2
        movd  52(%edx),%mm5
        pfadd     %mm2,%mm1
        movd  48(%edx),%mm6
        movq      %mm1,%mm3
        movq   8(%edx),%mm4
        pfadd     %mm0,%mm1
        psllq      $32,%mm6
        movq      %mm1,(%ecx)

        pfadd     %mm6,%mm5
        pfsub     %mm3,%mm0
        movq      %mm5,%mm7
        movq    (%ebx),%mm2
        pfadd     %mm4,%mm5
        pfmul     %mm2,%mm0

        movq      %mm5,8(%ecx)
        movd %mm0,60(%ecx)
        pfsub     %mm7,%mm4
        movq   8(%ebx),%mm6
        psrlq $32,%mm0
        movd  44(%edx),%mm1
        movd %mm0,56(%ecx)
        pfmul     %mm6,%mm4

        movd  40(%edx),%mm2
        movd %mm4,52(%ecx)
        movq  16(%edx),%mm0
        psrlq $32,%mm4
        psllq      $32,%mm2
        movd %mm4,48(%ecx)
        pfadd     %mm2,%mm1
        movd  36(%edx),%mm5

        movq      %mm1,%mm3
        movd  32(%edx),%mm6
        pfadd     %mm0,%mm1
        movq  24(%edx),%mm4
        psllq      $32,%mm6
        movq      %mm1,16(%ecx)

        pfadd     %mm6,%mm5
        pfsub     %mm3,%mm0
        movq      %mm5,%mm7
        pfadd     %mm4,%mm5
        movq  16(%ebx),%mm2
        movq      %mm5,24(%ecx)
        pfmul     %mm2,%mm0

        pfsub     %mm7,%mm4
        movq  24(%ebx),%mm6
        movd %mm0,44(%ecx)
        pfmul     %mm6,%mm4
        psrlq $32,%mm0
        movd %mm4,36(%ecx)
        movd %mm0,40(%ecx)
        psrlq $32,%mm4

	addl $64,%edx
        movd %mm4,32(%ecx)

	addl $64,%ecx
	addl $32,%ebx

	decl %eax
	jnz part2

	addl $-128,%ecx
	addl $-128,%edx

        movl $2,%eax
        movl pnts+8,%ebx
part3:
        / 1
        movd  28(%ecx),%mm1
        movd  24(%ecx),%mm2
        movq    (%ecx),%mm0
        psllq      $32,%mm2
        pfadd     %mm2,%mm1
        movq      %mm1,%mm3
        pfadd     %mm0,%mm1
        movq      %mm1,(%edx)

        pfsub     %mm3,%mm0
        movq    (%ebx),%mm2
        pfmul     %mm2,%mm0

        movd %mm0,28(%edx)
        psrlq $32,%mm0
        movd %mm0,24(%edx)

        / 1 + 32
        movd  60(%ecx),%mm1
        movd  56(%ecx),%mm2
        movq    32(%ecx),%mm0
        psllq      $32,%mm2
        pfadd     %mm2,%mm1
        movq      %mm1,%mm3
        pfadd     %mm0,%mm1
        movq      %mm1,32(%edx)

        pfsub     %mm3,%mm0
        movq   16(%ebx),%mm2
        pfmul     %mm2,%mm0

        movd %mm0,60(%edx)
        psrlq $32,%mm0
        movd %mm0,56(%edx)

        / 2
        movd  20(%ecx),%mm1
        movd  16(%ecx),%mm2
        movq   8(%ecx),%mm0
        psllq      $32,%mm2
        pfadd     %mm2,%mm1
        movq      %mm1,%mm3
        pfadd     %mm0,%mm1
        movq      %mm1,8(%edx)

        pfsub     %mm3,%mm0
        movq   8(%ebx),%mm2
        pfmul     %mm2,%mm0

        movd %mm0,20(%edx)
        psrlq $32,%mm0
        movd %mm0,16(%edx)

        / 2 + 32
        movd  52(%ecx),%mm1
        movd  48(%ecx),%mm2
        movq   40(%ecx),%mm0
        psllq      $32,%mm2
        pfadd     %mm2,%mm1
        movq      %mm1,%mm3
        pfadd     %mm0,%mm1
        movq      %mm1,40(%edx)

        pfsub     %mm3,%mm0
        movq   24(%ebx),%mm2
        pfmul     %mm2,%mm0

        movd %mm0,52(%edx)
        psrlq $32,%mm0
        movd %mm0,48(%edx)

        addl $64,%ecx
        addl $64,%edx
        decl %eax
        jnz part3

        addl $-128,%ecx
        addl $-128,%edx

        movl $8,%eax
        movl pnts+12,%ebx
	movq (%ebx),%mm4

part4:
        / 1
        movd  12(%edx),%mm1
        movd  8(%edx),%mm2
        movq    (%edx),%mm0
        psllq      $32,%mm2
        pfadd     %mm2,%mm1
        movq      %mm1,%mm3
        pfadd     %mm0,%mm1
        movq      %mm1,(%ecx)

        pfsub     %mm3,%mm0
        pfmul     %mm4,%mm0

        movd %mm0,12(%ecx)
        psrlq $32,%mm0
        movd %mm0,8(%ecx)

        addl $16,%ecx
        addl $16,%edx

	movq %mm4,%mm0
	pfsub %mm4,%mm4
	pfsub %mm0,%mm4

        decl %eax
        jnz part4

        addl $-128,%ecx
        addl $-128,%edx

/ part 5

        movl $4,%eax

        movl pnts+16,%ebx
        movd (%ebx),%mm0
	movd 4(%ebx),%mm1
	movq %mm1,%mm4
	psllq $32,%mm4
	pfadd %mm0,%mm4

	movq %mm0,%mm5
        psllq $32,%mm5
        pfadd %mm1,%mm5

loop5: 

        movq 8(%ecx),%mm0
	movq (%ecx),%mm2
        movq %mm0,%mm1
	movq %mm2,%mm3
        pfmul %mm5,%mm1
	pfmul %mm4,%mm3
        pfacc %mm1,%mm0
	movq %mm0,%mm1
	pfacc %mm3,%mm2
	psrlq $32,%mm1
	movq %mm2,(%edx)
        movd 24(%ecx),%mm2
	pfadd %mm1,%mm0
        movd 28(%ecx),%mm3
	movq %mm0,8(%edx)

        movq %mm2,%mm6
        movd 16(%ecx),%mm0
        pfadd %mm3,%mm6   / 6
        movd 20(%ecx),%mm1
        pfsub %mm2,%mm3
        movq %mm0,%mm7

        pfmul %mm4,%mm3
        pfadd %mm1,%mm7
        movd %mm3,28(%edx) / 7

        pfsub %mm1,%mm0   / 5 = 4 - 5
        pfadd %mm3,%mm6   / 6 += 7

        pfmul %mm4,%mm0   
	pfadd %mm6,%mm7   / 4 += 6
        movd %mm7,16(%edx) 

	pfadd %mm0,%mm6   / 6 += 5
        addl $32,%ecx
        movd %mm6,24(%edx)
	
	pfadd %mm3,%mm0   / 5 += 7
        movd %mm0,20(%edx)

        addl $32,%edx
	decl %eax
	jnz loop5

	addl $-128,%edx

        movl 8(%ebp),%ebx / out0
        movl 12(%ebp),%edi / out1

	subl %ebx,%edi	/ allows ebx 16 bit relative addressing 
			/ maybe only ´as´ need this

	movl (%edx),%eax
	movl %eax,0x40*16(%ebx)
        movl 4*4(%edx),%eax
        movl %eax,0x40*12(%ebx)
        movl 4*2(%edx),%eax
        movl %eax,0x40*8(%ebx)
        movl 4*6(%edx),%eax
        movl %eax,0x40*4(%ebx)
        movl 4(%edx),%eax
        movl %eax,(%ebx)
        movl %eax,(%ebx,%edi)
        movl 4*5(%edx),%eax
        movl %eax,0x40*4(%ebx,%edi)
        movl 4*3(%edx),%eax
        movl %eax,0x40*8(%ebx,%edi)
        movl 4*7(%edx),%eax
        movl %eax,0x40*12(%ebx,%edi)

	movd 32(%edx),%mm0 / 8
	movd 48(%edx),%mm1 / C
	pfadd %mm1,%mm0
	movd %mm0,4*0xe0(%ebx)	
	movd 40(%edx),%mm0
	pfadd %mm0,%mm1
        movd %mm1,4*0xa0(%ebx)
	movd 56(%edx),%mm1
	pfadd %mm1,%mm0
	movd %mm0,4*0x60(%ebx)	

	movd 36(%edx),%mm0
	pfadd %mm0,%mm1
        movd %mm1,4*0x20(%ebx)
        movd 52(%edx),%mm1
        pfadd %mm1,%mm0
        movd %mm0,4*0x20(%ebx,%edi)
        movd 44(%edx),%mm0
        pfadd %mm0,%mm1
        movd %mm1,4*0x60(%ebx,%edi)

        movd 60(%edx),%mm1
        pfadd %mm1,%mm0
        movd %mm0,4*0xa0(%ebx,%edi)
        movd %mm1,4*0xe0(%ebx,%edi)

	////

	movq 4*0x10(%edx),%mm2
	movq 4*0x18(%edx),%mm0

	movq 4*0x1c(%edx),%mm1
	pfadd %mm1,%mm0
	pfadd %mm0,%mm2
	movd %mm2,4*0x10*15(%ebx)
	psrlq $32,%mm2
	movd %mm2,4*0x10*1(%ebx,%edi)
	movq 4*0x14(%edx),%mm2
	pfadd %mm2,%mm0
	movd %mm0,4*0x10*13(%ebx)
	psrlq $32,%mm0
	movd %mm0,4*0x10*3(%ebx,%edi)

	movq 4*0x1a(%edx),%mm0
	pfadd %mm0,%mm1
	pfadd %mm1,%mm2
	movd %mm2,4*0x10*11(%ebx)
	psrlq $32,%mm2
	movd %mm2,4*0x10*5(%ebx,%edi)
	movq 4*0x12(%edx),%mm2
	pfadd %mm2,%mm1
	movd %mm1,4*0x10*9(%ebx)
	psrlq $32,%mm1
	movd %mm1,4*0x10*7(%ebx,%edi)

        movq 4*0x1e(%edx),%mm1
        pfadd %mm1,%mm0
        pfadd %mm0,%mm2
        movd %mm2,4*0x10*7(%ebx)
        psrlq $32,%mm2
        movd %mm2,4*0x10*9(%ebx,%edi)
        movq 4*0x16(%edx),%mm2
        pfadd %mm2,%mm0
        movd %mm0,4*0x10*5(%ebx)
        psrlq $32,%mm0
        movd %mm0,4*0x10*11(%ebx,%edi)

	movd 4*0x19(%edx),%mm0
	pfadd %mm0,%mm1
	pfadd %mm1,%mm2
	movd %mm2,4*0x10*3(%ebx)
	psrlq $32,%mm2
	movd %mm2,4*0x10*13(%ebx,%edi)
	movd 4*0x11(%edx),%mm2
	pfadd %mm2,%mm1

        movd %mm1,4*0x10*1(%ebx)
        psrlq $32,%mm1
        movd %mm1,4*0x10*15(%ebx,%edi)

	femms
        popl %edi
        popl %ebx
        movl %ebp,%esp
        popl %ebp
        ret

