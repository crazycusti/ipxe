#ifndef _IPXE_OFW_IO_H
#define _IPXE_OFW_IO_H
FILE_LICENCE ( GPL2_OR_LATER );
#include <byteswap.h>
#ifdef IOAPI_OFW
#define IOAPI_PREFIX_ofw
#else
#define IOAPI_PREFIX_ofw __ofw_
#endif
#define OFW_MMIO( suffix, type, from, to )                                     \
	static inline __always_inline type IOAPI_INLINE (                      \
	    ofw, read##suffix ) ( volatile type * p )                          \
	{                                                                      \
		return from ( *p );                                            \
	}                                                                      \
	static inline __always_inline void IOAPI_INLINE (                      \
	    ofw, write##suffix ) ( type v, volatile type * p )                 \
	{                                                                      \
		*p = to ( v );                                                 \
	}
#define ofw_identity( x ) ( x )
OFW_MMIO ( b, uint8_t, ofw_identity, ofw_identity )
OFW_MMIO ( w, uint16_t, le16_to_cpu, cpu_to_le16 )
OFW_MMIO ( l, uint32_t, le32_to_cpu, cpu_to_le32 )
OFW_MMIO ( q, uint64_t, le64_to_cpu, cpu_to_le64 )
static inline __always_inline void IOAPI_INLINE ( ofw, mb ) ( void )
{
	__sync_synchronize ();
}
#include <ipxe/dummy_pio.h>
DUMMY_PIO ( ofw );
#endif
