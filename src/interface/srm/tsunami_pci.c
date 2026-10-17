/* Dense Tsunami PCI configuration for SRM AlphaServer DS20/DP264. */
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <errno.h>
#include <ipxe/pci.h>
#include <ipxe/srm.h>
#include <ipxe/init.h>
unsigned long srm_system_type;
#define TSUNAMI_IO 0xfffffd0000000000UL
int tsunami_can_probe ( struct pci_device *pci ) {
 return srm_system_type == 34 && pci->busdevfn < 256;
}
void tsunami_discover ( uint32_t start, struct pci_range *range ) {
 range->start = start;
 range->count = ( srm_system_type == 34 && start < 256 ? 256 - start : 0 );
}
static void *tsunami_config ( struct pci_device *pci, unsigned int where,
 unsigned int size ) {
 if ( !tsunami_can_probe ( pci ) || where > 256 - size || ( where & ( size - 1 ) ) )
  return NULL;
 return (void *)( TSUNAMI_IO + 0x1fe000000UL + ( pci->busdevfn << 8 ) + where );
}
#define TSUNAMI_CONFIG( kind, type ) \
 int tsunami_read_##kind ( struct pci_device *pci, unsigned int where, type *value ) { \
  volatile type *p = tsunami_config ( pci, where, sizeof ( *p ) ); \
  *value = p ? *p : (type)-1; return p ? 0 : -EINVAL; \
 } \
 int tsunami_write_##kind ( struct pci_device *pci, unsigned int where, type value ) { \
  volatile type *p = tsunami_config ( pci, where, sizeof ( *p ) ); \
  if ( !p ) { return -EINVAL; } *p = value; __sync_synchronize (); return 0; \
 }
TSUNAMI_CONFIG ( byte, uint8_t )
TSUNAMI_CONFIG ( word, uint16_t )
TSUNAMI_CONFIG ( dword, uint32_t )
/* Preserve firmware windows. Claim an unused direct DMA window only for
 * low image-owned RAM; return the original registers when leaving iPXE. */
static int dma_window = -1;
static uint64_t dma_saved[3];
static volatile uint64_t *window_reg ( unsigned int offset ) {
 return (void *)( TSUNAMI_IO + 0x180000000UL + offset );
}
static int tsunami_dma_window ( void ) {
 int i;
 uint64_t base, mask, target;
 if ( dma_window >= 0 ) return 1;
 for ( i = 0; i < 4; i++ ) {
  base = *window_reg ( i * 64 );
  mask = *window_reg ( 0x100 + i * 64 );
  target = *window_reg ( 0x200 + i * 64 );
  if ( ( base & 3 ) == 1 && !( base & 0xfff00000UL ) &&
       !target && mask >= 0x03f00000 ) return 1;
 }
 /* Window 3 is scatter/gather-only; direct DMA uses windows 0..2. */
 for ( i = 2; i >= 0; i-- ) {
  if ( *window_reg ( i * 64 ) & 1 ) continue;
  dma_saved[0] = *window_reg ( i * 64 );
  dma_saved[1] = *window_reg ( 0x100 + i * 64 );
  dma_saved[2] = *window_reg ( 0x200 + i * 64 );
  *window_reg ( 0x100 + i * 64 ) = 0x03f00000;
  *window_reg ( 0x200 + i * 64 ) = 0;
  __sync_synchronize ();
  *window_reg ( i * 64 ) = 1;
  __sync_synchronize ();
  dma_window = i;
  DBGC ( &dma_window, "TSUNAMI DMA window %d base=%016llx mask=%016llx target=%016llx\n",
         i, (unsigned long long)*window_reg ( i * 64 ),
         (unsigned long long)*window_reg ( 0x100 + i * 64 ),
         (unsigned long long)*window_reg ( 0x200 + i * 64 ) );
  return 1;
 }
 return 0;
}
static void tsunami_shutdown ( int booting __unused ) {
 unsigned int offset;
 if ( dma_window < 0 ) return;
 offset = dma_window * 64;
 *window_reg ( offset ) = 0;
 __sync_synchronize ();
 *window_reg ( 0x100 + offset ) = dma_saved[1];
 *window_reg ( 0x200 + offset ) = dma_saved[2];
 *window_reg ( offset ) = dma_saved[0];
 __sync_synchronize ();
 dma_window = -1;
}
struct startup_fn tsunami_startup __startup_fn ( STARTUP_EARLY ) = {
 .shutdown = tsunami_shutdown,
};
void *tsunami_ioremap ( struct pci_device *pci, unsigned long address, size_t len ) {
 if ( !tsunami_can_probe ( pci ) || !tsunami_dma_window () || !address || address >= 0x100000000UL ||
      len > 0x100000000UL - address ) return NULL;
 return (void *)( TSUNAMI_IO + address );
}
PROVIDE_PCIAPI ( tsunami, pci_can_probe, tsunami_can_probe );
PROVIDE_PCIAPI ( tsunami, pci_discover, tsunami_discover );
PROVIDE_PCIAPI ( tsunami, pci_read_config_byte, tsunami_read_byte );
PROVIDE_PCIAPI ( tsunami, pci_read_config_word, tsunami_read_word );
PROVIDE_PCIAPI ( tsunami, pci_read_config_dword, tsunami_read_dword );
PROVIDE_PCIAPI ( tsunami, pci_write_config_byte, tsunami_write_byte );
PROVIDE_PCIAPI ( tsunami, pci_write_config_word, tsunami_write_word );
PROVIDE_PCIAPI ( tsunami, pci_write_config_dword, tsunami_write_dword );
PROVIDE_PCIAPI ( tsunami, pci_ioremap, tsunami_ioremap );
