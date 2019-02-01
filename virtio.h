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
    // Generic PCI field: PCI_CAP_ID_VNDR
    uint8 cap_vendor;
    //Generic PCI field: next ptr
    uint8 cap_next;
    // Generic PCI field: capability length
    uint8 cap_lenth;
    // Identifies the structure
    uint8 cfg_type;
    // Where to find it
    uint8 bar;
    // Pad to full dword
    uint8 padding[3];
    // Offset within bar
    uint32 offset;
    // Length of the structure, in bytes
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
    uint32 device_feature_select;     /* read-write */
    uint32 device_feature;            /* read-only for driver */
    uint32 driver_feature_select;     /* read-write */
    uint32 driver_feature;            /* read-write */
    uint16 msix_config;               /* read-write */
    uint16 num_queues;                /* read-only for driver */
    uint8 device_status;               /* read-write */
    uint8 config_generation;           /* read-only for driver */

    /* About a specific virtqueue. */
    uint16 queue_select;              /* read-write */
    uint16 queue_size;                /* read-write, power of 2, or 0. */
    uint16 queue_msix_vector;         /* read-write */
    uint16 queue_enable;              /* read-write */
    uint16 queue_notify_off;          /* read-only for driver */
    uint64 queue_desc;                /* read-write */
    uint64 queue_avail;               /* read-write */
    uint64 queue_used;                /* read-write */
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
};

#define NVIRTIO                         10

// Array of virtio devices
extern struct virtio_device* virtdevs[NVIRTIO];

#define VIRTIO_ACKNOWLEDGE              1
#define VIRTIO_DRIVER                   2
#define VIRTIO_FAILED                   128
#define VIRTIO_FEATURES_OK              8
#define VIRTIO_DRIVER_OK                4
#define VIRTIO_DEVICE_NEEDS_RESET       64

#define VIRTIO_NET_CSUM (0)
#define VIRTIO_NET_GUEST_CSUM (1)
#define VIRTIO_NET_CTRL_GUEST_OFFLOADS (2)
#define VIRTIO_NET_MAC (5)
#define VIRTIO_NET_GUEST_TSO4 (7)
#define VIRTIO_NET_GUEST_TSO6 (8)
#define VIRTIO_NET_GUEST_ECN (9)
#define VIRTIO_NET_GUEST_UFO (10)
#define VIRTIO_NET_HOST_TSO4 (11)
#define VIRTIO_NET_HOST_TSO6 (12)
#define VIRTIO_NET_HOST_ECN (13)
#define VIRTIO_NET_HOST_UFO (14)
#define VIRTIO_NET_MRG_RXBUF (15)
#define VIRTIO_NET_STATUS (16)
#define VIRTIO_NET_CTRL_VQ (17)
#define VIRTIO_NET_CTRL_RX (18)
#define VIRTIO_NET_CTRL_VLAN (19)
#define VIRTIO_NET_GUEST_ANNOUNCE (21)
#define VIRTIO_NET_MQ (22)
#define VIRTIO_NET_CTRL_MAC_ADDR (23)
#define VIRTIO_EVENT_IDX (29)
