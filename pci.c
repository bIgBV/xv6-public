#include "pci.h"
#include "virtio.h"
#include "defs.h"
#include "pcireg.h"


/*
 * Reads the register at offset `off` in the PCI config space of the device
 * `dev`
 */
static uint32 conf_read32(struct pci_device *dev, uint32 off) {
    return PCI_CONF_READ32(dev->bus->bus_num, dev->dev, dev->func, off);
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

void config_virtio_net(struct pci_device* device) {
    uint32 cap_register = conf_read32(device, PCI_CAP_REG);
    uint8 cap_pointer = PCI_CAP_POINTER(cap_register) & PCI_CAP_MASK;

    cprintf("Got a value of: 0x%x\n", cap_pointer);

    // while (cap_pointer) {
    //     uint8 next = (PCI_CAP_POINTER(conf_read32(device, cap_pointer + 0x01)) & PCI_CAP_MASK);
    //     cprintf("Next pointer is at: %x\n", next);
    //     cap_pointer = next;
    // }
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
