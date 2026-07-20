#ifdef NASM_ASSEMBLER
#define DATA_LONG dd
#define DATA_SHORT dw
#define DATA_BYTE db
#else
#define DATA_LONG .long
#define DATA_SHORT .short
#define DATA_BYTE .byte
.intel_syntax noprefix
#endif
