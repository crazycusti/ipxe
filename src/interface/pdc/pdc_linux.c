/* Narrow PDC Linux ELF entry, including PA2.0 wide kernel decompressors. */
FILE_LICENCE ( GPL2_OR_LATER );
FILE_SECBOOT ( FORBIDDEN );
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ipxe/image.h>
#include <ipxe/native_elf.h>
#include <ipxe/umalloc.h>
#include <ipxe/init.h>
extern unsigned char pdc_linux_copy[], pdc_linux_copy_end[];
struct pdc_linux_segment { uint32_t source, dest, filesz, memsz; };
struct pdc_linux_parameters {
 uint32_t count, entry, free, cmd, rd_start, rd_end, reserved[2];
 struct pdc_linux_segment segment[NATIVE_ELF_SEGMENTS];
 char command[1024];
};
static int pdc_linux_probe ( struct image *image ) {
 struct native_elf elf;
 int rc = native_elf_parse ( image, &elf );
 return rc ? rc : ( elf.machine == 15 && !elf.little ? 0 : -ENOEXEC );
}
static int pdc_linux_exec ( struct image *image ) {
 struct native_elf elf;
 struct image *rd = find_image ( "initrd" );
 const char *cmd = image->cmdline ? image->cmdline : "";
 struct pdc_linux_parameters *p;
 void ( *enter ) ( struct pdc_linux_parameters * );
 unsigned long code, address, entry = 0;
 size_t codesz = pdc_linux_copy_end - pdc_linux_copy;
 unsigned int i, j;
 int rc;
 if ( ( rc = native_elf_parse ( image, &elf ) ) != 0 ) return rc;
 if ( strlen ( cmd ) >= sizeof ( p->command ) ) return -E2BIG;
 if ( *(volatile uint32_t *)0x380 > 0x80000 ) return -ERANGE;
 /* The external heap is backed by confirmed contiguous PAGE0 RAM. */
 if ( (unsigned long)image->data < 0x04000000 ||
      ( rd && (unsigned long)rd->data < 0x04000000 ) ) return -ENOMEM;
 for ( i = 0; i < elf.count; i++ ) {
  struct native_elf_segment *s = &elf.segment[i];
  /* PALO/Linux translates the high kernel virtual window to low RAM. */
  if ( ( s->physical >= 0x10000000 && s->physical < 0x20000000 ) ||
       ( s->physical >= 0x40000000 && s->physical < 0x50000000 ) ) s->physical &= 0x0fffffff;
  if ( s->physical < 0x10000 || s->physical >= 0x04000000 ||
       s->memsz > 0x04000000 - s->physical ) return -ERANGE;
  if ( s->physical < 0x80400 && s->physical + s->memsz > 0x80000 ) return -ERANGE;
  for ( j = 0; j < i; j++ ) {
   struct native_elf_segment *old = &elf.segment[j];
   if ( s->physical < old->physical + old->memsz && old->physical < s->physical + s->memsz ) return -ENOEXEC;
  }
  if ( elf.entry >= s->virtual && elf.entry - s->virtual < s->filesz )
   entry = s->physical + elf.entry - s->virtual;
 }
 if ( !entry ) return -ENOEXEC;
 p = umalloc ( sizeof ( *p ) + 4096 + codesz );
 if ( !p ) return -ENOMEM;
 if ( (unsigned long)p < 0x04000000 ) { ufree ( p ); return -ENOMEM; }
 memset ( p, 0, sizeof ( *p ) );
 p->count = elf.count; p->entry = entry;
 p->free = *(volatile uint32_t *)0x380;
 /* setup_cmdline runs before Linux maps all RAM. Its physical pointer
  * must be in the initial low-memory linear mapping. */
 strcpy ( (char *)0x80000, cmd ); p->cmd = 0x80000;
 p->rd_start = rd ? (unsigned long)rd->data : 0;
 p->rd_end = p->rd_start + ( rd ? rd->len : 0 );
 for ( i = 0; i < elf.count; i++ ) {
  struct native_elf_segment *s = &elf.segment[i];
  p->segment[i].source = (unsigned long)image->data + s->offset;
  p->segment[i].dest = s->physical;
  p->segment[i].filesz = s->filesz; p->segment[i].memsz = s->memsz;
 }
 code = ( (unsigned long)( p + 1 ) + 4095 ) & ~4095UL;
 memcpy ( (void *)code, pdc_linux_copy, codesz );
 printf ( "Linux PDC: entry=%lx initrd=%ld command=%s\n", entry, (unsigned long)( rd ? rd->len : 0 ), cmd );
 shutdown_boot ();
 for ( address = code; address < code + codesz; address += 32 )
  __asm__ volatile ( "fdc 0(%0); fic 0(%%sr0,%0)" : : "r" ( address ) : "memory" );
 __asm__ volatile ( "sync" ::: "memory" );
 enter = (void *)code; enter ( p );
 __builtin_unreachable ();
}
struct image_type pdc_linux_image __image_type ( PROBE_NORMAL ) = {
 .name = "Linux/PDC", .probe = pdc_linux_probe, .exec = pdc_linux_exec,
};
