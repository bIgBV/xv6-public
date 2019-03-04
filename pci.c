#include "pci.h"
#include "defs.h"
#include "virtio.h"
#include "pcireg.h"
#include "memlayout.h"

/*
 * Class codes of PCI devices at their offsets
 */
char* PCI_CLASSES[18] = {
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
    "Signal processing controller",
};

/*
 * Reads the register at offset `off` in the PCI config space of the device
 * `dev`
 */
uint32 confread32(struct pci_device *dev, uint32 off) {
    return PCI_CONF_READ32(dev->bus->bus_num, dev->dev, dev->func, off);
}

uint32 confread16(struct pci_device *dev, uint32 off) {
    return PCI_CONF_READ16(dev->bus->bus_num, dev->dev, dev->func, off);
}

uint32 confread8(struct pci_device *dev, uint32 off) {
    return PCI_CONF_READ8(dev->bus->bus_num, dev->dev, dev->func, off);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static void conf_write32(struct pci_device *dev, uint32 off, uint32 value) {
    return PCI_CONF_WRITE32(dev->bus->bus_num, dev->dev, dev->func, off, value);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static void conf_write16(struct pci_device *dev, uint32 off, uint16 value) {
    return PCI_CONF_WRITE16(dev->bus->bus_num, dev->dev, dev->func, off, value);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static void conf_write8(struct pci_device *dev, uint32 off, uint8 value) {
    return PCI_CONF_WRITE8(dev->bus->bus_num, dev->dev, dev->func, off, value);
}

/*
 * Simple function to log PCI devices
 */
static void log_pci_device(struct pci_device *dev) {
    char *class = PCI_CLASSES[PCI_CLASS(dev->dev_class)];
    uint32 bus_num = dev->bus->bus_num;
    uint32 dev_id = dev->dev;
    uint32 func = dev->func;
    uint32 vendor_id = PCI_VENDOR_ID(dev->dev_id);
    uint32 product = PCI_PRODUCT(dev->dev_id);
    uint32 class_name = PCI_CLASS(dev->dev_class);
    uint32 sub_class = PCI_SUBCLASS(dev->dev_class);
    uint32 irq_line = dev->irq_line;

    cprintf("PCI: %x:%x.%d: 0x%x:0x%x: class: %x.%x (%s) irq: %d\n",
		bus_num, dev_id, func,
		vendor_id, product,
		class_name, sub_class, class, irq_line);
}

/*
 * Iterate over the PCI device's Base Address Registers and store their
 * infomation on the deice indexed by the BAR index
 *
 * To determine the amount of address space needed by a PCI device,
 * you must save the original value of the BAR, write a value of all 1's
 * to the register, then read it back. The amount of memory can then be
 * determined by masking the information bits, performing a
 * bitwise NOT ('~' in C), and incrementing the value by 1.
 *
 * http://wiki.osdev.org/PCI
 */
int read_dev_bars(struct pci_device* dev) {
    uint8 width;
    uint8 modern_mem_bar = 0;

    for(int i = PCI_CFG_BAR_OFF, j = 0; i < PCI_CFG_BAR_END; i += width, j += 1) {
        modern_mem_bar = 0;

        cprintf("Reading bar %d for dev class: %s\n", j, PCI_CLASSES[PCI_CLASS(dev->dev_class)]);
        width = 4;
        uint64 prev = confread32(dev, i);

        conf_write32(dev, i, 0xffffffff);
        uint32 new_val = confread32(dev, i);

        if (new_val == 0) {
            continue;
        }

        uint32 reg = PCI_MAPREG_NUM(i);

        uint32 size = 0;
        uint64 base = 0;
        if (PCI_MAPREG_TYPE(prev) == PCI_MAPREG_TYPE_MEM) {
            if (PCI_MAPREG_MEM_TYPE(new_val) == PCI_MAPREG_MEM_TYPE_64BIT) {
                width = 8;
                cprintf("64bit BAR\n");
                modern_mem_bar = 1;
            }

            if (modern_mem_bar == 1) {
                uint64 part_two = confread32(dev, i + 4); // Each bar is 4 bytes wide.
                uint64 modern_bar = (prev & 0xFFFFFFF0) + ((part_two & 0xFFFFFFFF) << 32);
                dev->membase = (uint64)P2V(modern_bar);
            }

            size = PCI_MAPREG_MEM_SIZE(new_val);
            base = (uint64)P2V(PCI_MAPREG_MEM_ADDR(prev));
            cprintf("mem region %d: %d bytes at 0x%p\n",
                    reg, size, base);
        } else {
            size = PCI_MAPREG_IO_SIZE(new_val);
            base = prev & 0xFFFFFFFC;
            dev->iobase = base;
            cprintf("io region %d: %d bytes at 0x%p\n",
                    reg, size, base);
        }

        dev->bar_base[reg] = base;
        dev->bar_size[reg] = size;

        if (size && !base)
			cprintf("PCI device %x:%x.%d (%x:%x) "
				"may be misconfigured: "
				"region %d: base 0x%x, size %d\n",
				dev->bus->bus_num, dev->dev, dev->func,
				PCI_VENDOR_ID(dev->dev_id), PCI_PRODUCT(dev->dev_id),
				reg, base, size);
    }

    return 0;
}


void log_pci_cap(uint8 type, uint8 offset) {
    switch (type) {
        case VIRTIO_PCI_CAP_COMMON_CFG:
            cprintf("cap: VIRTIO_PCI_CAP_COMMON_CFG offset: %d\n", offset);
            break;
        case VIRTIO_PCI_CAP_NOTIFY_CFG:
            cprintf("cap: VIRTIO_PCI_CAP_NOTIFY_CFG offset: %d\n", offset);
            break;
        case VIRTIO_PCI_CAP_ISR_CFG:
            cprintf("cap: VIRTIO_PCI_CAP_ISR_CFG offset: %d\n", offset);
            break;
        case VIRTIO_PCI_CAP_DEVICE_CFG:
            cprintf("cap: VIRTIO_PCI_CAP_DEVICE offset: %d\n", offset);
            break;
        case VIRTIO_PCI_CAP_PCI_CFG:
            cprintf("cap: VIRTIO_PCI_CAP_PCI_CFG offset: %d\n", offset);
            break;
    }
}


int config_virtio_net(struct pci_device* device) {
    uint8 next;

    // Enable memory and IO addressing
    conf_write32(
            device, PCI_COMMAND_STATUS_REG,
            PCI_COMMAND_IO_ENABLE
            | PCI_COMMAND_MEM_ENABLE
            | PCI_COMMAND_MASTER_ENABLE);

    // uint32 status_register = confread32(device, PCI_COMMAND_STATUS_REG);
    // uint16 status = PCI_STATUS(status);
    uint16 status = confread16(device, PCI_COMMAND_STATUS_REG+2);

    // Check if the device has a capabalities list
    if (!(status & PCI_COMMAND_CAPABALITES_LIST)) {
        return -1;
    }

    uint32 cap_register = confread32(device, PCI_CAP_REG);
    uint8 cap_pointer = PCI_CAP_POINTER(cap_register) & PCI_CAP_MASK;

    while (cap_pointer) {
        next = confread8(device, cap_pointer + PCI_CAP_NEXT) & PCI_CAP_MASK;
        uint8 type = confread8(device, cap_pointer + PCI_CAP_CFG_TYPE);
        uint8 bar = confread8(device, cap_pointer + PCI_CAP_BAR);
        uint32 offset = confread32(device, cap_pointer + PCI_CAP_OFF);

        // Location of the given capability in the PCI config space.
        device->capabalities[type] = cap_pointer;

        log_pci_cap(type, cap_pointer);
        cap_pointer = next;
    }

    return 0;
}

void setup_bar_access(struct pci_device *dev, uint8 cap_pointer, uint8 width, uint32 field_offset) {

    // right now i'm only reading the common config bar
    // conf_write8(dev, cap_pointer + offsetof(struct virtio_pci_cap, bar), bar);
    conf_write8(dev, cap_pointer + offsetof(struct virtio_pci_cap, length), width);
    conf_write32(dev, cap_pointer + offsetof(struct virtio_pci_cap, offset), field_offset);

}


int conf_virtio_pci_cap(struct pci_device *dev) {
    uint8 cap_pointer = dev->capabalities[VIRTIO_PCI_CAP_PCI_CFG];

//    for (int i = 0; i < 32; i++) {
//        if (i % 4 == 0) {
//            cprintf("\n");
//        }
//        uint32 val = confread8(dev, cap_pointer + i);
//        cprintf("%p, ");
//    }
//
//    cprintf("\n");

    // Write to queue select
    setup_bar_access(dev, cap_pointer, 2, offsetof(struct virtio_pci_common_cfg, queue_select));
    conf_write16(dev, cap_pointer + sizeof(struct virtio_pci_cap), 0);

    // read back queue size
    setup_bar_access(dev, cap_pointer, 2, offsetof(struct virtio_pci_common_cfg, queue_size));
    uint32 bar_addr = confread32(dev, cap_pointer + sizeof(struct virtio_pci_cap));

    cprintf("Got bar window: %p\n", bar_addr);
}


static int pci_enumerate(struct pci_bus *bus) {
    int num_dev = 0;
    struct pci_device dev_fn;
    memset(&dev_fn, 0, sizeof(dev_fn));
    dev_fn.bus = bus;

    for (dev_fn.dev = 0; dev_fn.dev < PCI_MAX_DEVICES; dev_fn.dev++) {
        uint32 bhcl = confread32(&dev_fn, PCI_BHLC_REG);

        if (PCI_HDRTYPE_TYPE(bhcl) > 1) {
            continue;
        }

        num_dev++;

        struct pci_device fn = dev_fn;

        // Configure the device functions
        for (fn.func = 0; fn.func < (PCI_HDRTYPE_MULTIFN(bhcl) ? 8 : 1); fn.func++) {
            struct pci_device individual_fn = fn;
            individual_fn.dev_id = confread32(&fn, PCI_ID_REG);

            // 0xffff is an invalid vendor ID
            if (PCI_VENDOR_ID(individual_fn.dev_id) == 0xffff) {
                continue;
            }

            uint32 intr = confread32(&individual_fn, PCI_INTERRUPT_REG);
            individual_fn.irq_line = PCI_INTERRUPT_LINE(intr);
            individual_fn.irq_pin = PCI_INTERRUPT_PIN(intr);

            individual_fn.dev_class = confread32(&individual_fn, PCI_CLASS_REG);

            if (PCI_VENDOR_ID(individual_fn.dev_id) == VIRTIO_VENDOR_ID) {
                // we have a virtio device
                config_virtio_net(&individual_fn);

                if (PCI_DEVICE_ID(individual_fn.dev_id) > VIRTIO_DEVICE_ID_BASE) {
                    cprintf("We have a virtio v1.0 sepc device.\n");
                }
                else {

                    switch(PCI_DEVICE_ID(individual_fn.dev_id)) {
                        case (T_NETWORK_CARD):
                            cprintf("We have a transitional network device.\n");

                            // populate BAR information.
                            read_dev_bars(&individual_fn);

                            conf_virtio_pci_cap(&individual_fn);

                            break;
                        default:
                            cprintf("We have some other transitional device.\n");
                    }
                }
            }

            log_pci_device(&individual_fn);
        }
    }
    return num_dev;

}

int pciinit(void) {
    static struct pci_bus root;
    memset(&root, 0, sizeof(root));

    return pci_enumerate(&root);
}
