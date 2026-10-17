FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <byteswap.h>
static volatile unsigned char bit_lock;
int arch_bitop ( unsigned int bit, volatile void *bits, int set )
{
	volatile uint32_t *word = (volatile uint32_t *)bits + bit / 32;
	uint32_t mask = cpu_to_le32 ( 1U << ( bit % 32 ) );
	uint32_t old;
	unsigned int held;
	do {
		__asm__ volatile ( "ldstub [%1], %0"
				   : "=r"( held )
				   : "r"( &bit_lock )
				   : "memory" );
	} while ( held );
	old = *word;
	*word = set ? old | mask : old & ~mask;
	__asm__ volatile ( "stbar" ::: "memory" );
	bit_lock = 0;
	return !!( old & mask );
}
