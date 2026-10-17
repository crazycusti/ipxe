/* Native ELF decoding across word sizes and byte orders. */
FILE_LICENCE ( GPL2_OR_LATER );
#undef NDEBUG
#include <stdint.h>
#include <string.h>
#include <ipxe/image.h>
#include <ipxe/native_elf.h>
#include <ipxe/test.h>

static void put ( unsigned char *p, uint64_t value, unsigned int n, int le ) {
 unsigned int i;
 for ( i = 0; i < n; i++ ) { p[le ? i : n - 1 - i] = value; value >>= 8; }
}
static void native_elf_test_exec ( void ) {
 unsigned char data[256], good[256];
 struct image image = { .data = data, .len = sizeof ( data ) };
 struct native_elf elf;
 unsigned int wide, le, n, ph, offset, va, pa, fs, ms, i;
 for ( wide = 0; wide <= 1; wide++ ) for ( le = 0; le <= 1; le++ ) {
  n = wide ? 8 : 4; ph = wide ? 56 : 32;
  offset = wide ? 8 : 4; va = wide ? 16 : 8; pa = wide ? 24 : 12;
  fs = wide ? 32 : 16; ms = wide ? 40 : 20;
  memset ( data, 0, sizeof ( data ) );
  memcpy ( data, "\177ELF", 4 ); data[4] = wide + 1; data[5] = le ? 1 : 2; data[6] = 1;
  put ( data + 16, 2, 2, le ); put ( data + 18, 21, 2, le );
  put ( data + 20, 1, 4, le ); put ( data + 24, 0x1020, n, le );
  put ( data + ( wide ? 32 : 28 ), 64, n, le );
  put ( data + ( wide ? 54 : 42 ), ph, 2, le );
  put ( data + ( wide ? 56 : 44 ), 1, 2, le );
  put ( data + 64, 1, 4, le );
  put ( data + 64 + offset, 192, n, le );
  put ( data + 64 + va, 0x1000, n, le );
  put ( data + 64 + pa, 0x2000, n, le );
  put ( data + 64 + fs, 64, n, le ); put ( data + 64 + ms, 128, n, le );
  memcpy ( good, data, sizeof ( data ) );
  ok ( native_elf_parse ( &image, &elf ) == 0 );
  ok ( elf.bits == ( wide ? 64U : 32U ) && elf.little == le && elf.entry == 0x1020 );
  ok ( elf.count == 1 && elf.segment[0].physical == 0x2000 && elf.segment[0].memsz == 128 );
  /* Every truncated prefix must be rejected without reading beyond it. */
  for ( i = 0; i < sizeof ( data ); i++ ) {
   image.len = i; ok ( native_elf_parse ( &image, &elf ) != 0 );
  }
  image.len = sizeof ( data );
  put ( data + 64 + fs, 129, n, le ); ok ( native_elf_parse ( &image, &elf ) != 0 );
  memcpy ( data, good, sizeof ( data ) );
  put ( data + 64 + offset, ~(uint64_t)0, n, le ); ok ( native_elf_parse ( &image, &elf ) != 0 );
  memcpy ( data, good, sizeof ( data ) );
  put ( data + 24, 0x1040, n, le ); ok ( native_elf_parse ( &image, &elf ) != 0 ); /* BSS entry */
  memcpy ( data, good, sizeof ( data ) );
  put ( data + ( wide ? 32 : 28 ), ~(uint64_t)0, n, le ); ok ( native_elf_parse ( &image, &elf ) != 0 );
  memcpy ( data, good, sizeof ( data ) );
  put ( data + ( wide ? 56 : 44 ), 2, 2, le );
  memcpy ( data + 64 + ph, data + 64, ph );
  ok ( native_elf_parse ( &image, &elf ) != 0 ); /* overlapping destinations */
  put ( data + 64 + ph + pa, 0x2100, n, le );
  ok ( native_elf_parse ( &image, &elf ) == 0 );
  if ( wide ) {
   put ( data + 64 + pa, ~(uint64_t)0 - 1, n, le );
   ok ( native_elf_parse ( &image, &elf ) != 0 );
  }
 }
}
struct self_test native_elf_test __self_test = {
 .name = "native_elf", .exec = native_elf_test_exec,
};
