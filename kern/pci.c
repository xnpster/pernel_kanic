#include <inc/x86.h>
#include <inc/assert.h>
#include <inc/string.h>
#include <kern/pci.h>
#include <kern/pcireg.h>
#include <kern/e1000.h>

// for debug purposes
#define pci_show_devs  1
#define pci_show_addrs 0

// PCI "configuration mechanism one"
#define PCI_CONFIGURATION_ADDRESS_PORT (uint32_t)0xCF8
#define PCI_CONFIGURATION_DATA_PORT    (uint32_t)0xCFC

// if we have found a bridge to another bus,
// then we initialize devices on that bus
static int pci_bridge_attach(struct pci_func *pcif);

// PCI driver table
struct pci_driver {
    uint32_t key1, key2;
    int (*attachfn)(struct pci_func *pcif);
};

// pci_attach_class matches the class and subclass of a PCI device
struct pci_driver pci_attach_class[] = {
    {PCI_CLASS_BRIDGE, PCI_SUBCLASS_BRIDGE_PCI, &pci_bridge_attach},
    {0, 0, 0}, // used as a sign of the end of the array
};

// pci_attach_vendor matches the vendor ID and device ID of a PCI device. key1
// and key2 should be the vendor ID and device ID respectively
struct pci_driver pci_attach_vendor[] = {
    {0x8086, 0x100e, e1000_attach},
    {0, 0, 0},
};

static void
pci_conf1_set_addr(
    uint32_t bus,
    uint32_t dev,
    uint32_t func,
    uint32_t offset
) {
    assert(bus < 256);
    assert(dev < 32);
    assert(func < 8);
    assert(offset < 256);
    assert((offset & 0x3) == 0);

    uint32_t v = ((uint32_t)1 << 31) | (bus << 16) | (dev << 11) | (func << 8) | (offset);
    outl(PCI_CONFIGURATION_ADDRESS_PORT, v);
}

// read configuration information from data port
static uint32_t
pci_conf_read(struct pci_func *f, uint32_t off) {
    pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);
    return inl(PCI_CONFIGURATION_DATA_PORT);
}

// write configuration information to data port
static void
pci_conf_write(struct pci_func *f, uint32_t off, uint32_t v) {
    pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);
    outl(PCI_CONFIGURATION_DATA_PORT, v);
}


static int
pci_attach_match(uint32_t key1, uint32_t key2,
                 struct pci_driver *list, struct pci_func *pcif) {
    uint32_t i;

    // we go through the array and compare
    // whether the keys match the keys of
    // the devices we are interested in
    for (i = 0; list[i].attachfn; i++) {
        if (list[i].key1 == key1 && list[i].key2 == key2) {
            // call device attach function
            int r = list[i].attachfn(pcif);
            if (r > 0)
                return r;
            if (r < 0)
                cprintf("pci_attach_match: attaching %x.%x (%p): %i\n", key1, key2, list[i].attachfn, r);
        }
    }
    return 0;
}

// we call the attach function depending
// either on the class and subclass of the
// device or depending on the specific model
static int
pci_attach(struct pci_func *f) {
    return pci_attach_match(PCI_CLASS(f->dev_class),
                            PCI_SUBCLASS(f->dev_class),
                            &pci_attach_class[0], f) ||
           pci_attach_match(PCI_VENDOR(f->dev_id),
                            PCI_PRODUCT(f->dev_id),
                            &pci_attach_vendor[0], f);
}

// array of pci classes
static const char *pci_class[] = {
    [0x0] = "Unknown",
    [0x1] = "Storage controller",
    [0x2] = "Network controller",
    [0x3] = "Display controller",
    [0x4] = "Multimedia device",
    [0x5] = "Memory controller",
    [0x6] = "Bridge device",
};

// for debug purposes
static void
pci_print_func(struct pci_func *f) {
    const char *class = pci_class[0];
    if (PCI_CLASS(f->dev_class) < sizeof(pci_class) / sizeof(pci_class[0]))
        class = pci_class[PCI_CLASS(f->dev_class)];

    cprintf("PCI: %02x:%02x.%d: %04x:%04x: class: %x.%x (%s) irq: %d\n",
            f->bus->busno, f->dev, f->func,
            PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id),
            PCI_CLASS(f->dev_class), PCI_SUBCLASS(f->dev_class), class,
            f->irq_line);
}

/*
This function scans pci for the presence of devices and attaches them.
The scanning strategy is recursive scan.
https://wiki.osdev.org/PCI#Recursive_Scan
*/
static int
pci_scan_bus(struct pci_bus *bus) {
    int totaldev = 0;
    struct pci_func df;
    memset(&df, 0, sizeof(df));
    df.bus = bus;

    for (df.dev = 0; df.dev < 32; df.dev++) {
        uint32_t dev_id = pci_conf_read(&df, PCI_ID_REG);
        if (dev_id == 0xffff) // device does not exist
            continue;

        // bhlc - BIST, Header Type Latency, Timer, Cache Line Size 
        uint32_t bhlc = pci_conf_read(&df, PCI_BHLC_REG);
        if (PCI_HDRTYPE_TYPE(bhlc) > 1) // Unsupported device 
            continue;

        totaldev++;

        struct pci_func f = df;
        for (f.func = 0; f.func < (PCI_HDRTYPE_MULTIFN(bhlc) ? 8 : 1); f.func++) {
            struct pci_func af = f;

            af.dev_id = pci_conf_read(&f, PCI_ID_REG);
            if (PCI_VENDOR(af.dev_id) == 0xffff)
                continue;

            // intr - Max_Lat, Min_Gnt, Interrupt Pin, Interrupt Line 
            uint32_t intr = pci_conf_read(&af, PCI_INTERRUPT_REG);
            af.irq_line = PCI_INTERRUPT_LINE(intr);

            af.dev_class = pci_conf_read(&af, PCI_CLASS_REG);
            if (pci_show_devs)
                pci_print_func(&af);
            pci_attach(&af);
        }
    }

    return totaldev;
}

// determines the number of the bus to which the bridge
// leads and starts its scanning, is part of a recursive strategy
static int
pci_bridge_attach(struct pci_func *pcif) {
    uint32_t ioreg = pci_conf_read(pcif, PCI_BRIDGE_STATIO_REG);
    uint32_t busreg = pci_conf_read(pcif, PCI_BRIDGE_BUS_REG);

    if (PCI_BRIDGE_IO_32BITS(ioreg)) {
        cprintf("PCI: %02x:%02x.%d: 32-bit bridge IO not supported.\n",
                pcif->bus->busno, pcif->dev, pcif->func);
        return 0;
    }

    struct pci_bus nbus;
    memset(&nbus, 0, sizeof(nbus));
    nbus.parent_bridge = pcif;
    nbus.busno = PCI_BRIDGE_BUS_NUM_SECONDARY(busreg);

    if (pci_show_devs)
        cprintf(
            "PCI: %02x:%02x.%d: bridge to PCI bus %d--%d\n",
            pcif->bus->busno, pcif->dev, pcif->func,
            nbus.busno,
            PCI_BRIDGE_BUS_NUM_SUBORDINATE(busreg)
        );

    pci_scan_bus(&nbus);
    return 1;
}

// get information from BAR and store it to func
void
pci_get_bar_info(struct pci_func *f) {
    pci_conf_write(
        f, PCI_COMMAND_STATUS_REG,
        PCI_COMMAND_IO_ENABLE |
        PCI_COMMAND_MEM_ENABLE |
        PCI_COMMAND_MASTER_ENABLE
    );

    uint32_t bar_width;
    uint32_t bar;
    // we look through all the bars,
    // the variable bar is the address of the corresponding register
    for (
        bar = PCI_MAPREG_START; bar < PCI_MAPREG_END;
        bar += bar_width
    ) {
        // save value of BAR
        uint32_t oldv = pci_conf_read(f, bar);

        bar_width = 4;

        // read value of region to map
        pci_conf_write(f, bar, 0xffffffff);
        uint32_t rv = pci_conf_read(f, bar);

        if (rv == 0)
            continue;

        int regnum = PCI_MAPREG_NUM(bar);
        uint32_t base, size;
        // check the type of region and, depending on it,
        // we get information about the size and address
        if (PCI_MAPREG_TYPE(rv) == PCI_MAPREG_TYPE_MEM) {
            if (PCI_MAPREG_MEM_TYPE(rv) == PCI_MAPREG_MEM_TYPE_64BIT)
                bar_width = 8;

            size = PCI_MAPREG_MEM_SIZE(rv);
            base = PCI_MAPREG_MEM_ADDR(oldv);
            if (pci_show_addrs)
                cprintf(
                    "  mem region %d: %d bytes at 0x%x\n",
                    regnum, size, base
                );
        } else {
            size = PCI_MAPREG_IO_SIZE(rv);
            base = PCI_MAPREG_IO_ADDR(oldv);
            if (pci_show_addrs)
                cprintf(
                    "  io region %d: %d bytes at 0x%x\n",
                    regnum, size, base
                );
        }
        // restore the value in BAR
        pci_conf_write(f, bar, oldv);

        // save the obtained values for later use
        f->reg_base[regnum] = base;
        f->reg_size[regnum] = size;

        if (size && !base)
            cprintf(
                "PCI device %02x:%02x.%d (%04x:%04x) "
                "may be misconfigured: "
                "region %d: base 0x%x, size %d\n",
                f->bus->busno, f->dev, f->func,
                PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id),
                regnum, base, size
            );
    }

    cprintf("PCI function %02x:%02x.%d (%04x:%04x) enabled\n",
            f->bus->busno, f->dev, f->func,
            PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id));
}

int
pci_init(void) {
    static struct pci_bus root_bus;
    // start scanning from the zero bus
    memset(&root_bus, 0, sizeof(root_bus));
    return pci_scan_bus(&root_bus);
}
