#ifndef CONFIG_DEFAULTS_OPENFIRMWARE_H
#define CONFIG_DEFAULTS_OPENFIRMWARE_H
FILE_LICENCE ( GPL2_OR_LATER );
#define IOAPI_OFW
#define IOMAP_VIRT
#define UACCESS_FLAT
#define DMAAPI_FLAT
#define UMALLOC_OPENFIRMWARE
#define MEMMAP_NULL
#define NAP_NULL
#define SMBIOS_NULL
#define SANBOOT_NULL
#define TIME_NULL
#define REBOOT_NULL
#if defined ( __powerpc__ ) || defined ( __powerpc64__ )
#define PCIAPI_OFPCI
#elif defined ( __sparc__ ) && defined ( __arch64__ )
#define PCIAPI_SABRE
#else
#define PCIAPI_NULL
#endif
#define ACPI_NULL
#define MPAPI_NULL
#define SERIAL_NULL
#define FDT_NULL
#endif
