/* Linux/Alpha entry using the established aboot parameter-page contract. */
FILE_LICENCE ( GPL2_OR_LATER );
FILE_SECBOOT ( FORBIDDEN );
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ipxe/image.h>
#include <ipxe/native_elf.h>
#include <ipxe/init.h>
#include <ipxe/uaccess.h>
extern char _srm_start[], _srm_stack_top[];
extern void alpha_linux_enter ( unsigned long entry ) __attribute__ (( noreturn ));
extern int srm_ram_range ( unsigned long, size_t );
static int overlap ( unsigned long a, size_t n, unsigned long b, size_t m ) {
 return n && m && a < b + m && b < a + n;
}
static int alpha_linux_probe ( struct image *image ) {
 struct native_elf elf;
 int rc = native_elf_parse ( image, &elf );
 return rc ? rc : ( elf.machine == 0x9026 && elf.bits == 64 && elf.little ? 0 : -ENOEXEC );
}
static int alpha_linux_exec ( struct image *image ) {
 struct native_elf elf;
 struct image *rd = find_image ( "initrd" );
 const char *cmd = image->cmdline ? image->cmdline : "";
 unsigned long low = ~0UL, dest, parameter, entry = 0;
 unsigned int i;
 int rc;
 if ( ( rc = native_elf_parse ( image, &elf ) ) != 0 ) return rc;
 if ( strlen ( cmd ) >= 256 ) return -E2BIG;
 for ( i = 0; i < elf.count; i++ ) {
  struct native_elf_segment *s = &elf.segment[i];
  if ( ( s->physical & 0xfffffc0000000000ULL ) != 0xfffffc0000000000ULL ||
       s->memsz > (size_t)-1 ) return -ENOEXEC;
  dest = s->physical & 0x3ffffffffffUL;
  if ( dest < 0x300000 || !srm_ram_range ( dest, s->memsz ) ||
       overlap ( dest, s->memsz, virt_to_phys ( _srm_start ), _srm_stack_top - _srm_start ) ||
       overlap ( dest, s->memsz, virt_to_phys ( image->data ), image->len ) ||
       ( rd && overlap ( dest, s->memsz, virt_to_phys ( rd->data ), rd->len ) ) ) return -ERANGE;
  if ( dest < low ) low = dest;
  if ( elf.entry >= s->virtual && elf.entry - s->virtual < s->filesz )
   entry = (unsigned long)phys_to_virt ( dest + elf.entry - s->virtual );
 }
 if ( !entry || low < 0x6000 || !srm_ram_range ( low - 0x6000, 8192 ) ) return -ERANGE;
 parameter = (unsigned long)phys_to_virt ( low - 0x6000 );
 printf ( "Linux Alpha: entry=%lx initrd=%ld command=%s\n", entry, (unsigned long)( rd ? rd->len : 0 ), cmd );
 shutdown_boot ();
 for ( i = 0; i < elf.count; i++ ) {
  struct native_elf_segment *s = &elf.segment[i];
  void *p = phys_to_virt ( s->physical & 0x3ffffffffffUL );
  memcpy ( p, image->data + s->offset, s->filesz );
  memset ( p + s->filesz, 0, s->memsz - s->filesz );
 }
 memset ( (void *)parameter, 0, 8192 );
 memcpy ( (void *)parameter, cmd, strlen ( cmd ) + 1 );
 *(unsigned long *)( parameter + 0x100 ) = rd ? (unsigned long)rd->data : 0;
 *(unsigned long *)( parameter + 0x108 ) = rd ? rd->len : 0;
 __asm__ volatile ( "mb; imb" ::: "memory" );
 alpha_linux_enter ( entry );
}
struct image_type alpha_linux_image __image_type ( PROBE_NORMAL ) = {
 .name = "Linux/Alpha", .probe = alpha_linux_probe, .exec = alpha_linux_exec,
};
