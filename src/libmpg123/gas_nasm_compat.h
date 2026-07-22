#ifdef NASM_ASSEMBLER
#define DATA_LONG dd
#define DATA_SHORT dw
#define DATA_BYTE db
#define SECTION section
#define SECTION_TEXT SECTION .text
#define ALIGN align
#define GLOBAL global
#define RIP_REL
#define RIP_REL_F(ADDR) [rel ADDR]
#define RIP_REL_ADD_F(ADDR, ADD) [rel ADDR + ADD]
#define DWORD_PTR dword
#define QWORD_PTR qword
#define COMMENT ;
default rel

#else
#define DATA_LONG .long
#define DATA_SHORT .short
#define DATA_BYTE .byte
#define SECTION .section
#define SECTION_TEXT .text
#define ALIGN .balign
#define GLOBAL .globl
#define RIP_REL [rip]
#define RIP_REL_F(ADDR) ADDR[rip]
#define RIP_REL_ADD_F(ADDR, ADD) ADD+ADDR[rip]
#define DWORD_PTR dword ptr
#define QWORD_PTR qword ptr
#define COMMENT #

.intel_syntax noprefix
#endif
