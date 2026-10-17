/* Native narrow PA-RISC PDC/IODC platform services. */
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <stdio.h>
#include <ipxe/console.h>
#include <ipxe/timer.h>
#include <ipxe/profile.h>
#include <ipxe/malloc.h>
#include <ipxe/umalloc.h>

struct pdc_path {
	uint8_t flags, bus[6], module;
	uint32_t layers[6];
};
struct pdc_device {
	struct pdc_path path;
	uint32_t hpa, spa, io;
	uint16_t reserved, class_id;
};
extern volatile struct pdc_device pdc_console_device, pdc_keyboard_device;
extern volatile uint32_t pdc_cycles_per_10ms;
typedef int ( *iodc_fn ) ( uint32_t, uint32_t, uint32_t, const void *,
			   uint32_t *, uint32_t, void *, uint32_t, uint32_t,
			   uint32_t, uint32_t, uint32_t, uint32_t, uint32_t );
static uint32_t pdc_result[32] __attribute__ ( ( aligned ( 8 ) ) );
static unsigned char pdc_buffer[64] __attribute__ ( ( aligned ( 64 ) ) );
static int pending = -1;

static int pdc_io ( volatile struct pdc_device *dev, unsigned int op,
		    unsigned int size )
{
	iodc_fn call = (iodc_fn)dev->io;
	if ( !call )
		return -1;
	pdc_result[0] = 0;
	return call ( dev->hpa, op, dev->spa, (const void *)dev->path.layers,
		      pdc_result, 0, pdc_buffer, size, 0, 0, 0, 0, 0, 0 );
}
static void pdc_putchar ( int c )
{
	pdc_buffer[0] = c;
	pdc_io ( &pdc_console_device, 3, 1 );
}
static int pdc_iskey ( void )
{
	volatile struct pdc_device *input =
	    ( pdc_console_device.class_id == 7 ? &pdc_console_device
					       : &pdc_keyboard_device );
	if ( pending >= 0 )
		return 1;
	if ( !pdc_io ( input, 2, 1 ) && pdc_result[0] == 1 ) {
		pending = pdc_buffer[0];
		return 1;
	}
	return 0;
}
static int pdc_getchar ( void )
{
	int c;
	while ( !pdc_iskey () ) {
	}
	c = pending;
	pending = -1;
	return c;
}
struct console_driver pdc_console __console_driver = {
    .putchar = pdc_putchar,
    .getchar = pdc_getchar,
    .iskey = pdc_iskey,
    .usage = CONSOLE_USAGE_ALL,
};

/* Extend the 32-bit interval counter before converting to iPXE ticks.
 * PDC's boot CPU calibration, rather than an assumed QEMU frequency,
 * supplies the time scale. Called regularly by the event loop.
 */
static unsigned long pdc_ticks ( void )
{
	static uint32_t previous;
	static uint64_t elapsed;
	uint32_t current = profile_timestamp ();
	uint32_t scale = pdc_cycles_per_10ms;
	elapsed += (uint32_t)( current - previous );
	previous = current;
	return scale ? ( elapsed * TICKS_PER_SEC / ( (uint64_t)scale * 100 ) )
		     : 0;
}
static void pdc_delay ( unsigned long us )
{
	unsigned long start = pdc_ticks ();
	unsigned long ticks =
	    ( (uint64_t)us * TICKS_PER_SEC + 999999 ) / 1000000;
	while ( pdc_ticks () - start < ticks ) {
	}
}
struct timer pdc_timer __timer ( TIMER_NORMAL ) = {
    .name = "pdc",
    .currticks = pdc_ticks,
    .udelay = pdc_delay,
};

/* A bounded, image-owned heap avoids claiming memory still owned by PALO
 * or firmware. It is loaded/reserved as part of the ELF BSS segment.
 */
static unsigned char image_area[16 * 1024 * 1024]
    __attribute__ ( ( aligned ( 4096 ) ) );
static struct heap image_heap = {
    .blocks = LIST_HEAD_INIT ( image_heap.blocks ),
    .align = 16,
    .ptr_align = 4096,
};
static void *pdc_urealloc ( void *ptr, size_t size )
{
	return heap_realloc ( &image_heap, ptr, size );
}
PROVIDE_UMALLOC ( pdc, urealloc, pdc_urealloc );

extern int main ( void );
void pdc_platform_start ( void )
{
	heap_populate ( &image_heap, image_area, sizeof ( image_area ) );
	printf ( "\nPDC hppa platform entry ready\n" );
	main ();
}
