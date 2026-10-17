#ifndef _BITS_SETJMP_H
#define _BITS_SETJMP_H
FILE_LICENCE ( GPL2_OR_LATER );
/* GNU-EFI/EDK2 IA-64 layout: 20 spill-format FP registers and 18 words. */
typedef struct { uint64_t fp[40]; uint64_t state[18]; }
 __attribute__((aligned(16))) jmp_buf[1];
#endif
