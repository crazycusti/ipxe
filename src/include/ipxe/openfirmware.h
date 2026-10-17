#ifndef _IPXE_OPENFIRMWARE_H
#define _IPXE_OPENFIRMWARE_H
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
/* Native firmware cells: 64-bit on SPARC V9, 32-bit on PowerPC clients. */
#ifdef __powerpc64__
typedef uint32_t of_cell_t;
typedef int32_t of_scell_t;
#else
typedef unsigned long of_cell_t;
typedef long of_scell_t;
#endif
struct of_args {
	of_cell_t service, nargs, nret, args[12];
};
int of_call ( const char *name, unsigned int nargs, unsigned int nret,
	      of_cell_t *cells );
of_cell_t of_find ( const char *path );
int of_prop ( of_cell_t node, const char *name, void *buf, unsigned int len );
#endif
