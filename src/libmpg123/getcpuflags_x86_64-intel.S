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

	mov	DWORD_PTR 12[rdi], 0
	mov	DWORD_PTR 16[rdi], 0

	mov		eax, 0x80000000
	cpuid
	cmp		eax, 0x80000001
	jb		cpu_features
	mov		eax, 0x80000001
	cpuid
	mov	12[rdi], edx
cpu_features:
	mov		eax, 0x00000001
	cpuid
	mov	[rdi], eax
	mov	4[rdi], ecx
	mov	8[rdi], edx
	test	ecx, 0x04000000
	jz		end
	test	ecx, 0x08000000
	jz		end
	xor		ecx, ecx
	DATA_BYTE	0x0f, 0x01, 0xd0 /* xgetbv instruction */
	mov	16[rdi], eax
	mov	eax, [rdi]
end:
#ifdef IS_MSABI
	pop		rdi
#endif
	pop		rbx
	mov		rsp, rbp
	pop		rbp
	ret
	
NONEXEC_STACK
