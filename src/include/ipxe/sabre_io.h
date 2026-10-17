#ifndef _IPXE_SABRE_IO_H
#define _IPXE_SABRE_IO_H
FILE_LICENCE ( GPL2_OR_LATER );
#include <byteswap.h>
#ifdef IOAPI_OFW
#define IOAPI_PREFIX_ofw
#else
#define IOAPI_PREFIX_ofw __ofw_
#endif
/* Sabre registers use physical bypass ASIs, independently of the client's
 * firmware mappings. PCI words are little endian (ASI 0x1d). */
#define SABRE_MMIO(suffix,type,load,store,from,to) \
static inline __always_inline type IOAPI_INLINE ( ofw, read##suffix ) ( volatile type *p ) { \
 type v; \
 if ( (unsigned long)p >= 0x10000000000UL ) \
  __asm__ volatile ( load " [%1] 0x1d,%0" : "=r" ( v ) : "r" ( p ) : "memory" ); \
 else v = from ( *p ); \
 return v; \
} \
static inline __always_inline void IOAPI_INLINE ( ofw, write##suffix ) ( type v, volatile type *p ) { \
 if ( (unsigned long)p >= 0x10000000000UL ) \
  __asm__ volatile ( store " %0,[%1] 0x1d; membar #Sync" : : "r" ( v ), "r" ( p ) : "memory" ); \
 else *p = to ( v ); \
}
#define sabre_identity(x) (x)
SABRE_MMIO ( b, uint8_t, "lduba", "stba", sabre_identity, sabre_identity )
SABRE_MMIO ( w, uint16_t, "lduha", "stha", le16_to_cpu, cpu_to_le16 )
SABRE_MMIO ( l, uint32_t, "lduwa", "stwa", le32_to_cpu, cpu_to_le32 )
SABRE_MMIO ( q, uint64_t, "ldxa", "stxa", le64_to_cpu, cpu_to_le64 )
static inline __always_inline void IOAPI_INLINE ( ofw, mb ) ( void ) {
 __asm__ volatile ( "membar #Sync" ::: "memory" );
}
#include <ipxe/dummy_pio.h>
DUMMY_PIO ( ofw );
#endif
