#include "types.h"

#define DISABLE_FEATURE(v,feature) v &= ~(1<<feature)
#define ENABLE_FEATURE(v,feature) v |= (1<<feature)
#define HAS_FEATURE(v,feature) (v & (1<<feature))

// Virtio device IDs
enum VIRTIO_DEVICE {
    RESERVED = 0,
    NETWORK_CARD = 1,
    BLOCK_DEVICE = 2,
    CONSOLE = 3,
    ENTROPY_SOURCE = 4,
    MEMORY_BALLOONING_TRADITIONAL = 5,
    IO_MEMORY = 6,
    RPMSG = 7,
    SCSI_HOST = 8,
    NINEP_TRANSPORT = 9,
    MAC_80211_WLAN = 10,
    RPROC_SERIAL = 11,
    VIRTIO_CAIF = 12,
    MEMORY_BALLOON = 13,
    GPU_DEVICE = 14,
    CLOCK_DEVICE = 15,
    INPUT_DEVICE = 16
};

// Transitional vitio device ids
enum TRANSITIONAL_VIRTIO_DEVICE {
    T_NETWORK_CARD = 0X1000,
    T_BLOCK_DEVICE = 0X1001,
    T_MEMORY_BALLOONING_TRADITIONAL = 0X1002,
    T_CONSOLE = 0X1003,
    T_SCSI_HOST = 0X1004,
    T_ENTROPY_SOURCE = 0X1005,
    T_NINEP_TRANSPORT = 0X1006
};

// The PCI device ID of is VIRTIO_DEVICE_ID_BASE + virtio_device
#define VIRTIO_DEVICE_ID_BASE 0x1040
// A virtio device will always have this vendor id
#define VIRTIO_VENDOR_ID  0x1AF4

struct virtio_pci_cap {
    // Generic PCI field: PCI_CAP_ID_VNDR, 0
    uint8 cap_vendor;
    //Generic PCI field: next ptr, 1
    uint8 cap_next;
    // Generic PCI field: capability length, 2
    uint8 cap_len;
    // Identifies the structure, 3
    uint8 cfg_type;
    // Where to find it, 4
    uint8 bar;
    // Pad to full dword, 5
    uint8 padding[3];
    // Offset within bar, 8
    uint32 offset;
    // Length of the structure, in bytes, 12
    uint32 length;
};

/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG        1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG        2
/* ISR Status */
#define VIRTIO_PCI_CAP_ISR_CFG           3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG        4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG           5

struct virtio_pci_common_cfg {
    /* About the whole device. */
    uint32 device_feature_select;     /* read-write , 0*/
    uint32 device_feature;            /* read-only for driver , 4*/
    uint32 driver_feature_select;     /* read-write , 8*/
    uint32 driver_feature;            /* read-write , 12*/
    uint16 msix_config;               /* read-write , 20*/
    uint16 num_queues;                /* read-only for driver , 22*/
    uint8 device_status;               /* read-write , 24*/
    uint8 config_generation;           /* read-only for driver , 25*/

    /* About a specific virtqueue. */
    uint16 queue_select;              /* read-write , 26*/
    uint16 queue_size;                /* read-write, power of 2, or 0. , 28*/
    uint16 queue_msix_vector;         /* read-write , 30*/
    uint16 queue_enable;              /* read-write , 32*/
    uint16 queue_notify_off;          /* read-only for driver , 34*/
    uint64 queue_desc;                /* read-write , 36*/
    uint64 queue_avail;               /* read-write , 42*/
    uint64 queue_used;                /* read-write , 50*/
};


/*
 * A Virtio device.
 */
struct virtio_device {
    struct virtio_pci_common_cfg* conf;
    // memory mapped IO base
    uint64 base;
    // size of memory mapped region
    uint32 size;
    uint8 irq;
    uint32 iobase;
    uint64 memaddr;
};

#define NVIRTIO                         10

// Array of virtio devices
extern struct virtio_device* virtdevs[NVIRTIO];

/* The feature bitmap for virtio net */
#define VIRTIO_NET_F_CSUM	0	/* Host handles pkts w/ partial csum */
#define VIRTIO_NET_F_GUEST_CSUM	1	/* Guest handles pkts w/ partial csum */
#define VIRTIO_NET_F_MAC	5	/* Host has given MAC address. */
#define VIRTIO_NET_F_GSO	6	/* Host handles pkts w/ any GSO type */
#define VIRTIO_NET_F_GUEST_TSO4	7	/* Guest can handle TSOv4 in. */
#define VIRTIO_NET_F_GUEST_TSO6	8	/* Guest can handle TSOv6 in. */
#define VIRTIO_NET_F_GUEST_ECN	9	/* Guest can handle TSO[6] w/ ECN in. */
#define VIRTIO_NET_F_GUEST_UFO	10	/* Guest can handle UFO in. */
#define VIRTIO_NET_F_HOST_TSO4	11	/* Host can handle TSOv4 in. */
#define VIRTIO_NET_F_HOST_TSO6	12	/* Host can handle TSOv6 in. */
#define VIRTIO_NET_F_HOST_ECN	13	/* Host can handle TSO[6] w/ ECN in. */
#define VIRTIO_NET_F_HOST_UFO	14	/* Host can handle UFO in. */
#define VIRTIO_NET_F_MRG_RXBUF	15	/* Host can merge receive buffers. */
#define VIRTIO_NET_F_STATUS	16	/* virtio_net_config.status available */
#define VIRTIO_NET_F_CTRL_VQ	17	/* Control channel available */
#define VIRTIO_NET_F_CTRL_RX	18	/* Control channel RX mode support */
#define VIRTIO_NET_F_CTRL_VLAN	19	/* Control channel VLAN filtering */
#define VIRTIO_NET_F_CTRL_RX_EXTRA 20	/* Extra RX mode control support */
#define VIRTIO_NET_F_GUEST_ANNOUNCE 21	/* Guest can announce device on the network */
#define VIRTIO_F_EVENT_IDX        29  /* Support for avail_event and used_event fields */


#define VIRTIO_STATUS_RESET                    0
#define VIRTIO_STATUS_ACKNOWLEDGE              1
#define VIRTIO_STATUS_DRIVER                   2
#define VIRTIO_STATUS_FAILED                   128
#define VIRTIO_STATUS_FEATURES_OK              8
#define VIRTIO_STATUS_DRIVER_OK                4
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET       64

#define VIRTIO_HOST_F_OFF           0x0000
#define VIRTIO_GUEST_F_OFF          0x0004
#define VIRTIO_QADDR_OFF            0x0008

#define VIRTIO_QSIZE_OFF            0x000C
#define VIRTIO_QSEL_OFF             0x000E
#define VIRTIO_QNOTFIY_OFF          0x0010

#define VIRTIO_DEV_STATUS_OFF           0x0012
#define VIRTIO_ISR_STATUS_OFF           0x0013
#define VIRTIO_DEV_SPECIFIC_OFF         0x0014
/* if msi is enabled, device specific headers shift by 4 */
#define VIRTIO_MSI_ADD_OFF          0x0004
#define VIRTIO_STATUS_ACK           0x01
#define VIRTIO_STATUS_DRV           0x02
#define VIRTIO_STATUS_DRV_OK            0x04
#define VIRTIO_STATUS_FAIL          0x80

typedef struct
{
    uint64 address;
    uint32 length;
    uint16 flags;
    uint16 next;
} queue_buffer;

typedef struct
{
    uint16 flags;
    uint16 index;
    uint16 rings[];
} virtio_available;

typedef struct
{
    uint32 index;
    uint32 length;
} virtio_used_item;
typedef struct

{
    uint16 flags;
    uint16 index;
    virtio_used_item rings[];
} virtio_used;

typedef struct
{
    queue_buffer* buffers;
    virtio_available* available;
    virtio_used* used;
} virt_queue;
