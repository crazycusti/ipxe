/* Sabre PCI for UltraSPARC-IIi / sun4u, using physical-bypass accesses. */
FILE_LICENCE ( GPL2_OR_LATER );
#if defined ( __sparc__ ) && defined ( __arch64__ )
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ipxe/pci.h>
#include <ipxe/openfirmware.h>
#include <ipxe/init.h>
#define SABRE 0x1fe00000000UL
static int detected;
static int available ( void ) {
 char model[64] = { 0 };
 uint32_t reg[4];
 if ( !detected ) {
  of_prop ( of_find ( "/pci" ), "model", model, sizeof ( model ) - 1 );
  DBGC ( &detected, "SABRE PCI model %s\n", model );
  detected = ( !strcmp ( model, "SUNW,sabre" ) &&
               of_prop ( of_find ( "/pci" ), "reg", reg, sizeof ( reg ) ) >= 8 &&
               be32_to_cpu ( reg[0] ) == 0x1fe && be32_to_cpu ( reg[1] ) == 0 ? 1 : -1 );
 }
 return detected > 0;
}
int sabre_can_probe ( struct pci_device *pci ) {
 return available () && pci->busdevfn < 768;
}
void sabre_discover ( uint32_t start, struct pci_range *range ) {
 range->start = start;
 range->count = ( available () && start < 768 ? 768 - start : 0 );
}
static void *config ( struct pci_device *pci, unsigned int where, unsigned int size ) {
 if ( !sabre_can_probe ( pci ) || where > 256 - size || ( where & ( size - 1 ) ) ) return NULL;
 return (void *)( SABRE + 0x1000000 + ( pci->busdevfn << 8 ) + where );
}
#define SABRE_CONFIG(kind,type,suffix) \
int sabre_read_##kind ( struct pci_device *pci, unsigned int where, type *v ) { \
 void *p = config ( pci, where, sizeof ( *v ) ); \
 *v = p ? read##suffix ( p ) : (type)-1; return p ? 0 : -EINVAL; \
} \
int sabre_write_##kind ( struct pci_device *pci, unsigned int where, type v ) { \
 void *p = config ( pci, where, sizeof ( v ) ); \
 if ( !p ) { return -EINVAL; } write##suffix ( v, p ); return 0; \
}
SABRE_CONFIG ( byte, uint8_t, b )
SABRE_CONFIG ( word, uint16_t, w )
SABRE_CONFIG ( dword, uint32_t, l )
static uint64_t saved_control;
static int changed;
static uint64_t iommu_read ( void ) {
 uint64_t value;
 __asm__ volatile ( "ldxa [%1] 0x15,%0" : "=r" ( value ) : "r" ( SABRE + 0x200 ) : "memory" );
 return value;
}
static void iommu_write ( uint64_t value ) {
 __asm__ volatile ( "stxa %0,[%1] 0x15; membar #Sync" : : "r" ( value ), "r" ( SABRE + 0x200 ) : "memory" );
}
static void sabre_shutdown ( int booting __unused ) {
 if ( changed ) { iommu_write ( saved_control ); changed = 0; }
}
struct startup_fn sabre_startup __startup_fn ( STARTUP_EARLY ) = {
 .shutdown = sabre_shutdown,
};
void *sabre_ioremap ( struct pci_device *pci, unsigned long address, size_t len ) {
 if ( !sabre_can_probe ( pci ) || !address || address >= 0x100000000UL || len > 0x100000000UL - address ) return NULL;
 if ( !changed ) {
  saved_control = iommu_read ();
  iommu_write ( saved_control & ~1ULL );
  changed = 1;
 }
 return (void *)( 0x1ff00000000UL + address );
}
PROVIDE_PCIAPI ( sabre, pci_can_probe, sabre_can_probe );
PROVIDE_PCIAPI ( sabre, pci_discover, sabre_discover );
PROVIDE_PCIAPI ( sabre, pci_read_config_byte, sabre_read_byte );
PROVIDE_PCIAPI ( sabre, pci_read_config_word, sabre_read_word );
PROVIDE_PCIAPI ( sabre, pci_read_config_dword, sabre_read_dword );
PROVIDE_PCIAPI ( sabre, pci_write_config_byte, sabre_write_byte );
PROVIDE_PCIAPI ( sabre, pci_write_config_word, sabre_write_word );
PROVIDE_PCIAPI ( sabre, pci_write_config_dword, sabre_write_dword );
PROVIDE_PCIAPI ( sabre, pci_ioremap, sabre_ioremap );
#endif
