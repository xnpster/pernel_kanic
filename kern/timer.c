#include <inc/types.h>
#include <inc/assert.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/stdio.h>
#include <inc/x86.h>
#include <inc/uefi.h>
#include <kern/timer.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define kilo      (1000ULL)
#define Mega      (kilo * kilo)
#define Giga      (kilo * Mega)
#define Tera      (kilo * Giga)
#define Peta      (kilo * Tera)
#define ULONG_MAX ~0UL

#if LAB <= 6
/* Early variant of memory mapping that does 1:1 aligned area mapping
 * in 2MB pages. You will need to reimplement this code with proper
 * virtual memory mapping in the future. */
void *
mmio_map_region(physaddr_t pa, size_t size) {
    void map_addr_early_boot(uintptr_t addr, uintptr_t addr_phys, size_t sz);
    const physaddr_t base_2mb = 0x200000;
    uintptr_t org = pa;
    size += pa & (base_2mb - 1);
    size += (base_2mb - 1);
    pa &= ~(base_2mb - 1);
    size &= ~(base_2mb - 1);
    map_addr_early_boot(pa, pa, size);
    return (void *)org;
}
void *
mmio_remap_last_region(physaddr_t pa, void *addr, size_t oldsz, size_t newsz) {
    return mmio_map_region(pa, newsz);
}
#endif

struct Timer timertab[MAX_TIMERS];
struct Timer *timer_for_schedule;

struct Timer timer_hpet0 = {
        .timer_name = "hpet0",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim0,
        .handle_interrupts = hpet_handle_interrupts_tim0,
};

struct Timer timer_hpet1 = {
        .timer_name = "hpet1",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim1,
        .handle_interrupts = hpet_handle_interrupts_tim1,
};

struct Timer timer_acpipm = {
        .timer_name = "pm",
        .timer_init = acpi_enable,
        .get_cpu_freq = pmtimer_cpu_frequency,
};

void
acpi_enable(void) {
    FADT *fadt = get_fadt();
    outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
    while ((inw(fadt->PM1aControlBlock) & 1) == 0) /* nothing */
        ;
}

static void *
acpi_find_table(const char *sign) {
    /*
     * This function performs lookup of ACPI table by its signature
     * and returns valid pointer to the table mapped somewhere.
     *
     * It is a good idea to checksum tables before using them.
     *
     * HINT: Use mmio_map_region/mmio_remap_last_region
     * before accessing table addresses
     * (Why mmio_remap_last_region is requrired?)
     * HINT: RSDP address is stored in uefi_lp->ACPIRoot
     * HINT: You may want to distunguish RSDT/XSDT
     */
    
    // LAB 5: Your code here
    
    RSDP* rsdp = (RSDP*) uefi_lp->ACPIRoot;
    
    bool use_xsdt = (rsdp->Revision >= 2);
    RSDT* rsdt;
    int TablePointerSize = 8;
    
    if(!use_xsdt || !(rsdt = (RSDT*) (rsdp->XsdtAddress)))
    {
        rsdt = (RSDT*)(uint64_t) (rsdp->RsdtAddress);
        TablePointerSize = 4;
    }
    
    int entr_num = (rsdt->h.Length - sizeof(rsdt->h)) / TablePointerSize;
    uint8_t* entries_base = (uint8_t*) rsdt->PointerToOtherSDT;

    ACPISDTHeader* curr_h;
    for(int i = 0; i < entr_num; i++) {
        if(TablePointerSize == 8)
            curr_h = (ACPISDTHeader*) (*(uint64_t*)(entries_base + i*8));
        else
            curr_h = (ACPISDTHeader*) (uint64_t)(*(uint32_t*)(entries_base + i*4));

        if(strncmp(curr_h->Signature, sign, 4) == 0) {
           return mmio_map_region((physaddr_t) curr_h, curr_h->Length); 
        }
    }
    
    return NULL;
}

/* Obtain and map FADT ACPI table address. */
FADT *
get_fadt(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)
    // HINT: ACPI table signatures are
    //       not always as their names
    
    static FADT *kfadt;
    
    kfadt = (FADT*)acpi_find_table("FACP");

    return kfadt;
}

/* Obtain and map RSDP ACPI table address. */
HPET *
get_hpet(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)

    static HPET *khpet;
    khpet = (HPET*)acpi_find_table("HPET");
    return khpet;
}

/* Getting physical HPET timer address from its table. */
HPETRegister *
hpet_register(void) {
    HPET *hpet_timer = get_hpet();
    if (!hpet_timer->address.address) panic("hpet is unavailable\n");

    uintptr_t paddr = hpet_timer->address.address;
    return mmio_map_region(paddr, sizeof(HPETRegister));
}

/* Debug HPET timer state. */
void
hpet_print_struct(void) {
    //REM
    HPET *hpet = get_hpet();
    assert(hpet != NULL);
    cprintf("signature = %s\n", (hpet->h).Signature);
    cprintf("length = %08x\n", (hpet->h).Length);
    cprintf("revision = %08x\n", (hpet->h).Revision);
    cprintf("checksum = %08x\n", (hpet->h).Checksum);

    cprintf("oem_revision = %08x\n", (hpet->h).OEMRevision);
    cprintf("creator_id = %08x\n", (hpet->h).CreatorID);
    cprintf("creator_revision = %08x\n", (hpet->h).CreatorRevision);

    cprintf("hardware_rev_id = %08x\n", hpet->hardware_rev_id);
    cprintf("comparator_count = %08x\n", hpet->comparator_count);
    cprintf("counter_size = %08x\n", hpet->counter_size);
    cprintf("reserved = %08x\n", hpet->reserved);
    cprintf("legacy_replacement = %08x\n", hpet->legacy_replacement);
    cprintf("pci_vendor_id = %08x\n", hpet->pci_vendor_id);
    cprintf("hpet_number = %08x\n", hpet->hpet_number);
    cprintf("minimum_tick = %08x\n", hpet->minimum_tick);

    cprintf("address_structure:\n");
    cprintf("address_space_id = %08x\n", (hpet->address).address_space_id);
    cprintf("register_bit_width = %08x\n", (hpet->address).register_bit_width);
    cprintf("register_bit_offset = %08x\n", (hpet->address).register_bit_offset);
    cprintf("address = %08lx\n", (unsigned long)(hpet->address).address);
}

static volatile HPETRegister *hpetReg;
/* HPET timer period (in femtoseconds) */
static uint64_t hpetFemto = 0;
/* HPET timer frequency */
static uint64_t hpetFreq = 0;

/* HPET timer initialisation */
void
hpet_init() {
    if (hpetReg == NULL) {
        nmi_disable();
        hpetReg = hpet_register();
        uint64_t cap = hpetReg->GCAP_ID;
        hpetFemto = (uintptr_t)(cap >> 32);
        if (!(cap & HPET_LEG_RT_CAP)) panic("HPET has no LegacyReplacement mode");

        /* cprintf("hpetFemto = %llu\n", hpetFemto); */
        hpetFreq = (1 * Peta) / hpetFemto;
        /* cprintf("HPET: Frequency = %d.%03dMHz\n", (uintptr_t)(hpetFreq / Mega), (uintptr_t)(hpetFreq % Mega)); */
        /* Enable ENABLE_CNF bit to enable timer */
        hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
        nmi_enable();
    }
}

/* HPET register contents debugging. */
void
hpet_print_reg(void) {
    cprintf("GCAP_ID = %016lx\n", (unsigned long)hpetReg->GCAP_ID);
    cprintf("GEN_CONF = %016lx\n", (unsigned long)hpetReg->GEN_CONF);
    cprintf("GINTR_STA = %016lx\n", (unsigned long)hpetReg->GINTR_STA);
    cprintf("MAIN_CNT = %016lx\n", (unsigned long)hpetReg->MAIN_CNT);
    cprintf("TIM0_CONF = %016lx\n", (unsigned long)hpetReg->TIM0_CONF);
    cprintf("TIM0_COMP = %016lx\n", (unsigned long)hpetReg->TIM0_COMP);
    cprintf("TIM0_FSB = %016lx\n", (unsigned long)hpetReg->TIM0_FSB);
    cprintf("TIM1_CONF = %016lx\n", (unsigned long)hpetReg->TIM1_CONF);
    cprintf("TIM1_COMP = %016lx\n", (unsigned long)hpetReg->TIM1_COMP);
    cprintf("TIM1_FSB = %016lx\n", (unsigned long)hpetReg->TIM1_FSB);
    cprintf("TIM2_CONF = %016lx\n", (unsigned long)hpetReg->TIM2_CONF);
    cprintf("TIM2_COMP = %016lx\n", (unsigned long)hpetReg->TIM2_COMP);
    cprintf("TIM2_FSB = %016lx\n", (unsigned long)hpetReg->TIM2_FSB);
}

/* HPET main timer counter value. */
uint64_t
hpet_get_main_cnt(void) {
    return hpetReg->MAIN_CNT;
}

/* - Configure HPET timer 0 to trigger every 0.5 seconds on IRQ_TIMER line
 * - Configure HPET timer 1 to trigger every 1.5 seconds on IRQ_CLOCK line
 *
 * HINT To be able to use HPET as PIT replacement consult
 *      LegacyReplacement functionality in HPET spec.
 * HINT Don't forget to unmask interrupt in PIC */

#define TICKS_PER_SEC 10000000
#define TICKS_PER_500MS (TICKS_PER_SEC / 2)
#define TICKS_PER_0D5SEC TICKS_PER_500MS
#define TICKS_PER_1D5SEC (TICKS_PER_500MS * 20)

void
hpet_enable_interrupts_tim0(void) {
    // LAB 5: Your code here
    if(!(hpetReg->GCAP_ID & HPET_LEG_RT_CAP))
        panic("Legacy mode not supported\n");
    

    uint64_t femptosec_per_tick = (uint32_t) (hpetReg->GCAP_ID >> 32);
    //femptosec = 10^-15 sec
    
    // millis_per_tick = femptosec_per_tick * 10^-12
    // ticks_per_500_ms = 500/millis_per_tick = (500/femptosec_per_tick) * 10^12
    // = (5 * 10^14)/femptosec_per_tick = 1/2 * 1/femptosec_per_tick * 10^15

    uint64_t ticks_per_500_ms = (Peta / 2)/femptosec_per_tick;

    //disable interrupts
    hpetReg->GEN_CONF = hpetReg->GEN_CONF & ~((uint64_t)HPET_ENABLE_CNF);
    //enable lrgacy route
    hpetReg->GEN_CONF = hpetReg->GEN_CONF | HPET_LEG_RT_CNF;
    
    //reset main counter
    hpetReg->MAIN_CNT = (uint64_t) 0;

    if(!(hpetReg->TIM0_CONF & HPET_TN_PER_INT_CAP))
        panic("Periodic interrupts of timer 0 not supported\n");
    
    //set interrupt type to periodic
    hpetReg->TIM0_CONF |= HPET_TN_TYPE_CNF;
    //enable interrupts
    hpetReg->TIM0_CONF |= HPET_TN_INT_ENB_CNF;
    
    uint64_t timer_val_mask = hpetReg->TIM0_CONF & HPET_TN_SIZE_CAP ? 0xFFFFFFFFFFFFFFFF : 0xFFFFFFFF;
   
    //allow to set accumulator
    hpetReg->TIM0_CONF |= HPET_TN_VAL_SET_CNF;
    //set accumulator frequency
    hpetReg->TIM0_COMP = ticks_per_500_ms & timer_val_mask;
    
    //enable interrupts
    hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
    pic_irq_unmask(IRQ_TIMER);
}

void
hpet_enable_interrupts_tim1(void) {
    // LAB 5: Your code here
    if(!(hpetReg->GCAP_ID & HPET_LEG_RT_CAP))
        panic("Legacy mode not supported\n");
    
    uint64_t femptosec_per_tick = (uint32_t) (hpetReg->GCAP_ID >> 32);
    //femptosec = 10^-15 sec
    
    // millis_per_tick = femptosec_per_tick * 10^-12
    // ticks_per_1500_ms = 1500/millis_per_tick = (1500/femptosec_per_tick) * 10^12
    // = (15 * 10^14)/femptosec_per_tick = 3/2 * 1/femptosec_per_tick * 10^15

    uint64_t ticks_per_1500_ms = (3 * Peta / 2)/femptosec_per_tick;

    //disable interrupts
    hpetReg->GEN_CONF = hpetReg->GEN_CONF & ~((uint64_t)HPET_ENABLE_CNF);
    //enable lrgacy route
    hpetReg->GEN_CONF = hpetReg->GEN_CONF | HPET_LEG_RT_CNF;
    
    //reset main counter
    hpetReg->MAIN_CNT = (uint64_t) 0;

    if(!(hpetReg->TIM0_CONF & HPET_TN_PER_INT_CAP))
        panic("Periodic interrupts of timer 1 not supported\n");
    
    //set interrupt type to periodic
    hpetReg->TIM1_CONF |= HPET_TN_TYPE_CNF;
    //enable interrupts
    hpetReg->TIM1_CONF |= HPET_TN_INT_ENB_CNF;
    
    uint64_t timer_val_mask = hpetReg->TIM0_CONF & HPET_TN_SIZE_CAP ? 0xFFFFFFFFFFFFFFFF : 0xFFFFFFFF;
   
    //allow to set accumulator
    hpetReg->TIM1_CONF |= HPET_TN_VAL_SET_CNF;
    //set accumulator frequency
    hpetReg->TIM1_COMP = ticks_per_1500_ms & timer_val_mask;
    
    //enable interrupts
    hpetReg->GEN_CONF |= HPET_ENABLE_CNF;

    pic_irq_unmask(IRQ_CLOCK);
}

void
hpet_handle_interrupts_tim0(void) {
    pic_send_eoi(IRQ_TIMER);
}

void
hpet_handle_interrupts_tim1(void) {
    pic_send_eoi(IRQ_CLOCK);
}

/* Calculate CPU frequency in Hz with the help with HPET timer.
 * HINT Use hpet_get_main_cnt function and do not forget about
 * about pause instruction. */
uint64_t
hpet_cpu_frequency(void) {
    static uint64_t cpu_freq = 0;

    // LAB 5: Your code here
    
    if(cpu_freq == 0) {
        uint64_t old_tsc = read_tsc();
        uint64_t old_cnt = hpet_get_main_cnt();
    
        uint64_t femptosec_per_tick = 0xFFFFFFFF & (hpetReg->GCAP_ID >> 32);
        uint64_t ticks_per_sec = Peta/femptosec_per_tick;
        
        bool is64 = (1 << 13) & hpetReg->GCAP_ID;
    
        int SEC_PART = 100; // count while 1/100 sec
        int64_t diff = 0;
        do {
            asm("pause");
            uint64_t curr = hpet_get_main_cnt();
            
            if(curr < old_cnt) {
                //overflow
                diff = is64 ? 0xFFFFFFFFFFFFFFFF : 0xFFFFFFFF;
                diff = diff - old_cnt + curr;
            } else {
                diff = curr - old_cnt;
            }
        } while (diff < ticks_per_sec / SEC_PART);
    
        cpu_freq = (read_tsc() - old_tsc) * ticks_per_sec / diff;
    }

    return cpu_freq;
}

uint32_t
pmtimer_get_timeval(void) {
    FADT *fadt = get_fadt();
    return inl(fadt->PMTimerBlock);
}

/* Calculate CPU frequency in Hz with the help with ACPI PowerManagement timer.
 * HINT Use pmtimer_get_timeval function and do not forget that ACPI PM timer
 *      can be 24-bit or 32-bit. */
uint64_t
pmtimer_cpu_frequency(void) {
    static uint64_t cpu_freq = 0;

    // LAB 5: Your code here
    
    if(cpu_freq == 0) {
        bool is32 = (1 << 8) & get_fadt()->Flags;
        int SEC_PART = 100; // count while 1/100 sec
        uint64_t old_tsc = read_tsc();
        uint64_t old_cnt = pmtimer_get_timeval();
     
        int64_t diff = 0; 
        do {
            asm("pause");
        
            diff = pmtimer_get_timeval() - old_cnt;
            if(diff < 0) {
                //overflow
                diff += is32 ? 0xFFFFFFFF : 0x00FFFFFF;
            }
        } while (diff < PM_FREQ / SEC_PART);
    
        cpu_freq = (read_tsc() - old_tsc) * PM_FREQ / diff;
    }

    return cpu_freq;
}
