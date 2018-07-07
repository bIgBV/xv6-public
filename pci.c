#include "pci.h"
#include "x86.h"
#include "defs.h"
#include "pcireg.h"


/*
 * Reads the register at offset `off` in the PCI config space of the device
 * `dev`
 */
static uint32 conf_read(struct pci_device *dev, uint32 off) {
    uint32 value = PCI_FORMAT(dev->bus->bus_num, dev->dev, dev->func, off);
    outl(PCI_CONFIG_ADDR, value);

    return inl(PCI_CONFIG_DATA);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static void conf_write(struct pci_device *dev, uint32 off, uint32 value) {
    outl(PCI_CONFIG_ADDR, PCI_FORMAT(dev->bus->bus_num, dev->dev, dev->func, off));
    outl(PCI_CONFIG_DATA, value);
}

/*
 * Simple function to log PCI devices
 */
static void log_pci_device(struct pci_device *dev) {
    char *class = PCI_CLASSES[PCI_CLASS(dev->dev_class)];

    cprintf("PCI: %x:%x.%d: %x:%x: class: %x.%x (%s) irq: %d\n",
		dev->bus->bus_num, dev->dev, dev->func,
		PCI_VENDOR(dev->dev_id), PCI_PRODUCT(dev->dev_id),
		PCI_CLASS(dev->dev_class), PCI_SUBCLASS(dev->dev_class), class, dev->irq_line);
}

static int pci_enumerate(struct pci_bus *bus) {
    int num_dev = 0;
    struct pci_device dev_fn;
    memset(&dev_fn, 0, sizeof(dev_fn));
    dev_fn.bus = bus;

    for (dev_fn.dev = 0; dev_fn.dev < PCI_MAX_DEVICES; dev_fn.dev++) {
        uint32 bhcl = conf_read(&dev_fn, PCI_BHLC_REG);

        if (PCI_HDRTYPE_TYPE(bhcl) > 1) {
            continue;
        }

        num_dev++;

        struct pci_device fn = dev_fn;

        // Configure the device functions
        for (fn.func = 0; fn.func < (PCI_HDRTYPE_MULTIFN(bhcl) ? 8 : 1); fn.func++) {
            struct pci_device individual_fn = fn;
            individual_fn.dev_id = conf_read(&fn, PCI_ID_REG);

            // 0xffff is an invalid device ID
            if (PCI_VENDOR(individual_fn.dev_id) == 0xffff) {
                continue;
            }

            uint32 intr = conf_read(&individual_fn, PCI_INTERRUPT_REG);
            individual_fn.irq_line = PCI_INTERRUPT_LINE(intr);
            individual_fn.irq_pin = PCI_INTERRUPT_PIN(intr);

            individual_fn.dev_class = conf_read(&individual_fn, PCI_CLASS_REG);


            if (PCI_CLASS(individual_fn.dev_class) == PCI_NETWORK_CONTROLLER) {
                cprintf("We've got a network device!\n");
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
