#ifndef _PORTABLE_BITS_BITOPS_H
#define _PORTABLE_BITS_BITOPS_H
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <byteswap.h>
static inline int test_and_set_bit ( unsigned int b, volatile void *p )
{
	volatile uint32_t *w = (volatile uint32_t *)p + b / 32;
	uint32_t m = cpu_to_le32 ( 1U << ( b % 32 ) );
	return !!( __atomic_fetch_or ( w, m, __ATOMIC_SEQ_CST ) & m );
}
static inline int test_and_clear_bit ( unsigned int b, volatile void *p )
{
	volatile uint32_t *w = (volatile uint32_t *)p + b / 32;
	uint32_t m = cpu_to_le32 ( 1U << ( b % 32 ) );
	return !!( __atomic_fetch_and ( w, ~m, __ATOMIC_SEQ_CST ) & m );
}
static inline void set_bit ( unsigned int b, volatile void *p )
{
	test_and_set_bit ( b, p );
}
static inline void clear_bit ( unsigned int b, volatile void *p )
{
	test_and_clear_bit ( b, p );
}
#endif
