#ifndef _BITS_PROFILE_H
#define _BITS_PROFILE_H
FILE_LICENCE ( GPL2_OR_LATER );
static inline __attribute__((always_inline)) unsigned long profile_timestamp ( void ) {
 unsigned long ticks;
 __asm__ __volatile__ ( "mov %0=ar.itc" : "=r" ( ticks ) );
 return ticks;
}
#endif
