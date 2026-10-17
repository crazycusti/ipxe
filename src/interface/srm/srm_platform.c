/* Native Alpha SRM console and cycle counter. */
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <stdio.h>
#include <ipxe/console.h>
#include <ipxe/timer.h>
#include <ipxe/profile.h>
#include <ipxe/malloc.h>
#include <ipxe/umalloc.h>
#include <ipxe/srm.h>
#include <ipxe/uaccess.h>
PROVIDE_UACCESS_INLINE ( srm, phys_to_virt );
PROVIDE_UACCESS_INLINE ( srm, virt_to_phys );
static unsigned long *dispatch;
static unsigned long cycle_freq;
static int pending = -1;
static int qemu_palcode;
extern unsigned long srm_dispatch ( void *, unsigned long, unsigned long,
 unsigned long, unsigned long, unsigned long );
static void srm_init ( void ) {
 volatile unsigned long *h = (void *)0x10000000UL;
 unsigned long *crb;
 if ( h[1] != 0x4857525042000000UL || h[5] != 8192 ||
      h[3] < 200 || h[24] < h[3] || h[24] > 0x1000000 )
  return;
 cycle_freq = h[14];
 srm_system_type = h[10];
 qemu_palcode = ( srm_system_type == 34 && (uint32_t)h[8] == 0x554d4551 );
 crb = (void *)( (unsigned long)h + h[24] );
 dispatch = (void *)crb[0];
}
static unsigned long callback ( unsigned long op, unsigned long a,
 unsigned long b, unsigned long c ) {
 return dispatch ? srm_dispatch ( dispatch, op, a, b, c, 0 ) : ~0UL;
}
static void srm_putchar ( int c ) {
 char b = c;
 callback ( 2, 0, (unsigned long)&b, 1 );
}
static int srm_iskey ( void ) {
 unsigned long r;
 if ( pending >= 0 ) return 1;
 /* QEMU PALcode's CRB_GETC blocks, unlike SRM. Poll its confirmed
  * Tsunami COM1 line status first so the event loop keeps running. */
 if ( qemu_palcode && !( *(volatile uint8_t *)0xfffffd01fc0003fdUL & 1 ) )
  return 0;
 r = callback ( 1, 0, 0, 0 );
 if ( !( r >> 63 ) ) { pending = r & 0xff; return 1; }
 return 0;
}
static int srm_getchar ( void ) {
 int c;
 while ( !srm_iskey () ) {}
 c = pending; pending = -1; return c;
}
struct console_driver srm_console __console_driver = {
 .putchar = srm_putchar, .getchar = srm_getchar, .iskey = srm_iskey,
 .usage = CONSOLE_USAGE_ALL,
};
static unsigned long srm_ticks ( void ) {
 static uint32_t previous;
 static uint64_t elapsed;
 uint32_t now = profile_timestamp ();
 elapsed += (uint32_t)( now - previous ); previous = now;
 return cycle_freq ? elapsed * TICKS_PER_SEC / cycle_freq : 0;
}
static void srm_delay ( unsigned long us ) {
 unsigned long start = srm_ticks ();
 unsigned long ticks = ( (uint64_t)us * TICKS_PER_SEC + 999999 ) / 1000000;
 while ( srm_ticks () - start < ticks ) {}
}
struct timer srm_timer __timer ( TIMER_NORMAL ) = {
 .name = "srm", .currticks = srm_ticks, .udelay = srm_delay,
};
/* A bounded, image-owned heap avoids claiming memory still owned by the loader
 * or firmware. It is loaded/reserved as part of the ELF BSS segment.
 */
static unsigned char image_area[16 * 1024 * 1024]
    __attribute__ ( ( aligned ( 4096 ) ) );
static struct heap image_heap = {
    .blocks = LIST_HEAD_INIT ( image_heap.blocks ),
    .align = 16,
    .ptr_align = 4096,
};
static void *srm_urealloc ( void *ptr, size_t size )
{
	return heap_realloc ( &image_heap, ptr, size );
}
PROVIDE_UMALLOC ( srm, urealloc, srm_urealloc );

/* HWRPB memory descriptors distinguish usable RAM from firmware pages. */
int srm_ram_range ( unsigned long start, size_t len ) {
 volatile unsigned long *h = (void *)0x10000000UL, *map, *cluster;
 unsigned long i, base, bytes;
 if ( !len || start > ~0UL - len || h[25] > 0x1000000 ) return 0;
 map = (void *)( (unsigned long)h + h[25] );
 if ( map[2] > 64 ) return 0;
 for ( i = 0; i < map[2]; i++ ) {
  cluster = map + 3 + i * 7;
  if ( cluster[6] || cluster[0] > ( ~0UL >> 13 ) || cluster[1] > ( ~0UL >> 13 ) ) continue;
  base = cluster[0] << 13; bytes = cluster[1] << 13;
  if ( start >= base && start - base <= bytes && len <= bytes - ( start - base ) ) return 1;
 }
 return 0;
}
extern int main ( void );
void srm_platform_start ( void )
{
	srm_init ();
	heap_populate ( &image_heap, image_area, sizeof ( image_area ) );
	if ( srm_ram_range ( 0x04000000, 0x08000000 ) )
		heap_populate ( &image_heap, phys_to_virt ( 0x04000000 ), 0x08000000 );
	printf ( "\nSRM alpha platform entry ready\n" );
	main ();
}
