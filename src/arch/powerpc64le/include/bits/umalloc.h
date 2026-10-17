#ifndef _BITS_OFW_UMALLOC_H
#define _BITS_OFW_UMALLOC_H
FILE_LICENCE ( GPL2_OR_LATER );
#ifdef UMALLOC_OPENFIRMWARE
#define UMALLOC_PREFIX_ofw
#else
#define UMALLOC_PREFIX_ofw __ofw_
#endif
#endif
