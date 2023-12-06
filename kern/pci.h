#ifndef JOS_KERN_PCI_H
#define JOS_KERN_PCI_H

#include <inc/types.h>

// a structure containing the bus number
// and a reference to the previous bus
struct pci_bus;

// a structure describing a device on a pci bus
struct pci_func {
    struct pci_bus *bus;

    uint32_t dev; // pci slot
    uint32_t func; // pci function

    uint32_t dev_id; // device id
    uint32_t dev_class; // device class

    // addresses, received from BAR
    uint32_t reg_base[6];
    // the size of the memory areas,
    // information about which was received from the BAR
    uint32_t reg_size[6];
    uint8_t irq_line; // interrupt line
};

struct pci_bus {
    struct pci_func *parent_bridge;
    uint32_t busno;
};

// start of initialization of pci devices
int pci_init(void);
// get information from BAR and store it to func
void pci_get_bar_info(struct pci_func *f);

#endif