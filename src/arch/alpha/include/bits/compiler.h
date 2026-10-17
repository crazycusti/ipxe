#ifndef _ALPHA_BITS_COMPILER_H
#define _ALPHA_BITS_COMPILER_H
FILE_LICENCE ( GPL2_OR_LATER );
#define RELOC_TYPE_NONE BFD_RELOC_NONE
#undef ASM_EQU
#define ASM_EQU .equ
#ifndef ASSEMBLY
#define ASM_NO_PREFIX ""
#define __asmcall
#define __libgcc
#endif
#endif
