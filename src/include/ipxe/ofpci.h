#ifndef _IPXE_OFPCI_PCI_H
#define _IPXE_OFPCI_PCI_H
FILE_LICENCE ( GPL2_OR_LATER );
#ifdef PCIAPI_OFPCI
#define PCIAPI_PREFIX_ofpci
#else
#define PCIAPI_PREFIX_ofpci __ofpci_
#endif
struct pci_device;
struct pci_range;
extern int ofpci_read_byte ( struct pci_device *, unsigned int, uint8_t * );
extern int ofpci_write_byte ( struct pci_device *, unsigned int, uint8_t );
extern int ofpci_read_word ( struct pci_device *, unsigned int, uint16_t * );
extern int ofpci_write_word ( struct pci_device *, unsigned int, uint16_t );
extern int ofpci_read_dword ( struct pci_device *, unsigned int, uint32_t * );
extern int ofpci_write_dword ( struct pci_device *, unsigned int, uint32_t );
extern int ofpci_can_probe ( struct pci_device * );
extern void ofpci_discover ( uint32_t, struct pci_range * );
extern void *ofpci_ioremap ( struct pci_device *, unsigned long, size_t );
#endif
