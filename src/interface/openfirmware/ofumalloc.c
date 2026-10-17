/* External image buffers allocated through IEEE 1275 claim/release. */
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <string.h>
#include <ipxe/openfirmware.h>
#include <ipxe/umalloc.h>
#include <ipxe/io.h>
#define OF_PAGE ( 1UL << PAGE_SHIFT )
#define OF_ALLOC_MAGIC 0x4f464d45UL
struct of_allocation {
	unsigned long magic;
	size_t total;
	size_t used;
};
static void *of_urealloc ( void *ptr, size_t size )
{
	struct of_allocation *old = NULL, *fresh;
	of_cell_t a[3];
	size_t total;
	if ( ptr == NOWHERE )
		ptr = NULL;
	if ( ptr ) {
		old = ptr - OF_PAGE;
		if ( old->magic != OF_ALLOC_MAGIC || old->total < OF_PAGE ||
		     old->used > old->total - OF_PAGE )
			return NULL;
	}
	if ( !size ) {
		if ( old ) {
			a[0] = (unsigned long)old;
			a[1] = old->total;
			of_call ( "release", 2, 0, a );
		}
		return NOWHERE;
	}
	if ( size > ( (size_t)-1 ) - OF_PAGE - ( OF_PAGE - 1 ) )
		return NULL;
	total = ( ( size + OF_PAGE - 1 ) & ~( OF_PAGE - 1 ) ) + OF_PAGE;
	a[0] = 0;
	a[1] = total;
	a[2] = OF_PAGE;
	if ( of_call ( "claim", 3, 1, a ) || !a[0] || a[0] == (of_cell_t)-1 )
		return NULL;
	/* Preserve the public allocator's page-alignment guarantee. */
	if ( ( a[0] & ( OF_PAGE - 1 ) ) ||
	     a[0] > ( (of_cell_t)-1 ) - total ) {
		a[1] = total;
		of_call ( "release", 2, 0, a );
		return NULL;
	}
	fresh = (void *)a[0];
	fresh->magic = OF_ALLOC_MAGIC;
	fresh->total = total;
	fresh->used = size;
	if ( old ) {
		memcpy ( (void *)fresh + OF_PAGE, ptr,
			 ( size < old->used ? size : old->used ) );
		a[0] = (unsigned long)old;
		a[1] = old->total;
		of_call ( "release", 2, 0, a );
	}
	return (void *)fresh + OF_PAGE;
}
PROVIDE_UMALLOC ( ofw, urealloc, of_urealloc );
