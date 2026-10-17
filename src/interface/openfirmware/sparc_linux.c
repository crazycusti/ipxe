/* SPARC V9 Linux ELF entry using the HdrS 3.01 boot parameter contract. */
FILE_LICENCE ( GPL2_OR_LATER );
FILE_SECBOOT ( FORBIDDEN );
#if defined ( __sparc__ ) && defined ( __arch64__ )
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <byteswap.h>
#include <ipxe/image.h>
#include <ipxe/native_elf.h>
#include <ipxe/openfirmware.h>
#include <ipxe/init.h>
extern unsigned long of_client_entry ( void );
static int sparc_linux_probe ( struct image *image ) {
 struct native_elf elf;
 int rc = native_elf_parse ( image, &elf );
 return rc ? rc : ( elf.machine == 43 && elf.bits == 64 && !elf.little ? 0 : -ENOEXEC );
}
static int sparc_linux_exec ( struct image *image ) {
 struct native_elf elf;
 struct image *rd = find_image ( "initrd" );
 const char *cmd = image->cmdline ? image->cmdline : "";
 unsigned long low = ~0UL, high = 0, entry = 0, address, bootstr, initrd = 0;
 unsigned int i;
 unsigned char *hdr;
 of_cell_t a[4];
 int rc;
 if ( ( rc = native_elf_parse ( image, &elf ) ) != 0 ) return rc;
 for ( i = 0; i < elf.count; i++ ) {
  struct native_elf_segment *s = &elf.segment[i];
  if ( s->physical < 0x400000 || s->physical >= 0x10000000 || s->memsz > 0x10000000 - s->physical ) return -ERANGE;
  if ( s->physical < low ) low = s->physical;
  if ( s->physical + s->memsz > high ) high = s->physical + s->memsz;
  if ( elf.entry >= s->virtual && elf.entry - s->virtual < s->filesz )
   entry = s->physical + elf.entry - s->virtual;
 }
 if ( !entry || entry > high - 56 ) return -ENOEXEC;
 a[0] = low; a[1] = high - low; a[2] = 0;
 if ( of_call ( "claim", 3, 1, a ) || a[0] != low ) return -ENOMEM;
 for ( i = 0; i < elf.count; i++ ) {
  struct native_elf_segment *s = &elf.segment[i];
  memcpy ( (void *)s->physical, image->data + s->offset, s->filesz );
  memset ( (void *)( s->physical + s->filesz ), 0, s->memsz - s->filesz );
 }
 hdr = (void *)entry;
 if ( memcmp ( hdr + 8, "HdrS", 4 ) || be16_to_cpu ( *(uint16_t *)(hdr+16) ) < 0x301 ) { rc = -ENOEXEC; goto release; }
 bootstr = be64_to_cpu ( *(uint64_t *)( hdr + 40 ) );
 if ( bootstr < low || bootstr > high - 8 ||
      be32_to_cpu ( *(uint32_t *)bootstr ) > high - bootstr - 8 ||
      strlen ( cmd ) >= be32_to_cpu ( *(uint32_t *)bootstr ) ) { rc = -ERANGE; goto release; }
 if ( !*(uint32_t *)(bootstr+4) ) {
  strcpy ( (char *)(bootstr+8), cmd );
  *(uint32_t *)(bootstr+4) = cpu_to_be32 ( 1 );
 }
 if ( rd ) {
  if ( rd->len > 128UL*1024*1024 ) { rc = -E2BIG; goto release; }
  for ( initrd = 0x10000000; initrd < 0x20000000; initrd += 0x01000000 ) {
   a[0] = initrd; a[1] = rd->len; a[2] = 0;
   if ( !of_call ( "claim", 3, 1, a ) && a[0] == initrd ) break;
  }
  if ( initrd == 0x20000000 ) { initrd = 0; rc = -ENOMEM; goto release; }
  memcpy ( (void *)initrd, rd->data, rd->len );
  *(uint64_t *)(hdr+48) = cpu_to_be64 ( initrd + 0x400000 );
  *(uint32_t *)(hdr+28) = cpu_to_be32 ( rd->len );
 }
 printf ( "Linux SPARC: entry=%lx initrd=%ld command=%s\n", entry, (unsigned long)( rd ? rd->len : 0 ), cmd );
 shutdown_boot ();
 for ( address = low; address < high; address += 32 )
  __asm__ volatile ( "flush %0" : : "r" ( address ) : "memory" );
 __asm__ volatile ( "membar #Sync" ::: "memory" );
 ( (void (*)(unsigned long,unsigned long,unsigned long,unsigned long,unsigned long))entry ) ( 0, 0, 0, 0, of_client_entry () );
 __builtin_unreachable ();
release:
 if ( initrd ) { a[0] = initrd; a[1] = rd->len; of_call ( "release", 2, 0, a ); }
 a[0] = low; a[1] = high-low; of_call ( "release", 2, 0, a );
 return rc;
}
struct image_type sparc_linux_image __image_type ( PROBE_NORMAL ) = {
 .name = "Linux/SPARC", .probe = sparc_linux_probe, .exec = sparc_linux_exec,
};
#endif
