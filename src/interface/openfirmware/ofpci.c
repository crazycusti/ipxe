/* Apple PCI via IEEE 1275 configuration and mapping methods.
 * Keep pseries on its firmware packet backend: this flat-DMA implementation
 * is restricted to PowerMac, whose PCI address mappings firmware owns.
 */
FILE_LICENCE ( GPL2_OR_LATER );
#if defined ( __powerpc__ ) || defined ( __powerpc64__ )
#include <stdint.h>
#include <errno.h>
#include <byteswap.h>
#include <string.h>
#include <ipxe/pci.h>
#include <ipxe/openfirmware.h>
static unsigned long uninorth;
static of_cell_t ofpci_root ( void ) {
 static int checked;
 static of_cell_t root;
 char model[64];
 of_cell_t a[1];
 int n;
 if ( !checked ) {
  checked = 1;
  n = of_prop ( of_find ( "/" ), "model", model, sizeof ( model ) );
  if ( n < 8 || memcmp ( model, "PowerMac", 8 ) ) return 0;
  a[0] = (unsigned long)"/pci";
  if ( !of_call ( "open", 1, 1, a ) && a[0] != (of_cell_t)-1 ) root = a[0];
  /* UniNorth CFA0 configuration (slots 11..31). OpenBIOS lacks the
   * standard config-* methods. Verify this host bridge before MMIO. */
  if ( n >= 11 && !memcmp ( model, "PowerMac3,1", 11 ) ) {
   uint32_t reg[4];
   if ( of_prop ( of_find ( "/pci" ), "reg", reg, sizeof ( reg ) ) >= 4 &&
        ( be32_to_cpu ( reg[0] ) == 0xf2000000 ||
          be32_to_cpu ( reg[0] ) == 0xf0000000 ) )
    uninorth = be32_to_cpu ( reg[0] );
  }
 }
 return root;
}
int ofpci_can_probe ( struct pci_device *pci ) {
 return ofpci_root () && pci->busdevfn < 256;
}
void ofpci_discover ( uint32_t start, struct pci_range *range ) {
 range->start = start;
 range->count = ( ofpci_root () && start < 256 ? 256 - start : 0 );
}
static int ofpci_config ( struct pci_device *pci, unsigned int where,
 unsigned int size, const char *method, uint32_t *value, int write ) {
 of_cell_t a[5];
 if ( !ofpci_can_probe ( pci ) || where > 256 - size || ( where & ( size - 1 ) ) )
  return -EINVAL;
 if ( uninorth ) {
  unsigned int slot = ( pci->busdevfn >> 3 ) & 31;
  volatile unsigned char *data = (void *)( uninorth + 0x00c00000UL + ( where & 7 ) );
  if ( slot < 11 ) { if ( !write ) *value = ~0U; return 0; }
  *(volatile uint32_t *)( uninorth + 0x00800000UL ) = cpu_to_le32 (
   ( 1U << slot ) | ( ( pci->busdevfn & 7 ) << 8 ) | ( where & 0xf8 ) );
  __asm__ volatile ( "sync" ::: "memory" );
  if ( write ) {
   if ( size == 1 ) *data = *value;
   else if ( size == 2 ) *(volatile uint16_t *)data = cpu_to_le16 ( *value );
   else *(volatile uint32_t *)data = cpu_to_le32 ( *value );
  } else {
   if ( size == 1 ) *value = *data;
   else if ( size == 2 ) *value = le16_to_cpu ( *(volatile uint16_t *)data );
   else *value = le32_to_cpu ( *(volatile uint32_t *)data );
  }
  return 0;
 }
 a[0] = (unsigned long)method; a[1] = ofpci_root ();
 a[2] = ( pci->busdevfn << 8 ) | where; a[3] = *value;
 if ( of_call ( "call-method", write ? 4 : 3, write ? 1 : 2, a ) || a[0] )
  return -EIO;
 if ( !write ) *value = a[1];
 return 0;
}
#define OFPCI_CONFIG( kind, type, suffix ) \
 int ofpci_read_##kind ( struct pci_device *pci, unsigned int where, type *value ) { \
  uint32_t v = ~0U; int rc = ofpci_config ( pci, where, sizeof ( *value ), "config-" suffix "@", &v, 0 ); \
  *value = v; return rc; \
 } \
 int ofpci_write_##kind ( struct pci_device *pci, unsigned int where, type value ) { \
  uint32_t v = value; return ofpci_config ( pci, where, sizeof ( value ), "config-" suffix "!", &v, 1 ); \
 }
OFPCI_CONFIG ( byte, uint8_t, "b" )
OFPCI_CONFIG ( word, uint16_t, "w" )
OFPCI_CONFIG ( dword, uint32_t, "l" )
void *ofpci_ioremap ( struct pci_device *pci, unsigned long address, size_t len ) {
 of_cell_t a[6] = { (unsigned long)"map-in", ofpci_root (), len,
                    0x02000000 | ( pci->busdevfn << 8 ), 0, address };
 if ( uninorth && address >= 0x80000000UL && address < 0xf0000000UL &&
      len <= 0xf0000000UL - address ) return (void *)address;
 if ( !ofpci_can_probe ( pci ) || !address || address > (of_cell_t)-1 ||
      len > (of_cell_t)-1 - address ||
      of_call ( "call-method", 6, 2, a ) || a[0] || !a[1] ) return NULL;
 return (void *)(unsigned long)a[1];
}
PROVIDE_PCIAPI ( ofpci, pci_can_probe, ofpci_can_probe );
PROVIDE_PCIAPI ( ofpci, pci_discover, ofpci_discover );
PROVIDE_PCIAPI ( ofpci, pci_read_config_byte, ofpci_read_byte );
PROVIDE_PCIAPI ( ofpci, pci_read_config_word, ofpci_read_word );
PROVIDE_PCIAPI ( ofpci, pci_read_config_dword, ofpci_read_dword );
PROVIDE_PCIAPI ( ofpci, pci_write_config_byte, ofpci_write_byte );
PROVIDE_PCIAPI ( ofpci, pci_write_config_word, ofpci_write_word );
PROVIDE_PCIAPI ( ofpci, pci_write_config_dword, ofpci_write_dword );
PROVIDE_PCIAPI ( ofpci, pci_ioremap, ofpci_ioremap );
#endif
