#include "types.h"


/*
 * Virtio netowork device queue descriptor entries header.
 */
struct virtio_net_hdr {
#define VIRTIO_NET_HDR_F_NEEDS_CSUM    1
    // Indicates weather or not `VIRTIO_NET_F_CSUM` has been negotiated.
    uint8 flags;
#define VIRTIO_NET_HDR_GSO_NONE        0
#define VIRTIO_NET_HDR_GSO_TCPV4       1
#define VIRTIO_NET_HDR_GSO_UDP         3
#define VIRTIO_NET_HDR_GSO_TCPV6       4
#define VIRTIO_NET_HDR_GSO_ECN      0x80
    // Indicates weather or not transport layer protocol frame fragmentation
    // has been negotiated.
    uint8 gso_type;
    uint16 hdr_len;
    uint16 gso_size;
    uint16 csum_start;
    uint16 csum_offset;
    uint16 num_buffers;
};
