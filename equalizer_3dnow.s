/
/ equalizer_3dnow.s - 3DNow! optimized do_equalizer()
/ by KIMURA Takuhiro <kim@hannah.ipc.miyakyo-u.ac.jp>
/                    <kim@comtec.co.jp>
/
	
.text
        .align 4
.globl do_equalizer_3dnow
	.type	 do_equalizer_3dnow,@function
do_equalizer_3dnow:
	pushl %esi
	pushl %ebx
	/ bandPtr
	movl 12(%esp),%ebx
	cmpl $0,equalfile
	je .L5
	/ channel
	movl 16(%esp),%ecx
	xorl %edx,%edx
	movl $equalizer,%esi
	sall $7,%ecx
        .align 4
.L9:
	movq (%ebx,%edx),%mm0
	pfmul (%esi,%ecx),%mm0

	movq 8(%ebx,%edx),%mm1
	pfmul 8(%esi,%ecx),%mm1
	movq %mm0,(%ebx,%edx)
	
	movq 16(%ebx,%edx),%mm0
	pfmul 16(%esi,%ecx),%mm0
	movq %mm1,8(%ebx,%edx)
	
	movq 24(%ebx,%edx),%mm1
	pfmul 24(%esi,%ecx),%mm1
	movq %mm0,16(%ebx,%edx)

	movq 32(%ebx,%edx),%mm0
	pfmul 32(%esi,%ecx),%mm0
	movq %mm1,24(%ebx,%edx)

	movq 40(%ebx,%edx),%mm1
	pfmul 40(%esi,%ecx),%mm1
	movq %mm0,32(%ebx,%edx)
	
	movq 48(%ebx,%edx),%mm0
	pfmul 48(%esi,%ecx),%mm0
	movq %mm1,40(%ebx,%edx)
	
	movq 56(%ebx,%edx),%mm1
	pfmul 56(%esi,%ecx),%mm1
	movq %mm0,48(%ebx,%edx)
	movq %mm1,56(%ebx,%edx)
	
	addl $64,%edx
	addl $32,%ecx
	cmpl $124,%edx
	jle .L9
	.align 4
.L5:
	popl %ebx
	popl %esi
	ret
