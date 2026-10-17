/* Bounded, endian-neutral ELF decoding for native platform boot adapters. */
FILE_LICENCE ( GPL2_OR_LATER );
FILE_SECBOOT ( FORBIDDEN );
#include <string.h>
#include <errno.h>
#include <ipxe/image.h>
#include <ipxe/native_elf.h>
static uint64_t number ( const unsigned char *p, unsigned int bytes, int le ) {
 uint64_t n = 0;
 unsigned int i;
 for ( i = 0; i < bytes; i++ ) n = ( n << 8 ) | p[le ? bytes - 1 - i : i];
 return n;
}
/* A canonical NetBSD identity note in a bounded ELF32 big-endian section
 * table. Read fields bytewise: downloaded offsets need not be aligned. */
int native_elf_netbsd32 ( struct image *image ) {
 const unsigned char *data=image->data, *section;
 uint32_t offset, size, start;
 unsigned int count, i;
 static const unsigned char ident[20]={0,0,0,7,0,0,0,4,0,0,0,1,'N','e','t','B','S','D',0,0};
 if ( image->len<52 || memcmp(data,"\177ELF",4) || data[4]!=1 || data[5]!=2 ) return 0;
 offset=number(data+32,4,0);count=number(data+48,2,0);
 if ( number(data+46,2,0)!=40 || count>128 || offset>image->len ||
      count>(image->len-offset)/40 ) return 0;
 for ( i=0; i<count; i++ ) {
  section=data+offset+i*40;
  if ( number(section+4,4,0)!=7 ) continue;
  start=number(section+16,4,0);size=number(section+20,4,0);
  if ( start>image->len || size>image->len-start ) return 0;
  if ( size>=24 && !memcmp(data+start,ident,sizeof(ident)) ) return 1;
 }
 return 0;
}
int native_elf_parse ( struct image *image, struct native_elf *elf ) {
 const unsigned char *data = image->data, *p;
 uint64_t off, entsize, count, i, j;
 unsigned int wide, le, found = 0;
 struct native_elf_segment *s, *old;
 memset ( elf, 0, sizeof ( *elf ) );
 if ( image->len < 52 || memcmp ( data, "\177ELF", 4 ) ||
      ( data[4] != 1 && data[4] != 2 ) ||
      ( data[5] != 1 && data[5] != 2 ) || data[6] != 1 ) return -ENOEXEC;
 wide = ( data[4] == 2 ); le = ( data[5] == 1 );
 if ( image->len < ( wide ? 64U : 52U ) || number ( data + 16, 2, le ) != 2 ||
      number ( data + 20, 4, le ) != 1 ) return -ENOEXEC;
 elf->bits = wide ? 64 : 32; elf->little = le;
 elf->machine = number ( data + 18, 2, le );
 elf->entry = number ( data + 24, wide ? 8 : 4, le );
 off = number ( data + ( wide ? 32 : 28 ), wide ? 8 : 4, le );
 entsize = number ( data + ( wide ? 54 : 42 ), 2, le );
 count = number ( data + ( wide ? 56 : 44 ), 2, le );
 if ( !count || count > NATIVE_ELF_SEGMENTS || entsize != ( wide ? 56U : 32U ) ||
      off > image->len || count > ( image->len - off ) / entsize ) return -ENOEXEC;
 for ( i = 0; i < count; i++ ) {
  p = data + off + i * entsize;
  if ( number ( p, 4, le ) != 1 ) continue;
  s = &elf->segment[elf->count];
  s->offset = number ( p + ( wide ? 8 : 4 ), wide ? 8 : 4, le );
  s->virtual = number ( p + ( wide ? 16 : 8 ), wide ? 8 : 4, le );
  s->physical = number ( p + ( wide ? 24 : 12 ), wide ? 8 : 4, le );
  s->filesz = number ( p + ( wide ? 32 : 16 ), wide ? 8 : 4, le );
  s->memsz = number ( p + ( wide ? 40 : 20 ), wide ? 8 : 4, le );
  if ( s->filesz > s->memsz || s->offset > image->len ||
       s->filesz > image->len - s->offset || s->memsz > ~0ULL - s->physical ||
       s->memsz > ~0ULL - s->virtual ) return -ENOEXEC;
  for ( j = 0; j < elf->count; j++ ) {
   old = &elf->segment[j];
   if ( s->memsz && old->memsz && s->physical < old->physical + old->memsz &&
        old->physical < s->physical + s->memsz ) return -ENOEXEC;
  }
  if ( elf->entry >= s->virtual && elf->entry - s->virtual < s->filesz ) found = 1;
  elf->count++;
 }
 return ( found && elf->count ) ? 0 : -ENOEXEC;
}
