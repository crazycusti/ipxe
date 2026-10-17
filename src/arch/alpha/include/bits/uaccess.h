FILE_LICENCE ( GPL2_OR_LATER );
#define UACCESS_PREFIX_srm
static inline __always_inline void *
UACCESS_INLINE ( srm, phys_to_virt ) ( physaddr_t p ) {
 return (void *)( p | 0xfffffc0000000000UL );
}
static inline __always_inline physaddr_t
UACCESS_INLINE ( srm, virt_to_phys ) ( volatile const void *p ) {
 return (unsigned long)p & 0x3ffffffffffUL;
}
