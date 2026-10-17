#ifndef _PORTABLE_BITS_BIGINT_H
#define _PORTABLE_BITS_BIGINT_H
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <string.h>
typedef uint32_t bigint_element_t;
static inline int bigint_add_raw ( const uint32_t *a, uint32_t *v,
				   unsigned int n )
{
	uint64_t c = 0;
	unsigned int i;
	for ( i = 0; i < n; i++ ) {
		c += (uint64_t)a[i] + v[i];
		v[i] = c;
		c >>= 32;
	}
	return c;
}
static inline int bigint_subtract_raw ( const uint32_t *a, uint32_t *v,
					unsigned int n )
{
	uint64_t b = 0, t;
	unsigned int i;
	for ( i = 0; i < n; i++ ) {
		t = (uint64_t)v[i] - a[i] - b;
		v[i] = t;
		b = t >> 63;
	}
	return b;
}
static inline int bigint_shl_raw ( uint32_t *v, unsigned int n )
{
	uint32_t c = 0, t;
	unsigned int i;
	for ( i = 0; i < n; i++ ) {
		t = v[i] >> 31;
		v[i] = ( v[i] << 1 ) | c;
		c = t;
	}
	return c;
}
static inline int bigint_shr_raw ( uint32_t *v, unsigned int n )
{
	uint32_t c = 0, t;
	while ( n ) {
		--n;
		t = v[n] & 1;
		v[n] = ( v[n] >> 1 ) | ( c << 31 );
		c = t;
	}
	return c;
}
static inline void bigint_grow_raw ( const uint32_t *s, unsigned int sn,
				     uint32_t *d, unsigned int dn )
{
	memcpy ( d, s, sn * sizeof ( *s ) );
	memset ( d + sn, 0, ( dn - sn ) * sizeof ( *d ) );
}
static inline void bigint_shrink_raw ( const uint32_t *s,
				       unsigned int sn __unused, uint32_t *d,
				       unsigned int dn )
{
	memcpy ( d, s, dn * sizeof ( *d ) );
}
static inline void bigint_multiply_one ( uint32_t a, uint32_t b, uint32_t *v,
					 uint32_t *c )
{
	uint64_t x = (uint64_t)a * b + *v + *c;
	*v = x;
	*c = x >> 32;
}
#endif
