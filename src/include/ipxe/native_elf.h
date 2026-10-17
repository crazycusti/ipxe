#ifndef _IPXE_NATIVE_ELF_H
#define _IPXE_NATIVE_ELF_H
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
struct image;
#define NATIVE_ELF_SEGMENTS 32
struct native_elf_segment {
 uint64_t offset, virtual, physical, filesz, memsz;
};
struct native_elf {
 uint64_t entry;
 unsigned int machine, bits, little, count;
 struct native_elf_segment segment[NATIVE_ELF_SEGMENTS];
};
int native_elf_parse ( struct image *, struct native_elf * );
int native_elf_netbsd32 ( struct image * );
#endif
