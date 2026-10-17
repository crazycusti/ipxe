#ifndef _HPPA_BITS_SETJMP_H
#define _HPPA_BITS_SETJMP_H
FILE_LICENCE ( GPL2_OR_LATER );
/* Callee-saved GPRs r3..r18 plus r19, DP, SP, RP and space registers. */
typedef struct {
	unsigned long regs[20], spaces[8];
} jmp_buf[1];
#endif
