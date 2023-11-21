#ifndef PCI_H
#define PCI_H

#include <inc/types.h>
#include <inc/assert.h>
#include <inc/error.h>
#include <inc/mmu.h>
#include <inc/string.h>
#include <inc/uefi.h>
#include <inc/x86.h>

// #define PCIE_DEBUG

#ifdef PCIE_DEBUG
#define ERROR(...)                                                           \
    do {                                                                     \
        cprintf("\e[31mERROR\e[0m[\e[94m%s\e[0m:%d]: ", __func__, __LINE__); \
        cprintf(__VA_ARGS__);                                                \
        cprintf("\n");                                                       \
    } while (0);

#define DEBUG(...)                                                           \
    do {                                                                     \
        cprintf("\e[32mDEBUG\e[0m[\e[94m%s\e[0m:%d]: ", __func__, __LINE__); \
        cprintf(__VA_ARGS__);                                                \
        cprintf("\n");                                                       \
    } while (0);
#else
#define DEBUG(...)
#define ERROR(...)
#endif

#define ECAM_VADDR       0x7000000000
#define NVME_VADDR       0x7001000000
#define NVME_QUEUE_VADDR 0x7002000000

#define PCI_MAX_DEVICES    10
#define PCI_NUM_DEVICES    32
#define PCI_NUM_FUNCTIONS  8
#define PCI_MAX_BUSSES     1
#define PCI_ECAM_BASE_ADDR 0xB0000000ULL
#define PCI_ECAM_MEMSIZE   0x1000

/* PCI configuration space registers. */
#define PCI_REG_VENDOR_ID 0x00 /* word */

#define PCI_REG_DEVICE_ID       0x02 /* word */
#define PCI_REG_COMMAND         0x04 /* word */
#define PCI_REG_STATUS          0x06 /* word */
#define PCI_REG_REVISION_ID     0x08 /* byte */
#define PCI_REG_PROG_IF         0x09 /* byte */
#define PCI_REG_SUBCLASS        0x0A /* byte */
#define PCI_REG_CLASS           0x0B /* byte */
#define PCI_REG_CACHE_LINE_SIZE 0x0C /* byte */
#define PCI_REG_LATENCY_TIMER   0x0D /* byte */
#define PCI_REG_HEADER_TYPE     0x0E /* byte */
#define PCI_REG_BIST            0x0F /* byte */

#define PCI_REG_BAR0 0x10 /* dword */
#define PCI_REG_BAR1 0x14 /* dword */
#define PCI_REG_BAR2 0x18 /* dword */
#define PCI_REG_BAR3 0x1C /* dword */

#define PCI_REG_BAR4          0x20 /* dword */
#define PCI_REG_BAR5          0x24 /* dword */
#define PCI_REG_CARDBUS_CIS   0x28 /* dword */
#define PCI_REG_SUB_VENDOR_ID 0x2C /* word */
#define PCI_REG_SUB_DEVICE_ID 0x2E /* word */

#define PCI_REG_EXPANSION_ROM  0x30 /* dword */
#define PCI_REG_CAPABILITIES   0x34 /* byte */
#define PCI_REG_INTERRUPT_LINE 0x3C /* byte */
#define PCI_REG_INTERRUPT_PIN  0x3D /* byte */
#define PCI_REG_MIN_GNT        0x3E /* byte */
#define PCI_REG_MAX_LAT        0x3F /* byte */

#define PCI_PORT_ADDRESS    0xCF8
#define PCI_PORT_DATA       0xCFC
#define PCI_PORT_ENABLE_BIT 0x80000000

#define PCI_DEVICE_VENDOR_NONE    0xFFFF
#define PCI_HEADER_TYPE_MULTIFUNC 0x80
#define PCI_BAR_COUNT             6

#define PCI_BAR_MEMORY_MASK 0xFFFFFFF0
#define PCI_BAR_PORT_MASK   0xFFFFFFFC

#define PCI_BAR_TYPE_MEMORY 0x0
#define PCI_BAR_TYPE_PORT   0x1

#define PCI_BAR_BITS32 0x0
#define PCI_BAR_BITS64 0x4

#define PCI_BAR_PREFETCHABLE 0x8

#define PCI_CMD_BUSMASTER 0x04

struct PciBaseRegister {
    bool port_mapped : 1;
    bool address_is_64bits : 1;
    bool prefetchable : 1;
    uint32_t base_address;
};

struct PciDevice {
    /* Relations to other devices. */
    struct PciDevice *parent;
    struct PciDevice *next;

    uint8_t bus;
    uint8_t device;
    uint8_t function;

    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t subvendor_id;
    uint16_t subdevice_id;

    uint8_t revision_id;
    uint8_t class;
    uint8_t subclass;
    uint8_t interface;
    uint8_t header_type;

    struct PciBaseRegister bars[PCI_BAR_COUNT];

    /* PCI config space interrupt info. */
    uint8_t interrupt_pin;
    uint8_t interrupt_line;
    /* Actual legacy interrupt number in use by device. */
    uint8_t interrupt_no;
};

enum pcie_iotype {
    PCIE_IOPL = 0,
    PCIE_ECAM
};

uint32_t get_bar_size(struct PciDevice *pcid, uint32_t barno);
uintptr_t get_bar_address(struct PciDevice *pcid, uint32_t barno);

void pci_init(char **argv);
struct PciDevice *find_pci_dev(int class, int sub);

struct PcieIoOps {
    uint32_t (*read32)(struct PciDevice *pcid, uint8_t reg);
    uint16_t (*read16)(struct PciDevice *pcid, uint8_t reg);
    uint8_t (*read8)(struct PciDevice *pcid, uint8_t reg);

    void (*write32)(struct PciDevice *pcid, uint8_t reg, uint32_t val);
    void (*write16)(struct PciDevice *pcid, uint8_t reg, uint16_t val);
    void (*write8)(struct PciDevice *pcid, uint8_t reg, uint8_t val);
};

extern uint64_t tsc_freq;

#endif
