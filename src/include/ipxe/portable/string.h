#ifndef _PORTABLE_BITS_STRING_H
#define _PORTABLE_BITS_STRING_H
FILE_LICENCE ( GPL2_OR_LATER );
static inline __attribute__ ( ( always_inline ) ) void *
memcpy ( void *d, const void *s, size_t n )
{
	return generic_memcpy ( d, s, n );
}
static inline __attribute__ ( ( always_inline ) ) void *
memmove ( void *d, const void *s, size_t n )
{
	return generic_memmove ( d, s, n );
}
static inline __attribute__ ( ( always_inline ) ) void *memset ( void *d, int c,
								 size_t n )
{
	return generic_memset ( d, c, n );
}
#endif
