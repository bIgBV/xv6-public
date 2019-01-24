#include "pci.h"
#include "virtio.h"
#include "defs.h"
#include "pcireg.h"
#include "memlayout.h"


/*
 * Reads the register at offset `off` in the PCI config space of the device
 * `dev`
 */
static uint32 conf_read32(struct pci_device *dev, uint32 off) {
    return PCI_CONF_READ32(dev->bus->bus_num, dev->dev, dev->func, off);
}

uint32 conf_read16(struct pci_device *dev, uint32 off) {
    return PCI_CONF_READ16(dev->bus->bus_num, dev->dev, dev->func, off);
}

uint32 conf_read8(struct pci_device *dev, uint32 off) {
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

static void udelay(unsigned int u)
{
	unsigned int i;
	for (i = 0; i < u; i++)
		inb(0x84);
}

/*
 * To determine the amount of address space needed by a PCI device,
 * you must save the original value of the BAR, write a value of all 1's
 * to the register, then read it back. The amount of memory can then be
 * determined by masking the information bits, performing a
 * bitwise NOT ('~' in C), and incrementing the value by 1.
 *
 * http://wiki.osdev.org/PCI
 */
int read_common_cfg(struct pci_device* device, uint8 bar, uint32 offset) {
    // The bar passed to the functino is just the offset. Each bar is 4 bytes
    // long. Multiply by 4 for the right offset.
    uint8 confbar = bar * 4;
    uint32 prev = conf_read32(device, confbar + PCI_CFG_BAR_OFF);

    conf_write32(device, confbar + PCI_CFG_BAR_OFF, 0xffffffff);
    uint32 new_val = conf_read32(device, confbar + PCI_CFG_BAR_OFF);

    if (new_val == 0) {
        return -1;
    }

    uint32 size;
    uint64 base;
    if (PCI_MAPREG_TYPE(new_val) == PCI_MAPREG_TYPE_MEM) {
        if (PCI_MAPREG_MEM_TYPE(new_val) == PCI_MAPREG_MEM_TYPE_64BIT)
            cprintf("64bit BAR\n");

        size = PCI_MAPREG_MEM_SIZE(new_val);
        base = P2V(PCI_MAPREG_MEM_ADDR(prev));
        cprintf("mem region %d: %d bytes at 0x%x\n",
                bar, size, base);
    } else {
        size = PCI_MAPREG_IO_SIZE(new_val);
        base = PCI_MAPREG_IO_ADDR(prev);
        cprintf("io region %d: %d bytes at 0x%x\n",
                bar, size, base);
    }

    conf_write32(device, confbar + PCI_CFG_BAR_OFF, prev);

    struct virtio_pci_common_cfg* conf = (struct virtio_pci_common_cfg*)base;

    uint32 previous_features = conf->device_feature;
    cprintf("Features: %x\n", previous_features);
    conf->device_feature_select = 0x1;

    while(previous_features == conf->device_feature) {}
    cprintf("New features: %x\n", conf->device_feature);

    previous_features = conf->device_feature;

    conf->device_feature_select = 0x0;

    while(previous_features == conf->device_feature) {}
    cprintf("New features: %x\n", conf->device_feature);

    return 0;
}

int config_virtio_net(struct pci_device* device) {
    uint8 vndr_id, next;

    // Enable memory and IO addressing
    conf_write32(
            device, PCI_COMMAND_STATUS_REG,
            PCI_COMMAND_IO_ENABLE
            | PCI_COMMAND_MEM_ENABLE
            | PCI_COMMAND_MASTER_ENABLE);

    // uint32 status_register = conf_read32(device, PCI_COMMAND_STATUS_REG);
    // uint16 status = PCI_STATUS(status);
    uint16 status = conf_read16(device, PCI_COMMAND_STATUS_REG+2);

    // Check if the device has a capabalities list
    if (!(status & PCI_COMMAND_CAPABALITES_LIST)) {
        return -1;
    }

    uint32 cap_register = conf_read32(device, PCI_CAP_REG);
    uint8 cap_pointer = PCI_CAP_POINTER(cap_register) & PCI_CAP_MASK;

    while (cap_pointer) {
        vndr_id = conf_read8(device, cap_pointer + PCI_CAP_TYPE);
        next = conf_read8(device, cap_pointer + PCI_CAP_NEXT) & PCI_CAP_MASK;
        uint8 type = conf_read8(device, cap_pointer + PCI_CAP_CFG_TYPE);
        uint8 bar = conf_read8(device, cap_pointer + PCI_CAP_BAR);
        uint32 offset = conf_read32(device, cap_pointer + PCI_CAP_OFF);
        uint32 len = conf_read32(device, cap_pointer + PCI_CAP_OFF+4);

        if (type == VIRTIO_PCI_CAP_COMMON_CFG) {
            if (read_common_cfg(device, bar, offset) != 0)
                return -1;
        }
        cap_pointer = next;
    }
}

static int pci_enumerate(struct pci_bus *bus) {
    int num_dev = 0;
    struct pci_device dev_fn;
    memset(&dev_fn, 0, sizeof(dev_fn));
    dev_fn.bus = bus;

    for (dev_fn.dev = 0; dev_fn.dev < PCI_MAX_DEVICES; dev_fn.dev++) {
        uint32 bhcl = conf_read32(&dev_fn, PCI_BHLC_REG);

        if (PCI_HDRTYPE_TYPE(bhcl) > 1) {
            continue;
        }

        num_dev++;

        struct pci_device fn = dev_fn;

        // Configure the device functions
        for (fn.func = 0; fn.func < (PCI_HDRTYPE_MULTIFN(bhcl) ? 8 : 1); fn.func++) {
            struct pci_device individual_fn = fn;
            individual_fn.dev_id = conf_read32(&fn, PCI_ID_REG);

            // 0xffff is an invalid vendor ID
            if (PCI_VENDOR_ID(individual_fn.dev_id) == 0xffff) {
                continue;
            }

            uint32 intr = conf_read32(&individual_fn, PCI_INTERRUPT_REG);
            individual_fn.irq_line = PCI_INTERRUPT_LINE(intr);
            individual_fn.irq_pin = PCI_INTERRUPT_PIN(intr);

            individual_fn.dev_class = conf_read32(&individual_fn, PCI_CLASS_REG);

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
