#ifndef _SPARC64_BITS_SETJMP_H
#define _SPARC64_BITS_SETJMP_H
FILE_LICENCE ( GPL2_OR_LATER );
typedef struct {
	unsigned long sp, pc, regs[16];
} jmp_buf[1];
#endif
