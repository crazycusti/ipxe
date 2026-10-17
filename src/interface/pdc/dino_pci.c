/* Narrow PDC PCI access for Dino and Astro/Elroy.
 * Register definitions follow the HP bridge interfaces, QEMU and SeaBIOS-hppa.
 * Probe only bridges confirmed by PDC IODC before touching their registers.
 */
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <byteswap.h>
#include <ipxe/pci.h>

#define DINO_HPA 0xfff80000UL
static const unsigned long elroy_hpa[] = {
	0xfed30000UL, 0xfed32000UL, 0xfed38000UL, 0xfed3c000UL,
};
/* One Dino root or up to four independent Elroy roots. */
static unsigned long bridge_hpa[4];
static unsigned int bridge_count;
static int astro;
extern volatile uint32_t pdc_entry_address;
static int pdc_bridge ( unsigned long hpa, unsigned int model,
			unsigned int revision )
{
	static uint32_t result[32] __attribute__ ( ( aligned ( 8 ) ) );
	static unsigned char iodc[16] __attribute__ ( ( aligned ( 8 ) ) );
	typedef int ( *pdc_fn ) (
	    unsigned long, unsigned long, void *, unsigned long, unsigned long,
	    void *, unsigned long, unsigned long, unsigned long, unsigned long,
	    unsigned long, unsigned long, unsigned long, unsigned long );
	pdc_fn call = (pdc_fn)pdc_entry_address;
	int rc;
	memset ( iodc, 0, sizeof ( iodc ) );
	memset ( result, 0, sizeof ( result ) );
	rc = ( call ? call ( 8, 0, result, hpa, 0, iodc, sizeof ( iodc ),
			   0, 0, 0, 0, 0, 0, 0 ) : -1 );
	DBGC ( &bridge_count, "PDC PCI %08lx rc=%d size=%d id=%02x/%02x type=%02x\n",
	       hpa, rc, result[0], iodc[0], iodc[1], iodc[3] );
	return ( !rc && result[0] >= 4 && iodc[0] == model &&
		 iodc[1] == revision && ( iodc[3] & 0x0f ) == 0x0d );
}
static int dino_present ( void )
{
	static int checked;
	unsigned int i;
	if ( !checked ) {
		checked = 1;
		if ( pdc_bridge ( DINO_HPA, 0x68, 0x03 ) ) {
			bridge_hpa[0] = DINO_HPA;
			bridge_count = 1;
		} else {
			for ( i = 0; i < 4; i++ ) {
				if ( pdc_bridge ( elroy_hpa[i], 0x78, 0x20 ) ) {
					bridge_hpa[i] = elroy_hpa[i];
					bridge_count = i + 1;
					astro = 1;
				}
			}
		}
	}
	return bridge_count;
}
int dino_can_probe ( struct pci_device *pci )
{
	return dino_present () && pci->busdevfn < bridge_count * 256 &&
		bridge_hpa[pci->busdevfn >> 8];
}
void dino_discover ( uint32_t start, struct pci_range *range )
{
	unsigned int limit = dino_present () * 256;
	range->start = start;
	range->count = ( start < limit ? limit - start : 0 );
}
static volatile unsigned char *dino_data ( struct pci_device *pci )
{
	return (volatile unsigned char *)( bridge_hpa[pci->busdevfn >> 8] +
					 ( astro ? 0x48 : 0x68 ) );
}
static int dino_config ( struct pci_device *pci, unsigned int where,
			 unsigned int size )
{
	if ( !dino_can_probe ( pci ) || where > 256 - size ||
	     ( where & ( size - 1 ) ) )
		return -EINVAL;
	/* Dino has a native-endian selector; Elroy uses PCI byte order. */
	volatile uint32_t *addr = (volatile uint32_t *)(
		bridge_hpa[pci->busdevfn >> 8] + ( astro ? 0x40 : 0x64 ) );
	uint32_t value = ( ( pci->busdevfn & 0xff ) << 8 ) | ( where & ~3U );
	*addr = ( astro ? cpu_to_le32 ( 0x80000000U | value ) : value );
	__asm__ volatile ( "sync" ::: "memory" );
	return 0;
}
#define DINO_CONFIG_ACCESS( kind, type, from, to )                             \
	int dino_read_##kind ( struct pci_device *pci, unsigned int where,     \
			       type *value )                                   \
	{                                                                      \
		int rc = dino_config ( pci, where, sizeof ( *value ) );        \
		*value = ( type ) - 1;                                         \
		if ( !rc )                                                     \
			*value = from (                                        \
			    *(volatile type *)( dino_data ( pci ) + ( where & 3 ) ) ); \
		return rc;                                                     \
	}                                                                      \
	int dino_write_##kind ( struct pci_device *pci, unsigned int where,    \
				type value )                                   \
	{                                                                      \
		int rc = dino_config ( pci, where, sizeof ( value ) );         \
		if ( !rc )                                                     \
			*(volatile type *)( dino_data ( pci ) + ( where & 3 ) ) =      \
			    to ( value );                                      \
		return rc;                                                     \
	}
#define dino_identity( value ) ( value )
DINO_CONFIG_ACCESS ( byte, uint8_t, dino_identity, dino_identity )
DINO_CONFIG_ACCESS ( word, uint16_t, le16_to_cpu, cpu_to_le16 )
DINO_CONFIG_ACCESS ( dword, uint32_t, le32_to_cpu, cpu_to_le32 )
void *dino_ioremap ( struct pci_device *pci __unused, unsigned long address,
		     size_t length )
{
	unsigned long base, end;
	if ( !dino_present () )
		return NULL;
	base = ( astro ? 0xf0000000UL : 0xf0800000UL );
	end = 0xff800000UL;
	if ( address < base || address >= end || length > end - address )
		return NULL;
	return (void *)address;
}
PROVIDE_PCIAPI ( dino, pci_can_probe, dino_can_probe );
PROVIDE_PCIAPI ( dino, pci_discover, dino_discover );
PROVIDE_PCIAPI ( dino, pci_read_config_byte, dino_read_byte );
PROVIDE_PCIAPI ( dino, pci_read_config_word, dino_read_word );
PROVIDE_PCIAPI ( dino, pci_read_config_dword, dino_read_dword );
PROVIDE_PCIAPI ( dino, pci_write_config_byte, dino_write_byte );
PROVIDE_PCIAPI ( dino, pci_write_config_word, dino_write_word );
PROVIDE_PCIAPI ( dino, pci_write_config_dword, dino_write_dword );
PROVIDE_PCIAPI ( dino, pci_ioremap, dino_ioremap );
