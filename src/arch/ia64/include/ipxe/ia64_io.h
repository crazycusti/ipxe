#ifndef _IPXE_IA64_IO_H
#define _IPXE_IA64_IO_H

/** @file
 *
 * iPXE I/O API for IA-64
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );
FILE_SECBOOT ( PERMITTED );

#ifdef IOAPI_IA64
#define IOAPI_PREFIX_ia64
#else
#define IOAPI_PREFIX_ia64 __ia64_
#endif

#include <ipxe/dummy_pio.h>

/*
 * Memory space mappings
 *
 */

/*
 * Physical<->Bus address mappings
 *
 */

static inline __always_inline unsigned long
IOAPI_INLINE ( ia64, phys_to_bus ) ( unsigned long phys_addr ) {
	return phys_addr;
}

static inline __always_inline unsigned long
IOAPI_INLINE ( ia64, bus_to_phys ) ( unsigned long bus_addr ) {
	return bus_addr;
}

/*
 * MMIO reads and writes up to native word size
 *
 */

#define IA64_IO( suffix, type ) \
static inline __always_inline type IOAPI_INLINE ( ia64, read ## suffix ) ( volatile type *p ) { \
 type v = *p; __asm__ __volatile__ ( "mf" ::: "memory" ); return v; } \
static inline __always_inline void IOAPI_INLINE ( ia64, write ## suffix ) ( type v, volatile type *p ) { \
 __asm__ __volatile__ ( "mf" ::: "memory" ); *p = v; }
IA64_IO ( b, uint8_t );
IA64_IO ( w, uint16_t );
IA64_IO ( l, uint32_t );
IA64_IO ( q, uint64_t );

/*
 * Memory barrier
 *
 */
static inline __always_inline void
IOAPI_INLINE ( ia64, mb ) ( void ) {
	__asm__ __volatile__ ( "mf" ::: "memory" );
}

/* Dummy PIO */
DUMMY_PIO ( ia64 );

#endif /* _IPXE_IA64_IO_H */
