#ifndef _PORTABLE_BITS_STRINGS_H
#define _PORTABLE_BITS_STRINGS_H
FILE_LICENCE ( GPL2_OR_LATER );
static inline __attribute__ ( ( always_inline ) ) int __ffsl ( long x )
{
	return __builtin_ffsl ( x );
}
static inline __attribute__ ( ( always_inline ) ) int __ffsll ( long long x )
{
	return __builtin_ffsll ( x );
}
static inline __attribute__ ( ( always_inline ) ) int __flsl ( long x )
{
	return x ? ( 8 * sizeof ( long ) ) - __builtin_clzl ( (unsigned long)x )
		 : 0;
}
static inline __attribute__ ( ( always_inline ) ) int __flsll ( long long x )
{
	return x ? 64 - __builtin_clzll ( (unsigned long long)x ) : 0;
}
#endif
