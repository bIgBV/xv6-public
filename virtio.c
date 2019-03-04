#include "types.h"
#include "defs.h"
#include "pci.h"
#include "virtio.h"

/*
* Table of all virtio devices in the machine
*
* This is populated by alloc_virt_dev
*/
struct virtio_device *virtdevs[NVIRTIO] = {0};

/*
 * Feature negotiation for a network device
 *
 * We are offloading checksuming to the device
 */
void virtionet_negotiate(uint32 *features) {
  // do not use control queue
  DISABLE_FEATURE(*features, VIRTIO_NET_F_CTRL_VQ);

  DISABLE_FEATURE(*features, VIRTIO_NET_F_GUEST_TSO4);
  DISABLE_FEATURE(*features, VIRTIO_NET_F_GUEST_TSO6);
  DISABLE_FEATURE(*features, VIRTIO_NET_F_GUEST_UFO);
  DISABLE_FEATURE(*features, VIRTIO_NET_F_MRG_RXBUF);
  DISABLE_FEATURE(*features, VIRTIO_F_EVENT_IDX);

  ENABLE_FEATURE(*features, VIRTIO_NET_F_CSUM);
}

/*
 * Allocates a virtio device given the base address of memory mapped region.
 */
int alloc_virt_dev(struct pci_device *dev, uint64 bar, uint32 offset) {
  int fd;

  for (int i = 0; i < 6; i++) {
      cprintf("Base: %p, size: %d\n", dev->bar_base[i], dev->bar_size[i]);
  }
  struct virtio_device *vdev = (struct virtio_dev *)kalloc();
  vdev->base = dev->membase;
  vdev->size = dev->bar_size[4];
  vdev->irq = dev->irq_line;
  vdev->conf = (struct virtio_pci_common_cfg *)vdev->base;
  vdev->iobase = dev->iobase;
  cprintf("base: %x iobase: %x\n", dev->iobase, vdev->iobase);

  for (fd = 0; fd < NVIRTIO; vdev++) {
    if (virtdevs[fd] == 0) {
      virtdevs[fd] = vdev;
      break;
      // return fd;
    }
  }

  // for (int i = 0; i < 100; i++) {
  //     if (i % 10 == 0) {
  //         cprintf("\n");
  //     }

  //     cprintf("%x ", inl(vdev->iobase+i));
  // }

  // TODO: move initialization to netcard conf
  return conf_virtio_mem(fd, &virtionet_negotiate);
}

int read_config(struct virtio_device *dev, uint32 offset) {
    uint32 before, after;
    uint32 val = -1;

    do {
        before = *(addr_t*)(dev->base + 25);
        val = *(addr_t*)(dev->base + offset);
        after = *(addr_t*)(dev->base + 25);
    } while(after != before);

    return val;
}

int write_config(struct virtio_device *dev, uint32 offset, void * val) {

}

int conf_virtio_mem(int fd, void (*negotiate)(uint32 *features)) {
    struct virtio_device *dev = virtdevs[fd];
    uint32 before, after;

    // First reset the device
    uint8 flag = VIRTIO_STATUS_RESET;
    *(addr_t*)(dev->base + 20) = flag;

    // Then acknowledge the device
    flag |= VIRTIO_STATUS_ACKNOWLEDGE;
    *(addr_t*)(dev->base + 20) = flag;

    // Let the device know that we can drive it
    flag |= VIRTIO_STATUS_DRIVER;
    *(addr_t*)(dev->base + 20) = flag;

    uint32 features;

    do {
        before = *(addr_t*)(dev->base + 25);
        features = *(addr_t*)(dev->base + 4);
        after = *(addr_t*)(dev->base + 25);
    cprintf("before: %d, after: %d\n", before, after);
    } while(after != before);

    negotiate(&features);

    *(addr_t*)(dev->base + 12) = features;


    flag |= VIRTIO_STATUS_FEATURES_OK;
    *(addr_t*)(dev->base + 20) = flag;

    uint32 val;
    do {
        before = *(addr_t*) (dev->base + 25);
        val = *(addr_t*)dev->base + 20;
        after = *(addr_t*) (dev->base + 25);
    } while(after != before);

    if (val & VIRTIO_STATUS_FEATURES_OK == 0) {
        cprintf("Unable to negotiate features\n");
        return -1;
    }

    *(addr_t*)(dev->base + 26) = 0;

    uint16 queue_size;

    do {
        before = *(addr_t*) (dev->base + 25);
        queue_size = *(addr_t*)(dev->base + 28);
        after = *(addr_t*) (dev->base + 25);
    } while(after != before);

    cprintf("queue 0 size: %d\n", queue_size);

    return 0;
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
  struct virtio_device *dev = virtdevs[fd];

  cprintf("Virtio iobase: %x\n", dev->iobase);
  // First reset the device
  uint8 flag = VIRTIO_STATUS_RESET;
  outb(dev->iobase+VIRTIO_DEV_STATUS_OFF, flag);

  // Then acknowledge the device
  flag |= VIRTIO_STATUS_ACKNOWLEDGE;
  outb(dev->iobase+VIRTIO_DEV_STATUS_OFF, flag);

  // Let the device know that we can drive it
  flag |= VIRTIO_STATUS_DRIVER;
  outb(dev->iobase+VIRTIO_DEV_STATUS_OFF, flag);

  uint32 features = inl(dev->iobase+VIRTIO_HOST_F_OFF);
  cprintf("host features: %x\n", features);
  // device specific feature negotiation
  negotiate(&features);
  cprintf("Wanted features: %x\n", features);
  outl(dev->iobase+VIRTIO_GUEST_F_OFF, features);


  flag |= VIRTIO_STATUS_FEATURES_OK;
  outb(dev->iobase+VIRTIO_DEV_STATUS_OFF, flag);

  uint32 val = inl(dev->iobase+VIRTIO_HOST_F_OFF);
  cprintf("host status: %x\n", val);
  // device specific feature negotiation

  if (val & VIRTIO_STATUS_FEATURES_OK == 0) {
    cprintf("Feature negotiation failed");
    return -1;
  }

  outw(dev->iobase+VIRTIO_QSEL_OFF, 0);
  uint16 num = inw(dev->iobase+VIRTIO_QSIZE_OFF);
  cprintf("Entries in queue %d: %d\n", 1, num);

  flag |= VIRTIO_STATUS_DRIVER_OK;
  outb(dev->iobase+VIRTIO_DEV_STATUS_OFF, flag);

  return 0;
}
