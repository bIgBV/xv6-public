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
  struct virtio_device *vdev = (struct virtio_dev *)kalloc();
  vdev->base = dev->bar_base[1] + offset;
  vdev->size = dev->bar_size[1];
  vdev->irq = dev->irq_line;
  cprintf("base: %p\n", vdev->base);
  vdev->conf = (struct virtio_pci_common_cfg *)vdev->base;
  vdev->iobase = dev->iobase & 0xFFFC;
  cprintf("First: %x, next: %x\n", dev->iobase, vdev->iobase);

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
  for (int i = 0; i < 100; i++) {
      if (i % 10 == 0) {
          cprintf("\n");
      }

      cprintf("%x ", *(addr_t*)(vdev->base+i));
  }

  cprintf("\n");

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
  struct virtio_device *dev = virtdevs[fd];

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

  outw(dev->iobase+VIRTIO_QSEL_OFF, 2);
  uint16 num = inw(dev->iobase+VIRTIO_QSIZE_OFF);
  cprintf("Entries in queue %d: %d\n", 1, num);

  flag |= VIRTIO_STATUS_DRIVER_OK;
  outb(dev->iobase+VIRTIO_DEV_STATUS_OFF, flag);

  return 0;
}
