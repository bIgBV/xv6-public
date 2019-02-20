#include "pci.h"
#include "defs.h"
#include "virtio.h"
#include "pcireg.h"
#include "memlayout.h"

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

    for(int i = PCI_CFG_BAR_OFF; i < PCI_CFG_BAR_END; i += width) {
        width = 4;
        uint32 prev = confread32(dev, i);

        conf_write32(dev, i, 0xffffffff);
        uint32 new_val = confread32(dev, i);

        if (new_val == 0) {
            continue;
        }

        uint32 reg = PCI_MAPREG_NUM(i);

        uint32 size;
        uint64 base;
        if (PCI_MAPREG_TYPE(new_val) == PCI_MAPREG_TYPE_MEM) {
            if (PCI_MAPREG_MEM_TYPE(new_val) == PCI_MAPREG_MEM_TYPE_64BIT) {
                width = 8;
                cprintf("64bit BAR\n");
            }
            cprintf("Mem region addr: %p\n", prev);

            size = PCI_MAPREG_MEM_SIZE(new_val);
            base = (uint64)P2V(PCI_MAPREG_MEM_ADDR(prev));
            cprintf("mem region %d: %d bytes at 0x%p\n",
                    reg, size, base);
        } else {
            size = PCI_MAPREG_IO_SIZE(new_val);
            base = prev;
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
        cprintf("Current iobase: %x\n", dev->iobase);
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

        int dev;
        if (type == VIRTIO_PCI_CAP_COMMON_CFG) {
            if ((dev = alloc_virt_dev(device, bar, offset)) != 0)
                return -1;
            cprintf("We have a virtio device: %x\n", virtdevs[dev]->conf->device_feature);
        }
        cap_pointer = next;
    }

    return 0;
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

            // populate BAR information.
            read_dev_bars(&individual_fn);

            if (PCI_VENDOR_ID(individual_fn.dev_id) == VIRTIO_VENDOR_ID) {
                // we have a virtio device
                if (PCI_DEVICE_ID(individual_fn.dev_id) > VIRTIO_DEVICE_ID_BASE) {
                    cprintf("We have a virtio v1.0 sepc device.\n");
                }
                else {
                    switch(PCI_DEVICE_ID(individual_fn.dev_id)) {
                        case (T_NETWORK_CARD):
                            cprintf("We have a transitional network device.\n");
                            config_virtio_net(&individual_fn);
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
