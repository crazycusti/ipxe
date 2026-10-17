/* Experimental IEEE 1275 platform services. */
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <byteswap.h>
#include <stddef.h>
#include <stdio.h>
#include <ipxe/console.h>
#include <ipxe/timer.h>

#include <ipxe/openfirmware.h>
static long ( *of_entry ) ( struct of_args * );
static of_cell_t of_stdin, of_stdout;
static int of_pending = -1;
unsigned long of_client_entry ( void ) { return (unsigned long)of_entry; }
#if defined ( __powerpc__ ) || defined ( __powerpc64__ )
unsigned long of_client_msr ( void ) {
#ifdef __powerpc64__
 extern unsigned long of_firmware_msr;
 return of_firmware_msr;
#else
 unsigned long msr;
 __asm__ volatile ( "mfmsr %0" : "=r" ( msr ) );
 return msr;
#endif
}
#endif

#ifdef __powerpc64__
extern int of_arch_invoke ( struct of_args *, unsigned long );
#endif

int of_call ( const char *name, unsigned int nargs, unsigned int nret,
	      of_cell_t *cells )
{
	struct of_args a;
	unsigned int i;
	if ( !of_entry || nargs > 12 || nret > 12 - nargs )
		return -1;
	a.service = (unsigned long)name;
	a.nargs = nargs;
	a.nret = nret;
	for ( i = 0; i < nargs; i++ )
		a.args[i] = cells[i];
	for ( ; i < nargs + nret; i++ )
		a.args[i] = 0;
	#ifdef __powerpc64__
	if ( (unsigned long)&a > 0xffffffffUL || (unsigned long)name > 0xffffffffUL )
		return -1;
	a.service = cpu_to_be32 ( a.service );
	a.nargs = cpu_to_be32 ( a.nargs );
	a.nret = cpu_to_be32 ( a.nret );
	for ( i = 0; i < nargs + nret; i++ )
		a.args[i] = cpu_to_be32 ( a.args[i] );
	if ( of_arch_invoke ( &a, (unsigned long)of_entry ) )
		return -1;
	for ( i = 0; i < nret; i++ )
		cells[i] = be32_to_cpu ( a.args[nargs + i] );
	return 0;
#else
	if ( of_entry ( &a ) )
		return -1;
	for ( i = 0; i < nret; i++ )
		cells[i] = a.args[nargs + i];
	return 0;
#endif
}
of_cell_t of_find ( const char *path )
{
	of_cell_t a[1] = { (unsigned long)path };
	if ( of_call ( "finddevice", 1, 1, a ) )
		return 0;
	return a[0];
}
int of_prop ( of_cell_t node, const char *name, void *buf, unsigned int len )
{
	of_cell_t a[4] = { node, (unsigned long)name, (unsigned long)buf, len };
	if ( of_call ( "getprop", 4, 1, a ) )
		return -1;
	return a[0];
}
static void of_putchar ( int c )
{
	char b = c;
	of_cell_t a[3] = { of_stdout, (unsigned long)&b, 1 };
	of_call ( "write", 3, 1, a );
}
static int of_iskey ( void )
{
	unsigned char c;
	of_cell_t a[3] = { of_stdin, (unsigned long)&c, 1 };
	if ( of_pending >= 0 )
		return 1;
	if ( !of_call ( "read", 3, 1, a ) && a[0] == 1 ) {
		of_pending = c;
		return 1;
	}
	return 0;
}
static int of_getchar ( void )
{
	int c;
	while ( !of_iskey () ) {
	}
	c = of_pending;
	of_pending = -1;
	return c;
}
struct console_driver of_console __console_driver = {
	.putchar = of_putchar,
	.getchar = of_getchar,
	.iskey = of_iskey,
	.usage = CONSOLE_USAGE_ALL,
};
static unsigned long of_ticks ( void )
{
	of_cell_t a[1] = { 0 };
	if ( of_call ( "milliseconds", 0, 1, a ) )
		return 0;
	return ( (unsigned long)a[0] * TICKS_PER_SEC ) / 1000;
}
static void of_delay ( unsigned long us )
{
	unsigned long start = of_ticks (),
		      ticks = ( us * TICKS_PER_SEC + 999999 ) / 1000000;
	while ( of_ticks () - start < ticks ) {
	}
}
struct timer of_timer __timer ( TIMER_NORMAL ) = {
	.name = "ieee1275",
	.currticks = of_ticks,
	.udelay = of_delay,
};
extern int main ( void );
void of_platform_start ( long ( *entry ) ( struct of_args * ) )
{
	of_cell_t chosen;
	of_entry = entry;
	chosen = of_find ( "/chosen" );
	if ( !chosen || chosen == (of_cell_t)-1 )
		return;
	{
		unsigned char buf[sizeof ( of_cell_t )];
		int n, i;
		n = of_prop ( chosen, "stdout", buf, sizeof ( buf ) );
		if ( n != 4 && n != sizeof ( of_cell_t ) )
			return;
		for ( i = 0; i < n; i++ )
			of_stdout = ( of_stdout << 8 ) | buf[i];
		n = of_prop ( chosen, "stdin", buf, sizeof ( buf ) );
		if ( n != 4 && n != sizeof ( of_cell_t ) )
			return;
		for ( i = 0; i < n; i++ )
			of_stdin = ( of_stdin << 8 ) | buf[i];
	}
	printf ( "\nIEEE1275 " _S2 ( ARCH ) " platform entry ready\n" );
	main ();
}
