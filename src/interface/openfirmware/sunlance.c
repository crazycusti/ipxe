/* Polled SBus Am7990 for SPARCstation-5. OpenPROM owns DMA mappings. */
FILE_LICENCE ( GPL2_OR_LATER );
#if defined ( __sparc__ ) && !defined ( __arch64__ )
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ipxe/netdevice.h>
#include <ipxe/ethernet.h>
#include <ipxe/if_ether.h>
#include <ipxe/iobuf.h>
#include <ipxe/device.h>
#include <ipxe/openfirmware.h>
#include <ipxe/timer.h>
extern void openprom_eval ( const char * );
#define LE 0x78c00000UL
#define LEDMA 0x78400010UL
#define RX_COUNT 4
#define PACKET 1536
struct le_desc { volatile uint16_t low, flags, length, count; };
struct le_dma {
 uint16_t mode; unsigned char mac[6]; uint16_t filter[4];
 uint16_t rx_low, rx_high, tx_low, tx_high;
 unsigned char padding[8];
 struct le_desc rx[RX_COUNT], tx;
 unsigned char receive[RX_COUNT][PACKET], transmit[PACKET];
};
_Static_assert ( sizeof ( struct le_dma ) <= 8192, "OpenPROM DMA pool bound" );
_Static_assert ( offsetof ( struct le_dma, rx ) % 8 == 0, "LANCE receive ring alignment" );
_Static_assert ( offsetof ( struct le_dma, tx ) % 8 == 0, "LANCE transmit ring alignment" );
struct sunlance {
 struct le_dma *memory;
 uint32_t bus, saved_ledma;
 unsigned int consumer;
 struct io_buffer *pending;
 int active;
};
static volatile unsigned long dma_result, bus_result;
static uint16_t mmio16 ( unsigned long address ) {
 uint16_t v; __asm__ volatile ( "lduha [%1] 0x20,%0" : "=r"(v) : "r"(address) : "memory" ); return v;
}
static void out16 ( unsigned long address, uint16_t v ) {
 __asm__ volatile ( "stha %0,[%1] 0x20; stbar" : : "r"(v),"r"(address) : "memory" );
}
static uint32_t mmio32 ( unsigned long address ) {
 uint32_t v; __asm__ volatile ( "lda [%1] 0x20,%0" : "=r"(v) : "r"(address) : "memory" ); return v;
}
static void out32 ( unsigned long address, uint32_t v ) {
 __asm__ volatile ( "sta %0,[%1] 0x20; stbar" : : "r"(v),"r"(address) : "memory" );
}
static uint16_t csr ( unsigned int reg ) { out16 ( LE+2, reg ); return mmio16 ( LE ); }
static void setcsr ( unsigned int reg, uint16_t value ) { out16 ( LE+2, reg ); out16 ( LE, value ); }
static uint32_t bus ( struct sunlance *nic, const void *ptr ) {
 return nic->bus + ( (const char *)ptr - (const char *)nic->memory );
}
static int le_open ( struct net_device *dev ) {
 struct sunlance *nic = dev->priv;
 struct le_dma *m;
 unsigned int i;
 char command[160];
 if ( !dma_result ) {
  snprintf ( command, sizeof ( command ), "h# 2000 (dma-alloc) h# %lx !", (unsigned long)&dma_result );
  openprom_eval ( command );
  DBGC ( nic, "SUNLANCE DMA allocation %lx\n", dma_result );
  if ( !dma_result || dma_result == ~0UL ) return -ENOMEM;
  snprintf ( command, sizeof ( command ), "h# %lx h# 2000 false (dma-map-in) h# %lx !", dma_result, (unsigned long)&bus_result );
  openprom_eval ( command );
 }
 if ( !bus_result || bus_result == ~0UL || ( bus_result & 0xffffff ) > 0x1000000 - sizeof ( *m ) ) return -ERANGE;
 nic->memory = m = (void *)dma_result; nic->bus = bus_result;
 memset ( m, 0, sizeof ( *m ) ); nic->consumer = 0;
 setcsr ( 0, 4 );
 nic->saved_ledma = mmio32 ( LEDMA + 12 );
 out32 ( LEDMA + 12, nic->bus & 0xff000000 ); nic->active = 1;
 for ( i = 0; i < ETH_ALEN; i++ ) m->mac[i] = dev->ll_addr[i ^ 1];
 for ( i = 0; i < RX_COUNT; i++ ) {
  uint32_t address = bus ( nic, m->receive[i] );
  m->rx[i].low = address; m->rx[i].length = -PACKET;
  m->rx[i].flags = ( ( address >> 16 ) & 255 ) | 0x8000;
 }
 m->rx_low = bus ( nic, m->rx ); m->rx_high = ( ( bus ( nic, m->rx ) >> 16 ) & 255 ) | ( 2 << 13 );
 m->tx_low = bus ( nic, &m->tx ); m->tx_high = ( bus ( nic, &m->tx ) >> 16 ) & 255;
 m->tx.low = bus ( nic, m->transmit ); m->tx.flags = ( bus ( nic, m->transmit ) >> 16 ) & 255;
 setcsr ( 1, nic->bus ); setcsr ( 2, (nic->bus >> 16) & 255 );
 setcsr ( 3, 4 ); setcsr ( 0, 1 );
 for ( i = 0; i < 1000; i++ ) { if ( csr ( 0 ) & 0x100 ) break; mdelay ( 1 ); }
 if ( i == 1000 ) { setcsr ( 0, 4 ); out32 ( LEDMA+12, nic->saved_ledma ); nic->active = 0; return -ETIMEDOUT; }
 setcsr ( 0, 0x102 ); netdev_link_up ( dev ); return 0;
}
static void le_close ( struct net_device *dev ) {
 struct sunlance *nic = dev->priv;
 if ( !nic->active ) return;
 setcsr ( 0, 4 ); out32 ( LEDMA+12, nic->saved_ledma ); nic->active = 0;
 if ( nic->pending ) { netdev_tx_complete_err ( dev, nic->pending, -ECANCELED ); nic->pending = NULL; }
}
static int le_transmit ( struct net_device *dev, struct io_buffer *iob ) {
 struct sunlance *nic = dev->priv;
 struct le_dma *m = nic->memory;
 size_t length = iob_len ( iob );
 if ( length > PACKET ) return -EMSGSIZE;
 if ( nic->pending ) return -ENOBUFS;
 memcpy ( m->transmit, iob->data, length );
 if ( length < ETH_ZLEN ) { memset ( m->transmit+length, 0, ETH_ZLEN-length ); length = ETH_ZLEN; }
 m->tx.length = -length; m->tx.count = 0; nic->pending = iob;
 __asm__ volatile ( "stbar" ::: "memory" );
 m->tx.flags = ( ( bus ( nic, m->transmit ) >> 16 ) & 255 ) | 0x8300;
 setcsr ( 0, 8 ); return 0;
}
static void le_poll ( struct net_device *dev ) {
 struct sunlance *nic = dev->priv;
 struct le_dma *m = nic->memory;
 unsigned int quota = RX_COUNT;
 if ( !nic->active ) return;
 if ( nic->pending && !(m->tx.flags & 0x8000) ) {
  netdev_tx_complete_err ( dev, nic->pending, m->tx.flags & 0x4000 ? -EIO : 0 ); nic->pending = NULL;
 }
 while ( quota-- ) {
  unsigned int index = nic->consumer % RX_COUNT, size;
  struct le_desc *d = &m->rx[index];
  struct io_buffer *iob;
  if ( d->flags & 0x8000 ) break;
  size = d->count & 0xfff;
  if ( (d->flags & 0x4300) == 0x300 && size >= ETH_HLEN+4 && size <= PACKET ) {
   iob = alloc_iob ( size-4 );
   if ( iob ) { memcpy ( iob_put ( iob, size-4 ), m->receive[index], size-4 ); netdev_rx ( dev, iob ); }
   else netdev_rx_err ( dev, NULL, -ENOMEM );
  } else netdev_rx_err ( dev, NULL, -EIO );
  d->length = -PACKET; d->count = 0;
  __asm__ volatile ( "stbar" ::: "memory" );
  d->flags = ( ( bus ( nic, m->receive[index] ) >> 16 ) & 255 ) | 0x8000;
  nic->consumer++;
 }
 setcsr ( 0, 0x700 );
}
static struct net_device_operations le_ops = { .open=le_open, .close=le_close, .transmit=le_transmit, .poll=le_poll };
static int le_probe ( struct root_device *root ) {
 struct net_device *dev;
 char name[64] = {0}; unsigned char idprom[32];
 of_cell_t node = of_find ( "/" ); int rc;
 of_prop ( node, "name", name, sizeof ( name )-1 );
 DBGC ( root, "SUNLANCE machine %s\n", name );
 if ( !strstr ( name, "SPARCstation-5" ) || of_prop ( node, "idprom", idprom, sizeof ( idprom ) ) != sizeof ( idprom ) ) return -ENODEV;
 dev = alloc_etherdev ( sizeof ( struct sunlance ) ); if ( !dev ) return -ENOMEM;
 netdev_init ( dev, &le_ops ); memcpy ( dev->hw_addr, idprom+2, ETH_ALEN );
 if ( is_zero_ether_addr ( dev->hw_addr ) || is_multicast_ether_addr ( dev->hw_addr ) ) { netdev_put ( dev ); return -EINVAL; }
 memcpy ( dev->ll_addr, dev->hw_addr, ETH_ALEN ); dev->dev = &root->dev;
 root->dev.desc.bus_type = BUS_TYPE_DT; root->dev.driver_name = "sunlance";
 rootdev_set_drvdata ( root, dev ); rc = register_netdev ( dev );
 if ( rc ) { netdev_nullify ( dev ); netdev_put ( dev ); }
 return rc;
}
static void le_remove ( struct root_device *root ) {
 struct net_device *dev = rootdev_get_drvdata ( root );
 unregister_netdev ( dev ); netdev_nullify ( dev ); netdev_put ( dev );
}
static struct root_driver le_driver = { .probe=le_probe, .remove=le_remove };
struct root_device sunlance_root __root_device = { .dev={.name="SBusLANCE"}, .driver=&le_driver };
#endif
