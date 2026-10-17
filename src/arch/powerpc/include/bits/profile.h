#ifndef _POWERPC_BITS_PROFILE_H
#define _POWERPC_BITS_PROFILE_H
FILE_LICENCE ( GPL2_OR_LATER );
static inline __attribute__ ( ( always_inline ) ) unsigned long
profile_timestamp ( void )
{
	unsigned long t;
	__asm__ volatile ( "mftb %0" : "=r"( t ) );
	return t;
}
#endif
