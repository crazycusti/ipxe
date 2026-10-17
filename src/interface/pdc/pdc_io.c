FILE_LICENCE ( GPL2_OR_LATER );
#include <ipxe/io.h>
PROVIDE_IOAPI_INLINE ( ofw, readb );
PROVIDE_IOAPI_INLINE ( ofw, readw );
PROVIDE_IOAPI_INLINE ( ofw, readl );
PROVIDE_IOAPI_INLINE ( ofw, readq );
PROVIDE_IOAPI_INLINE ( ofw, writeb );
PROVIDE_IOAPI_INLINE ( ofw, writew );
PROVIDE_IOAPI_INLINE ( ofw, writel );
PROVIDE_IOAPI_INLINE ( ofw, writeq );
PROVIDE_IOAPI_INLINE ( ofw, mb );
PROVIDE_DUMMY_PIO ( ofw );
