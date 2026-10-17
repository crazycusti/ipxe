#ifndef _IPXE_SABRE_PCI_H
#define _IPXE_SABRE_PCI_H
FILE_LICENCE ( GPL2_OR_LATER );
#ifdef PCIAPI_SABRE
#define PCIAPI_PREFIX_sabre
#else
#define PCIAPI_PREFIX_sabre __sabre_
#endif
struct pci_device;
struct pci_range;
extern int sabre_read_byte ( struct pci_device *, unsigned int, uint8_t * );
extern int sabre_write_byte ( struct pci_device *, unsigned int, uint8_t );
extern int sabre_read_word ( struct pci_device *, unsigned int, uint16_t * );
extern int sabre_write_word ( struct pci_device *, unsigned int, uint16_t );
extern int sabre_read_dword ( struct pci_device *, unsigned int, uint32_t * );
extern int sabre_write_dword ( struct pci_device *, unsigned int, uint32_t );
extern int sabre_can_probe ( struct pci_device * );
extern void sabre_discover ( uint32_t, struct pci_range * );
extern void *sabre_ioremap ( struct pci_device *, unsigned long, size_t );
#endif
