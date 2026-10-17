#ifndef _IPXE_DINO_PCI_H
#define _IPXE_DINO_PCI_H
FILE_LICENCE ( GPL2_OR_LATER );
#ifdef PCIAPI_DINO
#define PCIAPI_PREFIX_dino
#else
#define PCIAPI_PREFIX_dino __dino_
#endif
struct pci_device;
struct pci_range;
extern int dino_read_byte ( struct pci_device *, unsigned int, uint8_t * );
extern int dino_write_byte ( struct pci_device *, unsigned int, uint8_t );
extern int dino_read_word ( struct pci_device *, unsigned int, uint16_t * );
extern int dino_write_word ( struct pci_device *, unsigned int, uint16_t );
extern int dino_read_dword ( struct pci_device *, unsigned int, uint32_t * );
extern int dino_write_dword ( struct pci_device *, unsigned int, uint32_t );
extern int dino_can_probe ( struct pci_device * );
extern void dino_discover ( uint32_t, struct pci_range * );
extern void *dino_ioremap ( struct pci_device *, unsigned long, size_t );
#endif
