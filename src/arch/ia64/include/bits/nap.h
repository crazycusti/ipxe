#ifndef _BITS_NAP_H
#define _BITS_NAP_H
FILE_LICENCE ( GPL2_OR_LATER );
extern void ia64_cpu_halt ( void );
static inline __attribute__((always_inline)) void cpu_halt ( void ) {
 ia64_cpu_halt();
}
#endif
