/* IEEE 1275 Ethernet using firmware packet I/O. */
FILE_LICENCE ( GPL2_OR_LATER );
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ipxe/openfirmware.h>
#include <ipxe/device.h>
#include <ipxe/ethernet.h>
#include <ipxe/if_ether.h>
#include <ipxe/netdevice.h>
#include <ipxe/iobuf.h>

struct ofnet_nic {
	of_cell_t handle;
};
static int ofnet_open ( struct net_device *dev )
{
	netdev_link_up ( dev );
	return 0;
}
static void ofnet_close ( struct net_device *dev __unused )
{
}
static int ofnet_tx ( struct net_device *dev, struct io_buffer *iob )
{
	struct ofnet_nic *nic = dev->priv;
	of_cell_t a[3] = { nic->handle, (unsigned long)iob->data,
			   iob_len ( iob ) };
	if ( of_call ( "write", 3, 1, a ) || a[0] != iob_len ( iob ) )
		return -EIO;
	netdev_tx_complete ( dev, iob );
	return 0;
}
static void ofnet_poll ( struct net_device *dev )
{
	struct ofnet_nic *nic = dev->priv;
	unsigned int quota = 4;
	while ( quota-- ) {
		struct io_buffer *iob = alloc_iob ( 2048 );
		of_cell_t a[3];
		if ( !iob )
			return;
		a[0] = nic->handle;
		a[1] = (unsigned long)iob->data;
		a[2] = 2048;
		if ( of_call ( "read", 3, 1, a ) || (of_scell_t)a[0] <= 0 ) {
			free_iob ( iob );
			return;
		}
		if ( a[0] > 2048 || a[0] < ETH_HLEN ) {
			free_iob ( iob );
			netdev_rx_err ( dev, NULL, -EMSGSIZE );
			return;
		}
		iob_put ( iob, a[0] );
		netdev_rx ( dev, iob );
	}
}
static struct net_device_operations ofnet_ops = {
	.open = ofnet_open,
	.close = ofnet_close,
	.transmit = ofnet_tx,
	.poll = ofnet_poll,
};
static int ofnet_probe ( struct root_device *root )
{
	struct net_device *dev;
	struct ofnet_nic *nic;
	of_cell_t a[1];
	of_cell_t node;
	int rc;
	static const char * const aliases[] = { "net", "enet", "ethernet" };
	unsigned int i;
	for ( i = 0; i < sizeof ( aliases ) / sizeof ( aliases[0] ); i++ ) {
		a[0] = (unsigned long)aliases[i];
		if ( !of_call ( "open", 1, 1, a ) && a[0] && a[0] != (of_cell_t)-1 )
			break;
	}
	if ( i == sizeof ( aliases ) / sizeof ( aliases[0] ) ) {
		printf ( "IEEE1275 net open failed\n" );
		return -ENODEV;
	}
	dev = alloc_etherdev ( sizeof ( *nic ) );
	if ( !dev ) {
		of_call ( "close", 1, 0, a );
		return -ENOMEM;
	}
	netdev_init ( dev, &ofnet_ops );
	nic = dev->priv;
	nic->handle = a[0];
	if ( of_call ( "instance-to-package", 1, 1, a ) ) {
		rc = -ENODEV;
		goto fail;
	}
	node = a[0];
	printf ( "IEEE1275 network handle %lx node %lx\n", (unsigned long)nic->handle, (unsigned long)node );
	if ( of_prop ( node, "local-mac-address", dev->hw_addr, ETH_ALEN ) !=
		 ETH_ALEN &&
	     of_prop ( node, "mac-address", dev->hw_addr, ETH_ALEN ) !=
		 ETH_ALEN ) {
		rc = -ENODEV;
		goto fail;
	}
	if ( is_zero_ether_addr ( dev->hw_addr ) ||
	     is_multicast_ether_addr ( dev->hw_addr ) ) {
		rc = -EINVAL;
		goto fail;
	}
	memcpy ( dev->ll_addr, dev->hw_addr, ETH_ALEN );
	dev->dev = &root->dev;
	root->dev.desc.bus_type = BUS_TYPE_DT;
	root->dev.driver_name = "ofnet";
	rootdev_set_drvdata ( root, dev );
	rc = register_netdev ( dev );
	if ( rc )
		goto fail;
	netdev_link_up ( dev );
	printf ( "IEEE1275 Ethernet %s registered\n", dev->name );
	return 0;
fail:
	printf ( "IEEE1275 network probe failed %s\n", strerror ( rc ) );
	a[0] = nic->handle;
	of_call ( "close", 1, 0, a );
	netdev_nullify ( dev );
	netdev_put ( dev );
	return rc;
}
static void ofnet_remove ( struct root_device *root )
{
	struct net_device *dev = rootdev_get_drvdata ( root );
	struct ofnet_nic *nic = dev->priv;
	of_cell_t a[1] = { nic->handle };
	unregister_netdev ( dev );
	of_call ( "close", 1, 0, a );
	netdev_nullify ( dev );
	netdev_put ( dev );
}
static struct root_driver ofnet_driver = {
	.probe = ofnet_probe,
	.remove = ofnet_remove,
};
struct root_device ofnet_root __root_device = {
	.dev = { .name = "IEEE1275" },
	.driver = &ofnet_driver,
};
