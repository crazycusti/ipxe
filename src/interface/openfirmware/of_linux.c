/* Linux PowerPC Open Firmware entry, including BE and LE ELF payloads. */
FILE_LICENCE ( GPL2_OR_LATER );
FILE_SECBOOT ( FORBIDDEN );
#if defined ( __powerpc__ ) || defined ( __powerpc64__ )
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ipxe/image.h>
#include <ipxe/native_elf.h>
#include <ipxe/openfirmware.h>
#include <ipxe/init.h>
extern unsigned long of_client_entry ( void );
extern unsigned long of_client_msr ( void );
extern void of_linux_enter ( unsigned long, unsigned long, unsigned long,
 unsigned long, unsigned long ) __attribute__ (( noreturn ));
static int of_linux_probe ( struct image *image ) {
 struct native_elf elf;
 int rc = native_elf_parse ( image, &elf );
 return rc ? rc : ( ( elf.machine == 20 && elf.bits == 32 && !elf.little ) ||
                    ( elf.machine == 21 && elf.bits == 64 ) ? 0 : -ENOEXEC );
}
static int of_linux_exec ( struct image *image ) {
 struct native_elf elf;
 struct image *rd = find_image ( "initrd" );
 const char *cmd = image->cmdline ? image->cmdline : "";
 uint64_t low = ~0ULL, high = 0;
 unsigned long base, entry = 0, address, initrd = 0;
 unsigned int i;
 of_cell_t a[4];
 int rc;
 if ( ( rc = native_elf_parse ( image, &elf ) ) != 0 ) return rc;
 if ( strlen ( cmd ) > 4095 || ( rd && ( (unsigned long)rd->data > 0xffffffffUL || rd->len > 128UL * 1024 * 1024 ) ) ) return -E2BIG;
 for ( i = 0; i < elf.count; i++ ) {
  struct native_elf_segment *s = &elf.segment[i];
  if ( s->physical < low ) low = s->physical;
  if ( s->physical + s->memsz > high ) high = s->physical + s->memsz;
 }
 /* Linux's OF trampoline supports a relocated load; do not write address
  * zero or overwrite iPXE/firmware. Keep the complete reserved load range. */
 if ( high <= low || high - low > 256UL * 1024 * 1024 ) return -ERANGE;
 /* prom_init needs its low-memory allocator below the real-mode limit.
  * Firmware's unconstrained allocator can return a load address above it. */
 for ( base = 0x04000000; base < 0x20000000; base += 0x01000000 ) {
  if ( sizeof ( unsigned long ) == 4 && base + high - low > 0x10000000 ) break;
  a[0] = base; a[1] = high - low; a[2] = 0;
  if ( !of_call ( "claim", 3, 1, a ) && a[0] == base ) break;
 }
 if ( base == 0x20000000 || ( sizeof ( unsigned long ) == 4 && base + high - low > 0x10000000 ) ) return -ENOMEM;
 if ( base > 0xffffffffUL - ( high - low ) ) { rc = -ERANGE; goto release; }
 for ( i = 0; i < elf.count; i++ ) {
  struct native_elf_segment *s = &elf.segment[i];
  void *p = (void *)(unsigned long)( base + s->physical - low );
  memcpy ( p, image->data + s->offset, s->filesz );
  memset ( p + s->filesz, 0, s->memsz - s->filesz );
  if ( elf.entry >= s->virtual && elf.entry - s->virtual < s->filesz )
   entry = base + s->physical - low + elf.entry - s->virtual;
 }
 if ( !entry ) { rc = -ENOEXEC; goto release; }
 /* Firmware may allocate image buffers near zero. Linux relocates itself
  * there, so retain a separate initrd reservation outside that range. */
 if ( rd && rd->len ) {
  unsigned long limit = ( sizeof ( unsigned long ) == 4 ? 0x10000000 : 0x30000000 );
  for ( initrd = ( sizeof ( unsigned long ) == 4 ? 0x08000000 : 0x10000000 ); initrd < limit; initrd += 0x01000000 ) {
   if ( rd->len > limit - initrd ) continue;
   if ( initrd < base + high - low && base < initrd + rd->len ) continue;
   a[0] = initrd; a[1] = rd->len; a[2] = 0;
   if ( !of_call ( "claim", 3, 1, a ) && a[0] == initrd ) break;
  }
  if ( initrd == limit ) { initrd = 0; rc = -ENOMEM; goto release; }
  memcpy ( (void *)initrd, rd->data, rd->len );
 }
 a[0] = of_find ( "/chosen" ); a[1] = (unsigned long)"bootargs";
 a[2] = (unsigned long)cmd; a[3] = strlen ( cmd ) + 1;
 if ( of_call ( "setprop", 4, 1, a ) || a[0] != strlen ( cmd ) + 1 ) { rc = -EIO; goto release; }
 printf ( "Linux OF: entry=%lx initrd=%ld command=%s\n", entry, (unsigned long)( rd ? rd->len : 0 ), cmd );
 shutdown_boot ();
 for ( address = base; address < base + ( high - low ); address += 32 )
  __asm__ volatile ( "dcbst 0,%0; sync; icbi 0,%0" : : "r" ( address ) : "memory" );
 __asm__ volatile ( "sync; isync" ::: "memory" );
 of_linux_enter ( entry, initrd, rd ? rd->len : 0,
                  of_client_entry (), of_client_msr () );
release:
 if ( initrd ) { a[0] = initrd; a[1] = rd->len; of_call ( "release", 2, 0, a ); }
 a[0] = base; a[1] = high - low; of_call ( "release", 2, 0, a );
 return rc;
}
struct image_type of_linux_image __image_type ( PROBE_NORMAL ) = {
 .name = "Linux/OF", .probe = of_linux_probe, .exec = of_linux_exec,
};
#endif
