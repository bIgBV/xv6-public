#include "types.h"
#include "x86.h"

/*
 * PCI software configuration space access mechanism 1
 *
 * https://wiki.osdev.org/PCI#Configuration_Space_Access_Mechanism_.231
 */
#define PCI_CONFIG_ADDR             0xCF8
#define PCI_CONFIG_DATA             0xCFC



/*
 * The PCI specification specifies 8 bits for the bus identifier, 5 bits for
 * the device and 3 bits for selecting a particular function. This is the BDF
 * (Bus Device Function) address of a PCI device
 */
#define PCI_MAX_DEVICES 32

// Forward declaration
struct pci_device;


/*
 * Defines a PCI bus. Only considering it's parent bridge and the bus number
 * for now
 */
struct pci_bus {
    // A bridge is a type of PCI device which just forwards all transactions if
    // they are destined for any device behind it. The PCI host bridge is the
    // common interface for all other PCI devices/buses on a system
    struct pci_device *parent_bridge;
    uint32 bus_num;
};

struct pci_device {
    // The bus this device is on
    struct pci_bus *bus;

    // The device number of the device on the bus
    uint32 dev;
    // The function represented by this struct
    uint32 func;

    uint32 dev_id;
    uint32 dev_class;

    uint8 reg_base[6];
    uint8 reg_size[6];
    uint8 irq_line;
    uint8 irq_pin;
};


/*
 * represents a single capability struct in the capabilities linked list
 */
typedef struct pci_cap {
    /* Generic PCI field: PCI_CAP_ID_VNDR */
    uint8 cap_vndr_id;
    /* Generic PCI field: next ptr*/
    uint8 cap_next;
    /* Generic PCI field: capability length */
    uint8 cap_len;
    /* Identifies the structure. */
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
    uint8 cfg_type;
    /* Where to find it. */
    uint8 bar;
    /* Pad to full dword. */
    uint8 padding[3];
    /* Offset within bar. */
    uint32 offset;
    /* Length of the structure, in bytes. */
    uint8 length;
} cap;

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
 * Macro to create the 32 bit register values for a PCI transaction. `off` here
 * is the register value in the PCI config space of the device specified by
 * `dev` on the bus specificed by `bus` and the function specified by `fn`.
 *
 * The MSB of the 32 bit value needs to be 1 in order to mark it as
 * a configuration transaction
 */
#define PCI_FORMAT(bus, dev, fn, off) ({0x80000000 | bus << 16 | dev << 11 | fn << 8 | off;})

#define PCI_CONF_READ8(bus, dev, func, reg) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	inb(PCI_CONFIG_DATA+((reg)&3)))
#define PCI_CONF_READ16(bus, dev, func, reg) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	inw(PCI_CONFIG_DATA+((reg)&2)))
#define PCI_CONF_READ32(bus, dev, func, reg) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	inl(PCI_CONFIG_DATA))

#define PCI_CONF_WRITE8(bus, dev, func, reg, val) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	outb(PCI_CONFIG_DATA+((reg)&3), (val)))
#define PCI_CONF_WRITE_16(bus, dev, func, reg, val) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	outw(PCI_CONFIG_DATA+((reg)&2), (val)))
#define PCI_CONF_WRITE32(bus, dev, func, reg, val) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	outl(PCI_CONFIG_DATA, (val)))


/*
 * Class codes of PCI devices at their offsets
 */
const char* PCI_CLASSES[] = {
    "Unclassified",
    "Mass storage controller",
    "Network controller",
    "Display controller",
    "Multimedia controller",
    "Memory controller",
    "Bridge device",
    "Simple communication controller",
    "Base system peripheral",
    "Input device controller",
    "Docking station",
    "Processor",
    "Serial bus controller",
    "Wireless controller",
    "Intelligent controller",
    "Satellite communication controller",
    "Encryption controller",
    "Signal processing controller"
};
