#ifndef _HPPA_BITS_UMALLOC_H
#define _HPPA_BITS_UMALLOC_H
FILE_LICENCE ( GPL2_OR_LATER );
#ifdef UMALLOC_PDC
#define UMALLOC_PREFIX_pdc
#else
#define UMALLOC_PREFIX_pdc __pdc_
#endif
#endif
