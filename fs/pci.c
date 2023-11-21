#include "fs/pci_classes.h"
#include "fs/pci.h"
#include <inc/x86.h>
#include <inc/lib.h>

struct ECAMSegment {
    void *vaddr;
    uintptr_t paddr;
    uint16_t segment;
    uint8_t start_bus;
    uint8_t end_bus;
};

#define MAX_SEGMENTS 16

static struct PciDevice *pci_device_list = NULL;

static struct PciDevice pci_device_buffer[PCI_MAX_DEVICES];
static char *pci_class_descriptions[256];
static volatile uint8_t *ecam_base_addr;

static struct PcieIoOps pcie_io;

static struct ECAMSegment segments[16];
static size_t segment_count;

uint64_t tsc_freq = 3400000000;

__attribute__((always_inline)) static inline void
pci_iop_select_address(struct PciDevice *pcid, uint8_t reg) {
    /* Assemble address of requested port. */
    uint32_t address = PCI_PORT_ENABLE_BIT |
                       ((uint32_t)pcid->bus << 16) |
                       ((uint32_t)pcid->device << 11) |
                       ((uint32_t)pcid->function << 8) | (reg & 0xFC);

    /* Select address for the next PCI access. */
    outl(PCI_PORT_ADDRESS, address);
}

static inline uint32_t
pci_iop_read_dword(struct PciDevice *pcid, uint8_t reg) {
    /* Read 32-bit value from the selected PCI address
     * using I/O-port-based access mechanism.
     * TIP: Select address using pci_iop_select_address()
     *      and read from PCI_PORT_DATA. */
    // LAB 10: Your code here
    return 0;
}

static inline void
pci_iop_write_dword(struct PciDevice *pcid, uint8_t reg, uint32_t value) {
    pci_iop_select_address(pcid, reg);
    /* Write 32-bit value to the selected PCI address */
    outl(PCI_PORT_DATA, value);
}

static inline uint16_t
pci_iop_read_word(struct PciDevice *pcid, uint8_t reg) {
    return pci_iop_read_dword(pcid, reg) >> ((reg & 0x2) * 8);
}

static inline uint8_t
pci_iop_read_byte(struct PciDevice *pcid, uint8_t reg) {
    return pci_iop_read_dword(pcid, reg) >> ((reg & 0x3) * 8);
}

static inline void
pci_iop_write_word(struct PciDevice *pcid, uint8_t reg, uint16_t value) {
    uint8_t shift = ((reg & 0x2) * 8);
    uint32_t old_value = pci_iop_read_dword(pcid, reg) & ~(0xFFFF << shift);
    uint32_t new_value = value << shift;
    pci_iop_write_dword(pcid, reg, new_value | old_value);
}

static inline void
pci_iop_write_byte(struct PciDevice *pcid, uint8_t reg, uint8_t value) {
    uint8_t shift = ((reg & 0x3) * 8);
    uint32_t old_value = pci_iop_read_dword(pcid, reg) & ~(0xFF << shift);
    uint32_t new_value = value << shift;
    pci_iop_write_dword(pcid, reg, new_value | old_value);
}

static inline volatile void *
pci_ecam_addr(struct PciDevice *pcid, uint8_t reg) {
    return (volatile void *)(ecam_base_addr +
                             ((pcid->bus & 0xFF) << 20) +
                             ((pcid->device & 0x1F) << 15) +
                             ((pcid->function & 0x7) << 12) +
                             (reg & 0xFFF));
}

static inline uint32_t
pci_ecam_read_dword(struct PciDevice *pcid, uint8_t reg) {
    /* Read 32-bit value from register reg using ECAM.
     * TIP: Use pci_ecam_addr(). Don't forget to align
     * reg to 4 byte granularity */
    // LAB 10: Your code here
    return 0;
}

static inline uint16_t
pci_ecam_read_word(struct PciDevice *pcid, uint8_t reg) {
    return *(volatile uint16_t *)pci_ecam_addr(pcid, reg & 0xffe);
}

static inline uint8_t
pci_ecam_read_byte(struct PciDevice *pcid, uint8_t reg) {
    return *(volatile uint8_t *)pci_ecam_addr(pcid, reg);
}

static inline void
pci_ecam_write_dword(struct PciDevice *pcid, uint8_t reg, uint32_t value) {
    *(volatile uint32_t *)pci_ecam_addr(pcid, reg & 0xffc) = value;
}

static inline void
pci_ecam_write_word(struct PciDevice *pcid, uint8_t reg, uint16_t value) {
    *(volatile uint16_t *)pci_ecam_addr(pcid, reg & 0xffe) = value;
}

static inline void
pci_ecam_write_byte(struct PciDevice *pcid, uint8_t reg, uint8_t value) {
    *(volatile uint8_t *)pci_ecam_addr(pcid, reg) = value;
}

#define ANSII_FG_RED   "\e[91m"
#define ANSII_FG_GREEN "\e[92m"
#define ANSII_FG_BLUE  "\e[94m"
#define ANSII_FG_CYAN  "\e[96m"
#define ANSII_RESET    "\e[m"

static void
pci_print_info(struct PciDevice *pcid) {
    cprintf(ANSII_FG_BLUE "PCI device: %4X:%4X (%4X:%4X) | Class %X Sub %X IF %X | Bus %d Device %d Function %d\n" ANSII_RESET,
            pcid->vendor_id, pcid->device_id, pcid->subvendor_id, pcid->subdevice_id, pcid->class, pcid->subclass, pcid->interface, pcid->bus,
            pcid->device, pcid->function);

    /* Print class info and base addresses */
    cprintf(ANSII_FG_CYAN "  - %s\n", pci_class_descriptions[pcid->class]);

    /* Print base address registers. */
    for (uint8_t i = 0; i < PCI_BAR_COUNT; i++)
        if (pcid->bars[i].base_address || pcid->bars[i].address_is_64bits)
            DEBUG(ANSII_FG_CYAN "  - BAR%u: 0x%X (%s)\n" ANSII_RESET, i,
                  pcid->bars[i].base_address,
                  pcid->bars[i].port_mapped ? "port-mapped" : "memory-mapped");

    /* Interrupt info, if present */
    if (pcid->interrupt_no) {
        cprintf(ANSII_FG_CYAN "  - Interrupt %u (Pin %u Line %u)\n" ANSII_RESET,
                pcid->interrupt_no, pcid->interrupt_pin, pcid->interrupt_line);
    }
}

static void
pci_add_device(struct PciDevice *pcid, struct PciDevice *parent) {
    pcid->parent = parent;
    pcid->next = pci_device_list;
    pci_device_list = pcid;
}

static struct PciDevice *
alloc_pci_device(void) {
    static int pcidev_cnt = 0;
    if (pcidev_cnt >= PCI_MAX_DEVICES)
        return NULL;
    return &pci_device_buffer[pcidev_cnt++];
}

struct PciDevice *
init_pci_device(uint8_t bus, uint8_t device, uint8_t function) {
    /* Declare dummy device to be able to use PCIe config space access functions */
    struct PciDevice tmp_dev = {.bus = bus, .device = device, .function = function};

    /* Get device vendor ID. If vendor is 0xFFFF, the device doesn't exist. */
    uint16_t vendor_id = pcie_io.read16(&tmp_dev, PCI_REG_VENDOR_ID);
    if (vendor_id == PCI_DEVICE_VENDOR_NONE)
        return NULL;

    /* Create PCI device object. */
    struct PciDevice *pcid = alloc_pci_device();
    if (!pcid)
        return NULL;

    memset(pcid, 0, sizeof(*pcid));

    /* Set device location. */
    pcid->bus = bus;
    pcid->device = device;
    pcid->function = function;

    /* Set PCI device identifiers. */
    pcid->vendor_id = pcie_io.read16(pcid, PCI_REG_VENDOR_ID);
    pcid->device_id = pcie_io.read16(pcid, PCI_REG_DEVICE_ID);
    pcid->subvendor_id = pcie_io.read16(pcid, PCI_REG_SUB_VENDOR_ID);
    pcid->subdevice_id = pcie_io.read16(pcid, PCI_REG_SUB_DEVICE_ID);
    pcid->revision_id = pcie_io.read8(pcid, PCI_REG_REVISION_ID);
    pcid->class = pcie_io.read8(pcid, PCI_REG_CLASS);
    pcid->subclass = pcie_io.read8(pcid, PCI_REG_SUBCLASS);
    pcid->interface = pcie_io.read8(pcid, PCI_REG_PROG_IF);
    pcid->header_type = pcie_io.read8(pcid, PCI_REG_HEADER_TYPE);

    /* Set interrupt info. */
    pcid->interrupt_pin = pcie_io.read8(pcid, PCI_REG_INTERRUPT_PIN);
    pcid->interrupt_line = pcie_io.read8(pcid, PCI_REG_INTERRUPT_LINE);
    // FIXME Find device in the ACPI PRT table or use MSI/MSIx
    pcid->interrupt_no = 0;

    /* Set base address registers. */
    for (uint8_t i = 0; i < PCI_BAR_COUNT; i++) {
        /* Read BAR contents */
        uint32_t bar = pcie_io.read32(pcid, PCI_REG_BAR0 + (i * sizeof(uint32_t)));

        pcid->bars[i].port_mapped = (bar & PCI_BAR_TYPE_PORT) != 0;
        if (pcid->bars[i].port_mapped) {
            /* Port I/O BAR */
            pcid->bars[i].base_address = bar & PCI_BAR_PORT_MASK;
        } else {
            /* Memory BAR */
            pcid->bars[i].base_address = bar & PCI_BAR_MEMORY_MASK;
            pcid->bars[i].address_is_64bits = (bar & PCI_BAR_BITS64) != 0;
            pcid->bars[i].prefetchable = (bar & PCI_BAR_PREFETCHABLE) != 0;
            if (pcid->bars[i].address_is_64bits) {
                pcid->bars[i + 1].address_is_64bits = 1;
                pcid->bars[i + 1].base_address = pcie_io.read32(pcid, PCI_REG_BAR0 + ((i + 1) * sizeof(uint32_t)));
                pcid->bars[i + 1].prefetchable = pcid->bars[i].prefetchable;
                i++;
            }
        }
    }

    return pcid;
}

/*
 * Recursively scan a bus for devices, starting
 * with the bus passed a a param.
 */
static void
pci_check_busses(uint8_t bus, struct PciDevice *parent) {
#ifdef PCIE_DEBUG
    cprintf("PCI: Enumerating devices on bus %u...\n", bus);
#endif

    /* Enumerate the bus */
    for (uint8_t device = 0; device < PCI_NUM_DEVICES; device++) {
        /* Try reading device info */
        struct PciDevice *pcid = init_pci_device(bus, device, 0);
        if (!pcid)
            continue;

#ifdef PCIE_DEBUG
        pci_print_info(pcid);
#endif
        pci_add_device(pcid, parent);

        /* If device reports more than one function, scan those too. */
        if (pcid->header_type & PCI_HEADER_TYPE_MULTIFUNC) {
#ifdef PCIE_DEBUG
            cprintf(ANSII_FG_GREEN "  - Scanning other functions on multifunction device!\n" ANSII_RESET);
#endif

            /* Check each function on the device */
            for (uint8_t func = 1; func < PCI_NUM_FUNCTIONS; func++) {
                struct PciDevice *func_dev = init_pci_device(bus, device, func);
                if (!func_dev)
                    continue;

#ifdef PCIE_DEBUG
                pci_print_info(func_dev);
#endif
                pci_add_device(func_dev, parent);
            }
        }

        /* Check if the device is a PCI-to_PCI bridge and
         * recursively scan child bus, if needed. */
        if (pcid->class == PCI_CLASS_BRIDGE && pcid->subclass == PCI_SUBCLASS_BRIDGE_PCI) {
            uint16_t baread2 = pcie_io.read16(pcid, PCI_REG_BAR2);
            uint16_t secondary_bus = (baread2 & ~0x00FF) >> 8;
#ifdef PCIE_DEBUG
            uint16_t primary_bus = (baread2 & ~0xFF00);
            cprintf(ANSII_FG_GREEN "  - PCI bridge, Primary %X Secondary %X, scanning now.\n" ANSII_RESET, primary_bus, secondary_bus);
#endif
            pci_check_busses(secondary_bus, pcid);

            /* If device is a different kind of bridge */
        } else if (pcid->class == PCI_CLASS_BRIDGE) {
#ifdef PCIE_DEBUG
            cprintf(ANSII_FG_RED "  - Ignoring non-PCI bridge\n" ANSII_RESET);
#endif
        }
    }
}

struct PciDevice *
find_pci_dev(int class, int sub) {
    for (int i = 0; i != PCI_MAX_DEVICES; i++) {
        if (pci_device_buffer[i].class == class &&
            pci_device_buffer[i].subclass == sub) {
            DEBUG("Found PCI device: class %X, subclass %X\n", class, sub);
            return &pci_device_buffer[i];
        }
    }
    return 0;
}

uint32_t
get_bar_size(struct PciDevice *pcid, uint32_t barno) {
    if (pcid == NULL || barno >= PCI_BAR_COUNT)
        return 0;

    uint32_t tmp = pcid->bars[barno].base_address;

    /* Overwrite BARx address and read it back to get its size */
    pcie_io.write32(pcid, PCI_REG_BAR0 + 4 * barno, 0xFFFFFFFF);

    uint32_t outv = pcie_io.read32(pcid, PCI_REG_BAR0 + 4 * barno) & PCI_BAR_MEMORY_MASK;
    uint32_t size = ~outv + 1;

    /* Restore previous BARn value */
    pcie_io.write32(pcid, PCI_REG_BAR0 + 4 * barno, tmp);
    DEBUG("BAR%d: umnasked val = %x, size = 0x%x\n", barno, outv, size);

    return size;
}

uintptr_t
get_bar_address(struct PciDevice *pcid, uint32_t barno) {
    if (pcid == NULL || barno >= PCI_BAR_COUNT)
        return 0;

    uintptr_t base_addr = pcid->bars[0].base_address;
    if (pcid->bars[0].address_is_64bits)
        base_addr |= (uint64_t)(pcie_io.read32(pcid, PCI_REG_BAR0 + 4)) << 32;

    return base_addr;
}

static void
pci_set_iomech(enum pcie_iotype io) {
    if (io == PCIE_ECAM) {
        DEBUG("Using ECAM PCIe accesses\n");
        pcie_io.read32 = pci_ecam_read_dword;
        pcie_io.read16 = pci_ecam_read_word;
        pcie_io.read8 = pci_ecam_read_byte;
        pcie_io.write32 = pci_ecam_write_dword;
        pcie_io.write16 = pci_ecam_write_word;
        pcie_io.write8 = pci_ecam_write_byte;
    } else {
        DEBUG("Using I/O-port based PCIe accesses\n");
        pcie_io.read32 = pci_iop_read_dword;
        pcie_io.read16 = pci_iop_read_word;
        pcie_io.read8 = pci_iop_read_byte;
        pcie_io.write32 = pci_iop_write_dword;
        pcie_io.write16 = pci_iop_write_word;
        pcie_io.write8 = pci_iop_write_byte;
    }
}

static void
init_class_descriptions(void) {
    pci_class_descriptions[PCI_CLASS_NONE] = "Unclassified device";
    pci_class_descriptions[PCI_CLASS_MASS_STORAGE] = "Mass storage controller";
    pci_class_descriptions[PCI_CLASS_NETWORK] = "Network controller";
    pci_class_descriptions[PCI_CLASS_DISPLAY] = "Display controller";
    pci_class_descriptions[PCI_CLASS_MULTIMEDIA] = "Multimedia controller";
    pci_class_descriptions[PCI_CLASS_MEMORY] = "Memory controller";
    pci_class_descriptions[PCI_CLASS_BRIDGE] = "Bridge";
    pci_class_descriptions[PCI_CLASS_SIMPLE_COMM] = "Communication controller";
    pci_class_descriptions[PCI_CLASS_BASE_SYSTEM] = "Generic system peripheral";
    pci_class_descriptions[PCI_CLASS_INPUT] = "Input device controller";
    pci_class_descriptions[PCI_CLASS_DOCKING] = "Docking station";
    pci_class_descriptions[PCI_CLASS_PROCESSOR] = "Processor";
    pci_class_descriptions[PCI_CLASS_SERIAL_BUS] = "Serial bus controller";
    pci_class_descriptions[PCI_CLASS_WIRELESS] = "Wireless controller";
    pci_class_descriptions[PCI_CLASS_INTELLIGENT_IO] = "Intelligent controller";
    pci_class_descriptions[PCI_CLASS_SATELLITE_COMM] = "Satellite communications controller";
    pci_class_descriptions[PCI_CLASS_ENCRYPTION] = "Encryption controller";
    pci_class_descriptions[PCI_CLASS_DATA_ACQUISITION] = "Signal processing controller";
    pci_class_descriptions[0x12] = "Processing accelerators";
    pci_class_descriptions[0x13] = "Non-Essential Instrumentation";
    pci_class_descriptions[0x40] = "Coprocessor";
    pci_class_descriptions[PCI_CLASS_UNKNOWN] = "Unassigned class";
}

static inline int
to_hexdigit(unsigned c) {
    if (c - '0' <= 9)
        return c - '0';
    if (c - 'a' <= 'f' - 'a')
        return 10 + c - 'a';
    if (c - 'A' <= 'F' - 'A')
        return 10 + c - 'A';
    return -1;
}

static unsigned long
parse_hex(char **str) {
    unsigned long x = 0;
    for (;; (*str)++) {
        int d = to_hexdigit(**str);
        if (d < 0)
            return x;

        x = (x << 4) + d;
    }
}

static struct ECAMSegment
parse_ecam(char *str) {
    str += strlen("ecam=");

    struct ECAMSegment seg = {0};
    seg.paddr = parse_hex(&str);
    if (*str++ != ':')
        goto invalid;
    seg.segment = parse_hex(&str);
    if (*str++ != ':')
        goto invalid;
    seg.start_bus = parse_hex(&str);
    if (*str++ != ':')
        goto invalid;
    seg.end_bus = parse_hex(&str);

    return seg;

invalid:
    panic("Invalid ecam= format");
}

void
parse_argv(char **argv) {
    ecam_base_addr = (volatile uint8_t *)ECAM_VADDR;
    char *vaddr = (char *)ecam_base_addr;

    /*
     * All ECAM memory regions are parsed by the kernel from the ACPI MCFG
     * table and passed to the fs server via argv as strings in form:
     *   ecam=<base address>:<segment group>:<start bus>:<end bus>
     * with all values being hex numbers. So we need to parse them
     * and map physical memory regions accordingly.
     */

    if (argv)
        for (argv++; *argv; argv++) {
            if (!strncmp(*argv, "ecam=", 5)) {
                segments[segment_count++] = parse_ecam(*argv);
                struct ECAMSegment *ecam = &segments[segment_count - 1];
                ecam->vaddr = vaddr;

                DEBUG("ECAM: paddr=%lx vaddr=%p seg=%x startbus=%x endbus=%x\n",
                      ecam->paddr, ecam->vaddr, ecam->segment, ecam->start_bus, ecam->end_bus);


                uintptr_t size = PCI_ECAM_MEMSIZE * (ecam->end_bus - ecam->start_bus) * PCI_NUM_DEVICES * PCI_NUM_FUNCTIONS;
                int r = sys_map_physical_region(ecam->paddr, 0, ecam->vaddr, size, PROT_RW | PROT_CD);
                if (r < 0)
                    panic("sys_map_physical_region(): %i", r);

                vaddr += size;
            } else if (!strncmp(*argv, "tscfreq=", 8)) {
                char *str = *argv;
                str += strlen("tscfreq=");
                tsc_freq = parse_hex(&str);
                DEBUG("tsc_freq=%ld\n", tsc_freq);
            }
        }

    if (segment_count == 0)
        pci_set_iomech(PCIE_IOPL);
    else
        pci_set_iomech(PCIE_ECAM);
}

/*
 * Initialize PCI bus.
 */
void
pci_init(char **argv) {
    parse_argv(argv);

    init_class_descriptions();

    /* Scan all busses starting at bus 0. */
    pci_check_busses(0, NULL);
}
