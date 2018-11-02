#include "types.h"

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
const int VIRTIO_DEVICE_ID_BASE = 0x1040;
// A virtio device will always have this vendor id
const int VIRTIO_VENDOR_ID = 0x1AF4;

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
