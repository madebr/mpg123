#ifdef NASM_ASSEMBLER
#define DATA_LONG dd
#define DATA_SHORT dw
#define DATA_BYTE db
#define SECTION section
#define LEA(REG, DATA) lea REG, DWORD_PTR [DATA]
#define SECTION_TEXT SECTION .text
#define ALIGN align
#define GLOBAL global
#define RIP_REL
#define RIP_REL_F(ADDR) [rel ADDR]
#define RIP_REL_ADD_F(ADDR, ADD) [rel ADDR + ADD]
#define COMMENT ;
#define END_MODULE
#define HEX(V) 0x##V
#define WORD_PTR
#define DWORD_PTR
#define QWORD_PTR
#define XMMWORD_PTR
#define YMMWORD_PTR
#define PTR_NONASM
#define EXTERN(NAME) extern NAME
#define ALIGN_CODE(NUM) ALIGN NUM
default rel
#elif defined(MASM_ASSEMBLER)
option casemap:none
#ifdef ARCH_X86
.386
.486
.586
.686
#ifdef OPT_MMX
.mmx
#endif
#ifdef OPT_SSE
.xmm
#endif
.model flat
#endif
#define DATA_LONG dd
#define DATA_SHORT dw
#define DATA_BYTE db
#define SECTION section
#define LEA(REG, DATA) mov REG, OFFSET DATA
#define SECTION_TEXT .code
#define ALIGN align
#define GLOBAL PUBLIC
#define RIP_REL
#define RIP_REL_F(ADDR) [ADDR]
#define RIP_REL_ADD_F(ADDR, ADD) [ADDR + ADD]
#define COMMENT ;
#define END_MODULE end
#define HEX(V) 0##V##h
#define WORD_PTR word ptr
#define DWORD_PTR dword ptr
#define QWORD_PTR qword ptr
#define XMMWORD_PTR xmmword ptr
#define YMMWORD_PTR ymmword ptr
#define PTR_NONASM ptr
#define EXTERN(NAME) extern NAME:BYTE

// FIXME: aligning code does not work
#define ALIGN_CODE(NUM)

#else
#define DATA_LONG .long
#define DATA_SHORT .short
#define DATA_BYTE .byte
#define SECTION .section
#define LEA(REG, DATA) lea REG, DWORD_PTR [DATA]
#define SECTION_TEXT .text
#define ALIGN .balign
#define GLOBAL .globl
#define RIP_REL [rip]
#define RIP_REL_F(ADDR) [ADDR+rip]
#define RIP_REL_ADD_F(ADDR, ADD) [ADDR+rip+ADD]
#define COMMENT #
#define END_MODULE
#define HEX(V) 0x##V
#define WORD_PTR word ptr
#define DWORD_PTR dword ptr
#define QWORD_PTR qword ptr
#define XMMWORD_PTR xmmword ptr
#define YMMWORD_PTR ymmword ptr
#define PTR_NONASM ptr
#define EXTERN(NAME)
#define ALIGN_CODE(NUM) ALIGN NUM

.intel_syntax noprefix
#endif
