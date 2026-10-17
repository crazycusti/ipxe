/* Sun4m Linux/NetBSD ELF32 entry through the retained OpenPROM vector. */
FILE_LICENCE ( GPL2_OR_LATER );
FILE_SECBOOT ( FORBIDDEN );
#if defined ( __sparc__ ) && !defined ( __arch64__ )
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <byteswap.h>
#include <ipxe/image.h>
#include <ipxe/native_elf.h>
#include <ipxe/openfirmware.h>
#include <ipxe/umalloc.h>
#include <ipxe/init.h>
extern unsigned char sparc32_linux_copy[], sparc32_linux_copy_end[];
extern unsigned long openprom_vector ( void );
extern int openprom_linux_command ( const char * );
extern int openprom_ram_available ( uint32_t, uint32_t );
extern char _of_stack_top[];
struct sun4m_segment { uint32_t source, dest, filesz, memsz; };
struct sun4m_plan {
 uint32_t count, entry, vector, reserved;
 struct sun4m_segment segment[NATIVE_ELF_SEGMENTS+1];
 char command[256];
};
static int sun4m_probe ( struct image *image ) {
 struct native_elf elf;
 int rc = native_elf_parse ( image, &elf );
 return rc ? rc : ( elf.machine == 2 && elf.bits == 32 && !elf.little ? 0 : -ENOEXEC );
}
static int sun4m_exec ( struct image *image ) {
 struct native_elf elf;
 struct image *rd = find_image ( "initrd" );
 const char *cmd = image->cmdline ? image->cmdline : "";
 struct sun4m_plan *plan;
 unsigned char *hdr = NULL;
 unsigned long code, addr, initrd=0;
 size_t codesz = sparc32_linux_copy_end - sparc32_linux_copy;
 unsigned int i;
 char model[64] = {0};
 int rc, bsd;
 if ( ( rc = native_elf_parse ( image, &elf ) ) != 0 ) return rc;
 if ( elf.machine!=2 || elf.bits!=32 || elf.little ) return -ENOEXEC;
 bsd=native_elf_netbsd32(image);
 if ( strlen ( cmd ) >= sizeof ( plan->command ) ) return -E2BIG;
 of_prop ( of_find ( "/" ), "name", model, sizeof ( model )-1 );
 /* Only this low identity mapping has an emulator qualification fixture. */
 if ( !strstr ( model, "SPARCstation-5" ) ) return -ENOTSUP;
 for ( i = 0; i < elf.count; i++ ) {
  struct native_elf_segment *s = &elf.segment[i];
  uint64_t skip, offset;
  unsigned long limit=(unsigned long)_of_stack_top;
  if ( s->physical < 0xf0000000 || s->physical >= 0xf0000000+limit ||
       s->memsz > 0xf0000000+limit-s->physical ) return -ERANGE;
  s->physical -= 0xf0000000;
  skip = ( s->physical < 0x4000 ? 0x4000 - s->physical : 0 );
  if ( skip >= s->filesz ) return -ENOEXEC;
  s->physical += skip; s->virtual += skip; s->offset += skip;
  s->filesz -= skip; s->memsz -= skip;
  for ( offset = 0; offset + 24 <= s->filesz && offset < 0x40000; offset += 0x1000 ) {
   unsigned char *candidate = (void *)( image->data + s->offset + offset );
   if ( memcmp ( candidate, "HdrS", 4 ) ) continue;
   unsigned int version = ( candidate[8] << 8 ) | candidate[9];
   if ( hdr || version < 0x200 || version >= 0x300 ) return -ENOEXEC;
   hdr = candidate;
  }
 }
 if ( (!hdr && !bsd) || elf.entry < 0xf0004000 || elf.entry >= 0xf0000000+(unsigned long)_of_stack_top ) return -ENOEXEC;
 if ( bsd && rd ) return -ENOTSUP;
 if ( rd && rd->len > 16UL*1024*1024 ) return -E2BIG;
 plan = umalloc ( sizeof ( *plan ) + 4096 + codesz );
 if ( !plan ) { printf ( "Sun4m copy-plan allocation failed\n" ); return -ENOMEM; }
 /* Firmware allocations must leave the fixed low destination unoccupied. */
 if ( rd ) {
  for ( initrd=0x1000000; initrd<0x4000000; initrd+=0x100000 )
   if ( openprom_ram_available ( initrd, rd->len ) ) break;
  if ( initrd==0x4000000 ) { rc=-ENOMEM; goto release; }
 }
 memset ( plan, 0, sizeof ( *plan ) );
 plan->entry = elf.entry - 0xf0000000; plan->vector = openprom_vector ();
 strcpy ( plan->command, cmd );
 for ( i = 0; i < elf.count; i++ ) {
  struct native_elf_segment *s = &elf.segment[i];
  plan->segment[i] = (struct sun4m_segment){ (unsigned long)image->data+s->offset, s->physical, s->filesz, s->memsz };
 }
 plan->count = elf.count;
 if ( hdr ) {
  uint32_t value = cpu_to_be32 ( initrd ); memcpy ( hdr+16, &value, sizeof ( value ) );
  value = cpu_to_be32 ( rd ? rd->len : 0 ); memcpy ( hdr+20, &value, sizeof ( value ) );
 }
 if ( rd ) plan->segment[plan->count++] = (struct sun4m_segment){ (unsigned long)rd->data, initrd, rd->len, rd->len };
 if ( openprom_linux_command ( plan->command ) ) { rc=-ENOTSUP; goto release; }
 code = ( (unsigned long)(plan+1) + 4095 ) & ~4095UL;
 memcpy ( (void *)code, sparc32_linux_copy, codesz );
 printf ( "%s Sun4m: entry=%lx initrd=%ld command=%s\n", bsd ? "NetBSD" : "Linux", (unsigned long)plan->entry, (unsigned long)(rd ? rd->len : 0), cmd );
 shutdown_boot ();
 for ( addr=code; addr<code+codesz; addr+=8 ) __asm__ volatile ( "flush %0" : : "r"(addr) : "memory" );
 __asm__ volatile ( "stbar" ::: "memory" );
 ( (void (*)(struct sun4m_plan *))code ) ( plan );
 __builtin_unreachable ();
release:
 ufree ( plan ); return rc;
}
struct image_type sun4m_linux_image __image_type ( PROBE_NORMAL ) = {
 .name="ELF/Sun4m", .probe=sun4m_probe, .exec=sun4m_exec,
};
#endif
