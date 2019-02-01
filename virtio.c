#include "types.h"
#include "defs.h"
#include "pci.h"
#include "virtio.h"

struct virtio_device* virtdevs[NVIRTIO] = {0};

void virtionet_negotiate(uint32* features) {
    // do not use control queue
    DISABLE_FEATURE(*features,VIRTIO_NET_CTRL_VQ);

    DISABLE_FEATURE(*features,VIRTIO_NET_GUEST_TSO4);
    DISABLE_FEATURE(*features,VIRTIO_NET_GUEST_TSO6);
    DISABLE_FEATURE(*features,VIRTIO_NET_GUEST_UFO);
    DISABLE_FEATURE(*features,VIRTIO_NET_MRG_RXBUF);
    DISABLE_FEATURE(*features,VIRTIO_EVENT_IDX);

    ENABLE_FEATURE(*features,VIRTIO_NET_CSUM);
}

/*
 * Allocates a virtio device given the base address of memory mapped region.
 */
int alloc_virt_dev(struct pci_device* dev, uint64 bar, uint32 offset) {
    int fd;
    struct virtio_device *vdev = (struct virtio_dev*)kalloc();
    vdev->base = dev->bar_base[bar] + offset;
    vdev->size = dev->bar_size[bar];
    vdev->irq = dev->irq_line;
    vdev->conf = (struct virtio_pci_common_cfg*)vdev->base;

    for(fd = 0; fd < NVIRTIO; vdev++) {
        if (virtdevs[fd] == 0) {
            virtdevs[fd] = vdev;
            break;
            // return fd;
        }
    }


    // TODO: move initialization to netcard conf
    return conf_virtio(fd, &virtionet_negotiate);
}


/*
 * Configures a virtio device.
 *
 * http://docs.oasis-open.org/virtio/virtio/v1.0/cs04/virtio-v1.0-cs04.html#x1-490001
 *
 * This functions accepts a function pointer to a negotiate function. This
 * means that different virtio devices can customize feature negotiation.
 */
int conf_virtio(int fd, void (*negotiate)(uint32 *features)) {
    struct virtio_device* dev = virtdevs[fd];

    uint64 flag = VIRTIO_ACKNOWLEDGE;
    dev->conf->device_status = flag;
    flag |= VIRTIO_DRIVER;
    dev->conf->device_status = flag;

    uint32 features = dev->conf->device_feature;
    // device specific feature negotiation
    negotiate(&features);
    dev->conf->driver_feature = features;

    flag |= VIRTIO_FEATURES_OK;
    dev->conf->device_status = flag;
    uint64 val = dev->conf->device_status;
    cprintf("Reding back status: %x\n", val);

    if (val & VIRTIO_FEATURES_OK == 0) {
        cprintf("Feature negotiation failed");
        return -1;
    }

    flag |= VIRTIO_DRIVER_OK;
    dev->conf->device_status = flag;
    return 0;
}
