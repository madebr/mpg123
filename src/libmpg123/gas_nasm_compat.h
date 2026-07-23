#ifdef NASM_ASSEMBLER
#define DATA_LONG dd
#define DATA_SHORT dw
#define DATA_BYTE db
#define SECTION section
#define CPU_686
#define MMX
#define XMM
#define FLAT_MODEL
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
default rel
#elif defined(MASM_ASSEMBLER)
option casemap:none
#define DATA_LONG dd
#define DATA_SHORT dw
#define DATA_BYTE db
#define SECTION section
#if defined(_IM_X86)
#define CPU_686 .686
#else
#define CPU_686
#endif
#define MMX .mmx
#define XMM .xmm
#define FLAT_MODEL .model flat
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

#else
#define DATA_LONG .long
#define DATA_SHORT .short
#define DATA_BYTE .byte
#define SECTION .section
#define CPU_686
#define MMX
#define XMM
#define FLAT_MODEL
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

.intel_syntax noprefix
#endif
