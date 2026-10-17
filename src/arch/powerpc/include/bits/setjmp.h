#ifndef _POWERPC_BITS_SETJMP_H
#define _POWERPC_BITS_SETJMP_H
FILE_LICENCE ( GPL2_OR_LATER );
typedef struct {
	unsigned long sp, lr, cr, regs[18];
} jmp_buf[1];
#endif
