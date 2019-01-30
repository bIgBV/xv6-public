#include "types.h"
#include "defs.h"
#include "pci.h"
#include "virtio.h"

struct virtio_device* virtdevs[NVIRTIO] = {0};

/*
 * Allocates a virtio device given the base address of memory mapped region.
 */
int alloc_virt_dev(struct pci_device* dev, uint64 bar, uint32 offset) {
    int fd;
    struct virtio_device *vdev = (struct virtio_dev*)kalloc();
    vdev->base = dev->bar_base[bar] + offset;
    vdev->size = dev->bar_size[bar];
    vdev->conf = (struct virtio_pci_common_cfg*)vdev->base;

    for(fd = 0; fd < NVIRTIO; vdev++) {
        if (virtdevs[fd] == 0) {
            virtdevs[fd] = vdev;
            return fd;
        }
    }

    return -1;
}


int conf_virtio_dev(int fd) {

}

/*
 * Initialize a virtio net device associated with the given fd.
 */
int init_virtio_net(int fd) {

}
