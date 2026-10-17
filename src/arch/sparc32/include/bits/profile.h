#ifndef _SPARC32_BITS_PROFILE_H
#define _SPARC32_BITS_PROFILE_H
FILE_LICENCE ( GPL2_OR_LATER );
extern unsigned long openprom_profile_timestamp ( void );
static inline __attribute__ ( ( always_inline ) ) unsigned long
profile_timestamp ( void )
{
	return openprom_profile_timestamp ();
}
#endif
