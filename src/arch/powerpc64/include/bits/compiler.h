#ifndef _PPC64_BITS_COMPILER_H
#define _PPC64_BITS_COMPILER_H
FILE_LICENCE ( GPL2_OR_LATER );
#define RELOC_TYPE_NONE R_PPC64_NONE
#if _CALL_ELF == 1
/* R_PPC64_NONE at a function descriptor makes GNU ld reject the .opd
 * descriptor array.  Keep dependency relocations in a retained table instead. */
#undef REQUIRING_SYMBOL
#define REQUIRING_SYMBOL( symbol ) PROVIDE_REQUIRING_SYMBOL()
#endif
#ifndef ASSEMBLY
#define ASM_NO_PREFIX ""
#define __asmcall
#define __libgcc
#endif
#endif
