/* Scalar compiler helpers for an actual V8 CPU, without V9 instructions. */
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
union wide_word {
	uint64_t value;
	struct {
		uint32_t hi, lo;
	} word;
};
int __clzsi2 ( uint32_t value )
{
	int count = 0;
	while ( count < 32 && !( value & 0x80000000U ) ) {
		value <<= 1;
		count++;
	}
	return count;
}
int __clzdi2 ( uint64_t value )
{
	union wide_word v = { .value = value };
	return v.word.hi ? __clzsi2 ( v.word.hi ) : 32 + __clzsi2 ( v.word.lo );
}
int __ffssi2 ( uint32_t value )
{
	int count = 1;
	if ( !value )
		return 0;
	while ( !( value & 1 ) ) {
		value >>= 1;
		count++;
	}
	return count;
}
int __ffsdi2 ( uint64_t value )
{
	union wide_word v = { .value = value };
	return v.word.lo ? __ffssi2 ( v.word.lo )
			 : ( v.word.hi ? 32 + __ffssi2 ( v.word.hi ) : 0 );
}
uint64_t __ashldi3 ( uint64_t value, unsigned int count )
{
	union wide_word v = { .value = value };
	if ( count >= 64 ) {
		v.word.hi = v.word.lo = 0;
	} else if ( count >= 32 ) {
		v.word.hi = v.word.lo << ( count - 32 );
		v.word.lo = 0;
	} else if ( count ) {
		v.word.hi =
		    ( v.word.hi << count ) | ( v.word.lo >> ( 32 - count ) );
		v.word.lo <<= count;
	}
	return v.value;
}
uint64_t __lshrdi3 ( uint64_t value, unsigned int count )
{
	union wide_word v = { .value = value };
	if ( count >= 64 ) {
		v.word.hi = v.word.lo = 0;
	} else if ( count >= 32 ) {
		v.word.lo = v.word.hi >> ( count - 32 );
		v.word.hi = 0;
	} else if ( count ) {
		v.word.lo =
		    ( v.word.lo >> count ) | ( v.word.hi << ( 32 - count ) );
		v.word.hi >>= count;
	}
	return v.value;
}
int64_t __ashrdi3 ( int64_t value, unsigned int count )
{
	union wide_word v = { .value = value };
	int32_t hi = v.word.hi;
	if ( count >= 64 ) {
		v.word.hi = v.word.lo = hi >> 31;
	} else if ( count >= 32 ) {
		v.word.lo = hi >> ( count - 32 );
		v.word.hi = hi >> 31;
	} else if ( count ) {
		v.word.lo =
		    ( v.word.lo >> count ) | ( v.word.hi << ( 32 - count ) );
		v.word.hi = hi >> count;
	}
	return v.value;
}
extern uint32_t __bswapsi2 ( uint32_t value );
uint64_t __bswapdi2 ( uint64_t value )
{
	union wide_word v = { .value = value }, out;
	out.word.hi = __bswapsi2 ( v.word.lo );
	out.word.lo = __bswapsi2 ( v.word.hi );
	return out.value;
}
