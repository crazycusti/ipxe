#ifndef _SPARC32_BITS_SETJMP_H
#define _SPARC32_BITS_SETJMP_H
FILE_LICENCE ( GPL2_OR_LATER );
typedef struct {
	unsigned long sp, pc, locals[8], inputs[8];
} jmp_buf[1];
#endif
