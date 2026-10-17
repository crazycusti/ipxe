#ifndef _IPXE_TSUNAMI_PCI_H
#define _IPXE_TSUNAMI_PCI_H
FILE_LICENCE ( GPL2_OR_LATER );
#ifdef PCIAPI_TSUNAMI
#define PCIAPI_PREFIX_tsunami
#else
#define PCIAPI_PREFIX_tsunami __tsunami_
#endif
struct pci_device;
struct pci_range;
extern int tsunami_read_byte ( struct pci_device *, unsigned int, uint8_t * );
extern int tsunami_write_byte ( struct pci_device *, unsigned int, uint8_t );
extern int tsunami_read_word ( struct pci_device *, unsigned int, uint16_t * );
extern int tsunami_write_word ( struct pci_device *, unsigned int, uint16_t );
extern int tsunami_read_dword ( struct pci_device *, unsigned int, uint32_t * );
extern int tsunami_write_dword ( struct pci_device *, unsigned int, uint32_t );
extern int tsunami_can_probe ( struct pci_device * );
extern void tsunami_discover ( uint32_t, struct pci_range * );
extern void *tsunami_ioremap ( struct pci_device *, unsigned long, size_t );
#endif
