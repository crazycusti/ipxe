/* Narrow Dino PCI configuration and memory access.
 * Register definitions follow QEMU hw/pci-host/dino.c and SeaBIOS-hppa.
 * Only the firmware-confirmed 0x68/0x03 bridge at its standard HPA is used.
 */
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <errno.h>
#include <byteswap.h>
#include <ipxe/pci.h>

#define DINO_HPA 0xfff80000UL
#define DINO_ADDR ( (volatile uint32_t *)( DINO_HPA + 0x64 ) )
#define DINO_DATA ( (volatile unsigned char *)( DINO_HPA + 0x68 ) )
extern volatile uint32_t pdc_entry_address;
static int dino_present ( void )
{
	static int present = -1;
	static uint32_t result[32] __attribute__ ( ( aligned ( 8 ) ) );
	static unsigned char iodc[16] __attribute__ ( ( aligned ( 8 ) ) );
	typedef int ( *pdc_fn ) (
	    unsigned long, unsigned long, void *, unsigned long, unsigned long,
	    void *, unsigned long, unsigned long, unsigned long, unsigned long,
	    unsigned long, unsigned long, unsigned long, unsigned long );
	pdc_fn call = (pdc_fn)pdc_entry_address;
	if ( present < 0 ) {
		int rc;
		present = 0;
		rc = ( call ? call ( 8, 0, result, DINO_HPA, 0, iodc,
				     sizeof ( iodc ), 0, 0, 0, 0, 0, 0, 0 )
			    : -1 );
		DBGC ( &present,
		       "DINO PDC query rc=%d size=%d id=%02x/%02x type=%02x\n",
		       rc, result[0], iodc[0], iodc[1], iodc[3] );
		if ( !rc && result[0] >= 4 && iodc[0] == 0x68 &&
		     iodc[1] == 0x03 && ( iodc[3] & 0x0f ) == 0x0d )
			present = 1;
	}
	return present;
}
int dino_can_probe ( struct pci_device *pci )
{
	return dino_present () && pci->busdevfn < 256;
}
void dino_discover ( uint32_t start, struct pci_range *range )
{
	range->start = start;
	range->count = ( dino_present () && start < 256 ? 256 - start : 0 );
}
static int dino_config ( struct pci_device *pci, unsigned int where,
			 unsigned int size )
{
	if ( !dino_can_probe ( pci ) || where > 256 - size ||
	     ( where & ( size - 1 ) ) )
		return -EINVAL;
	/* Address selector is CPU-endian, configuration data is PCI-endian. */
	*DINO_ADDR = ( pci->busdevfn << 8 ) | ( where & ~3U );
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
			    *(volatile type *)( DINO_DATA + ( where & 3 ) ) ); \
		return rc;                                                     \
	}                                                                      \
	int dino_write_##kind ( struct pci_device *pci, unsigned int where,    \
				type value )                                   \
	{                                                                      \
		int rc = dino_config ( pci, where, sizeof ( value ) );         \
		if ( !rc )                                                     \
			*(volatile type *)( DINO_DATA + ( where & 3 ) ) =      \
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
	if ( !dino_present () || address < 0xf0800000UL ||
	     address >= 0xff800000UL || length > 0xff800000UL - address )
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
