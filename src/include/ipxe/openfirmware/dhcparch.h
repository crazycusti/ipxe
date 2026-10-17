#ifndef _IPXE_OPENFIRMWARE_DHCPARCH_H
#define _IPXE_OPENFIRMWARE_DHCPARCH_H
FILE_LICENCE ( GPL2_OR_LATER );
#include <ipxe/dhcp.h>
/* Experimental native Open Firmware client: do not impersonate x86 EFI. */
#define DHCP_ARCH_CLIENT_ARCHITECTURE 0xffff
#define DHCP_ARCH_CLIENT_NDI 1, 0, 0
#endif
