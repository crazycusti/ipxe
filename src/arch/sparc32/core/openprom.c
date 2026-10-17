/* OpenPROM v2/v3 vector adapter for the shared firmware platform.
 * Vector layout follows Linux asm/openprom.h (GPL-2.0).
 */
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdint.h>
#include <string.h>
#include <ipxe/openfirmware.h>

struct prom_nodes {
	int ( *next ) ( int );
	int ( *child ) ( int );
	int ( *length ) ( int, const char * );
	int ( *get ) ( int, const char *, void * );
	void *unused[2];
};
struct prom_vector {
	uint32_t magic, version, revision, printrev;
	void *memory[3];
	struct prom_nodes *nodes;
	void *bootstr;
	void *v0dev[9];
	void *stdin_type, *stdout_type;
	int ( *getchar ) ( void );
	void ( *putchar ) ( int );
	int ( *nbgetchar ) ( void );
	int ( *nbputchar ) ( int );
	void ( *putstr ) ( const char *, int );
	void *reboot, *printf, *abort;
	volatile uint32_t *ticks;
	void *halt, *sync, *eval, *args, *enaddr;
	void *bootpath, *bootargs;
	int *stdin_handle, *stdout_handle;
	int ( *package ) ( int );
	void *( *alloc ) ( void *, unsigned int );
	void ( *free ) ( void *, unsigned int );
	void *map, *unmap;
	int ( *open ) ( const char * );
	void ( *close ) ( int );
	int ( *read ) ( int, void *, unsigned int );
	int ( *write ) ( int, const void *, unsigned int );
};
static struct prom_vector *prom;
void openprom_eval ( const char *command ) {
 ( (void (*)(const char *))prom->eval ) ( command );
}
#define PROM_CHOSEN 0x7ffffffe

/* OpenPROM predates /chosen: use its authoritative v2 boot arguments. */
static int prom_find ( const char *path )
{
	if ( !strcmp ( path, "/" ) )
		return prom->nodes->next ( 0 );
	if ( !strcmp ( path, "/chosen" ) )
		return PROM_CHOSEN;
	return -1;
}

static long prom_client ( struct of_args *a )
{
	of_cell_t *p = a->args;
	of_cell_t result = -1;
	const char *service = (void *)a->service;
	if ( !strcmp ( service, "finddevice" ) ) {
		result = prom_find ( (void *)p[0] );
	} else if ( !strcmp ( service, "getprop" ) ) {
		int length;
		if ( p[0] == PROM_CHOSEN ) {
			uint32_t handle;
			if ( p[3] < sizeof ( handle ) )
				return -1;
			if ( !strcmp ( (void *)p[1], "stdin" ) )
				handle = *prom->stdin_handle;
			else if ( !strcmp ( (void *)p[1], "stdout" ) )
				handle = *prom->stdout_handle;
			else
				return -1;
			memcpy ( (void *)p[2], &handle, sizeof ( handle ) );
			p[a->nargs] = sizeof ( handle );
			return 0;
		}
		length = prom->nodes->length ( p[0], (void *)p[1] );
		/* OpenPROM getprop has no size parameter. Check before calling.
		 */
		if ( length >= 0 && (unsigned int)length <= p[3] )
			result = prom->nodes->get ( p[0], (void *)p[1],
						    (void *)p[2] );
	} else if ( !strcmp ( service, "milliseconds" ) ) {
		result = *prom->ticks;
	} else if ( !strcmp ( service, "read" ) ) {
		result = prom->read ( p[0], (void *)p[1], p[2] );
	} else if ( !strcmp ( service, "write" ) ) {
		if ( p[0] == (unsigned int)*prom->stdout_handle ) {
			unsigned int i;
			for ( i = 0; i < p[2]; i++ )
				prom->putchar ( ( (unsigned char *)p[1] )[i] );
			result = p[2];
		} else {
			result = prom->write ( p[0], (void *)p[1], p[2] );
		}
	} else if ( !strcmp ( service, "claim" ) ) {
		result = (unsigned long)prom->alloc ( (void *)p[0], p[1] );
	} else if ( !strcmp ( service, "release" ) ) {
		prom->free ( (void *)p[0], p[1] );
	} else if ( !strcmp ( service, "open" ) ) {
		result = prom->open ( (void *)p[0] );
	} else if ( !strcmp ( service, "close" ) ) {
		prom->close ( p[0] );
	} else if ( !strcmp ( service, "instance-to-package" ) ) {
		result = prom->package ( p[0] );
	} else {
		return -1;
	}
	if ( a->nret )
		p[a->nargs] = result;
	return 0;
}

unsigned long openprom_profile_timestamp ( void )
{
	return *prom->ticks;
}

extern void of_platform_start ( long ( *entry ) ( struct of_args * ) );
void openprom_start ( struct prom_vector *vector )
{
	prom = vector;
	if ( !prom || prom->magic != 0x10010407 || prom->version < 2 ||
	     !prom->nodes || !prom->ticks || !prom->stdin_handle ||
	     !prom->stdout_handle )
		return;
	of_platform_start ( prom_client );
}
