#include "types.h"
#include "defs.h"
#include "virtio.h"

struct virtio_device* virtdevs[NVIRTIO] = {0};

/*
 * Allocates a virtio device given the base address of memory mapped region.
 */
int alloc_virt_dev(uint64 base, uint32 size) {
    int fd;
    struct virtio_device *dev = (struct virtio_dev*)kalloc();
    dev->base = base;
    dev->size = size;
    dev->conf = (struct virtio_pci_common_cfg*)base;

    for(fd = 0; fd < NVIRTIO; dev++) {
        if (virtdevs[fd] == 0) {
            virtdevs[fd] = dev;
            return fd;
        }
    }

    return -1;
}

