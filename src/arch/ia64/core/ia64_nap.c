FILE_LICENCE ( GPL2_OR_LATER );
#include <ipxe/efi/efi.h>

/* EFI has no portable CPU-sleep service. A short Stall yields through
 * firmware, also permitting cooperative firmware timer implementations
 * to dispatch events when iPXE next drops its task priority level.
 */
void ia64_cpu_halt ( void ) {
 efi_systab->BootServices->Stall ( 1000 );
}
