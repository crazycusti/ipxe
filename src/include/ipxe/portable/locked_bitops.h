#ifndef _PORTABLE_LOCKED_BITOPS_H
#define _PORTABLE_LOCKED_BITOPS_H
FILE_LICENCE ( GPL2_OR_LATER );
/* One architecture-wide lock protects little-endian bitfields on CPUs
 * without native 32-bit atomic OR/AND instructions.
 */
extern int arch_bitop ( unsigned int bit, volatile void *bits, int set );
static inline int test_and_set_bit ( unsigned int bit, volatile void *bits )
{
	return arch_bitop ( bit, bits, 1 );
}
static inline int test_and_clear_bit ( unsigned int bit, volatile void *bits )
{
	return arch_bitop ( bit, bits, 0 );
}
static inline void set_bit ( unsigned int bit, volatile void *bits )
{
	arch_bitop ( bit, bits, 1 );
}
static inline void clear_bit ( unsigned int bit, volatile void *bits )
{
	arch_bitop ( bit, bits, 0 );
}
#endif
