#include "gas_nasm_compat.h"
/*
	getcpuflags_x86_64: get cpuflags for x86-64

	copyright 1995-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "x86_64_defs.h"
#include "mangle.h"

	SECTION_TEXT
	ALIGN4
	GLOBAL ASM_NAME(INT123_getcpuflags)
ASM_NAME(INT123_getcpuflags):
	push	rbp
	mov		rbp, rsp
	push	rbx
	
#ifdef IS_MSABI
	push	rdi
	mov		rdi, rcx
#endif
	mov	dword PTR_NONASM 12[rdi], 0
	mov	dword PTR_NONASM 16[rdi], 0

	mov		eax, HEX(80000000)
	cpuid
	cmp		eax, HEX(80000001)
	jb		cpu_features
	mov		eax, HEX(80000001)
	cpuid
	mov	12[rdi], edx
cpu_features:
	mov		eax, HEX(00000001)
	cpuid
	mov	[rdi], eax
	mov	4[rdi], ecx
	mov	8[rdi], edx
	test	ecx, HEX(04000000)
	jz		end_cpuflags
	test	ecx, HEX(08000000)
	jz		end_cpuflags
	xor		ecx, ecx
	DATA_BYTE	HEX(0f), HEX(01), HEX(d0) /* xgetbv instruction */
	mov	16[rdi], eax
	mov	eax, [rdi]
end_cpuflags:
#ifdef IS_MSABI
	pop		rdi
#endif
	pop		rbx
	mov		rsp, rbp
	pop		rbp
	ret
	
NONEXEC_STACK

END_MODULE
