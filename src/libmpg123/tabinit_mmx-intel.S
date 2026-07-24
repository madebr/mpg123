#include "gas_nasm_compat.h"
/*
	tabinit_mmx: make_decode_tables_mmx

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by the mysterious higway (apparently)

	See synth_mmx.S about license history.
*/

#include "mangle.h"

	RODATA
	ALIGN32
GLOBAL ASM_NAME(INT123_costab_mmxsse)
ASM_NAME(INT123_costab_mmxsse):
	DATA_LONG 1056974725
	DATA_LONG 1057056395
	DATA_LONG 1057223771
	DATA_LONG 1057485416
	DATA_LONG 1057855544
	DATA_LONG 1058356026
	DATA_LONG 1059019886
	DATA_LONG 1059897405
	DATA_LONG 1061067246
	DATA_LONG 1062657950
	DATA_LONG 1064892987
	DATA_LONG 1066774581
	DATA_LONG 1069414683
	DATA_LONG 1073984175
	DATA_LONG 1079645762
	DATA_LONG 1092815430
	DATA_LONG 1057005197
	DATA_LONG 1057342072
	DATA_LONG 1058087743
	DATA_LONG 1059427869
	DATA_LONG 1061799040
	DATA_LONG 1065862217
	DATA_LONG 1071413542
	DATA_LONG 1084439708
	DATA_LONG 1057128951
	DATA_LONG 1058664893
	DATA_LONG 1063675095
	DATA_LONG 1076102863
	DATA_LONG 1057655764
	DATA_LONG 1067924853
	DATA_LONG 1060439283
	ALIGN32
intwinbase:
	DATA_SHORT      0,    -1,    -1,    -1,    -1,    -1,    -1,    -2
	DATA_SHORT     -2,    -2,    -2,    -3,    -3,    -4,    -4,    -5
	DATA_SHORT     -5,    -6,    -7,    -7,    -8,    -9,   -10,   -11
	DATA_SHORT    -13,   -14,   -16,   -17,   -19,   -21,   -24,   -26
	DATA_SHORT    -29,   -31,   -35,   -38,   -41,   -45,   -49,   -53
	DATA_SHORT    -58,   -63,   -68,   -73,   -79,   -85,   -91,   -97
	DATA_SHORT   -104,  -111,  -117,  -125,  -132,  -139,  -147,  -154
	DATA_SHORT   -161,  -169,  -176,  -183,  -190,  -196,  -202,  -208
	DATA_SHORT   -213,  -218,  -222,  -225,  -227,  -228,  -228,  -227
	DATA_SHORT   -224,  -221,  -215,  -208,  -200,  -189,  -177,  -163
	DATA_SHORT   -146,  -127,  -106,   -83,   -57,   -29,     2,    36
	DATA_SHORT     72,   111,   153,   197,   244,   294,   347,   401
	DATA_SHORT    459,   519,   581,   645,   711,   779,   848,   919
	DATA_SHORT    991,  1064,  1137,  1210,  1283,  1356,  1428,  1498
	DATA_SHORT   1567,  1634,  1698,  1759,  1817,  1870,  1919,  1962
	DATA_SHORT   2001,  2032,  2057,  2075,  2085,  2087,  2080,  2063
	DATA_SHORT   2037,  2000,  1952,  1893,  1822,  1739,  1644,  1535
	DATA_SHORT   1414,  1280,  1131,   970,   794,   605,   402,   185
	DATA_SHORT    -45,  -288,  -545,  -814, -1095, -1388, -1692, -2006
	DATA_SHORT  -2330, -2663, -3004, -3351, -3705, -4063, -4425, -4788
	DATA_SHORT  -5153, -5517, -5879, -6237, -6589, -6935, -7271, -7597
	DATA_SHORT  -7910, -8209, -8491, -8755, -8998, -9219, -9416, -9585
	DATA_SHORT  -9727, -9838, -9916, -9959, -9966, -9935, -9863, -9750
	DATA_SHORT  -9592, -9389, -9139, -8840, -8492, -8092, -7640, -7134
	DATA_SHORT  -6574, -5959, -5288, -4561, -3776, -2935, -2037, -1082
	DATA_SHORT    -70,   998,  2122,  3300,  4533,  5818,  7154,  8540
	DATA_SHORT   9975, 11455, 12980, 14548, 16155, 17799, 19478, 21189
	DATA_SHORT  22929, 24694, 26482, 28289, 30112, 31947,-26209,-24360
	DATA_SHORT -22511,-20664,-18824,-16994,-15179,-13383,-11610, -9863
	DATA_SHORT  -8147, -6466, -4822, -3222, -1667,  -162,  1289,  2684
	DATA_SHORT   4019,  5290,  6494,  7629,  8692,  9679, 10590, 11420
	DATA_SHORT  12169, 12835, 13415, 13908, 14313, 14630, 14856, 14992
	DATA_SHORT  15038

intwindiv:
	DATA_LONG HEX(47800000)			COMMENT 65536.0

SECTION_TEXT
	ALIGN 16
/* void make_decode_tables_mmx_asm(long scaleval, float* decwin_mmx, float *decwins); */
GLOBAL ASM_NAME(INT123_make_decode_tables_mmx_asm)
ASM_NAME(INT123_make_decode_tables_mmx_asm):
	push ebp
	mov ebp,esp
	sub esp,12
	push edi
	push esi
	push ebx

	GET_GOT
	lea edi, WORD_PTR intwinbase
	mov ecx,[LOCAL_VAR(intwindiv)]
	mov [ebp-4],edi
	lea eax,[edi+444]
	mov [ebp-8],eax
	mov [ebp-12],ecx

/* stack: [ebp-12]=*intwindiv, [ebp-8]=intwinbase+444, [ebp-4]=intwinbase, ... ,
          [ebp+4]=back, [ebp+8]=scaleval, [ebp+12]=decwin_mmx, [ebp+16]=decwins */
#define INTWINDIV [ebp-12]
#define INTWINBASE_PLUS_444 [ebp-8]
#define INTWINBASE [ebp-4]
#define SCALEVAL [ebp+8]
#define DECWIN_MMX [ebp+12]
#define DECWINS [ebp+16]
	
	xor ecx,ecx
	xor ebx,ebx
	mov esi,32
	
	neg DWORD PTR_NONASM SCALEVAL	/* scaleval */
	push 2	/* intwinbase step */
label00:
	cmp ecx,528
	jnc label02
	movsx eax, word PTR_NONASM [edi]
	cmp edi,INTWINBASE_PLUS_444
	jc label01
	add eax,60000
label01:
	push eax
	fild DWORD PTR_NONASM [esp]
	fdiv DWORD PTR_NONASM INTWINDIV
	fimul DWORD PTR_NONASM SCALEVAL /* scaleval */
/* eax used to be popped the line before... I'll just use it here a bit */
	mov eax,DECWIN_MMX /* decwin_mmx */
	fst DWORD PTR_NONASM    [eax+ecx*4]
	fstp DWORD PTR_NONASM [eax+ecx*4+64]
	pop eax
label02:
	lea edx,DWORD_PTR [esi-1]
	and edx,ebx
	cmp edx,31
	jnz label03
	add ecx,-1023
	test ebx,esi
	jz  label03
	neg DWORD PTR_NONASM SCALEVAL
label03:
	add ecx,esi
	add edi,DWORD_PTR [esp]
	inc ebx
	cmp edi,INTWINBASE
	jz label04
	cmp ebx,256
	jnz label00
	neg DWORD PTR_NONASM [esp]
	jmp label00
label04:
	pop eax

	xor ecx,ecx
	xor ebx,ebx
	push 2 /* paired with popl above */
label05:
	cmp ecx,528
	jnc label11
	movsx eax, word PTR_NONASM [edi]
	cmp edi,INTWINBASE_PLUS_444
	jc label06
	add eax,60000
label06:
	cdq
	imul DWORD PTR_NONASM SCALEVAL
	shrd eax,edx,17
	cmp eax,32767
	mov edx,1055
	jle label07
	mov eax,32767
	jmp label08
label07:
	cmp eax,-32767
	jge label08
	mov eax,-32767
label08:
/* going to use ebx for decwins, watch the jumps */
	push ebx 
	mov ebx,DECWINS
	cmp ecx,512
	jnc label09
	sub edx,ecx
	mov WORD_PTR [ebx+edx*2],ax /* decwins */
	mov WORD_PTR [ebx+edx*2-32],ax
label09:
	test ecx,1
	jnz label10
	neg eax
label10:
	mov WORD_PTR [ebx+ecx*2],ax
	mov WORD_PTR [ebx+ecx*2+32],ax
	pop ebx /* that has to match the pushl before */
label11:
	lea edx,DWORD_PTR [esi-1]
	and edx,ebx
	cmp edx,31
	jnz label12
	add ecx,-1023
	test ebx,esi
	jz  label12
	neg DWORD PTR_NONASM SCALEVAL
label12:
	add ecx,esi
	add edi,DWORD_PTR [esp]
	inc ebx
	cmp edi,INTWINBASE
	jz label13
	cmp ebx,256
	jnz label05
	neg DWORD PTR_NONASM [esp]
	jmp label05
label13:
	pop eax
	
	pop ebx
	pop esi
	pop edi
	mov esp,ebp
	pop ebp
	ret

NONEXEC_STACK

END_MODULE
