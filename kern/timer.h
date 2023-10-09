/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TIMER_H
#define JOS_KERN_TIMER_H

#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>

struct Timer {
    const char *timer_name;          /* Timer name */
    void (*timer_init)(void);        /* Timer init */
    uint64_t (*get_cpu_freq)(void);  /* Get CPU frequency */
    void (*enable_interrupts)(void); /* Init timer interrupts */
    void (*handle_interrupts)(void);
};

#define MAX_TIMERS 5

extern struct Timer timertab[MAX_TIMERS];

extern struct Timer timer_pit;
extern struct Timer timer_rtc;
extern struct Timer timer_hpet0;
extern struct Timer timer_hpet1;
extern struct Timer timer_acpipm;
extern struct Timer *timer_for_schedule;

#pragma pack(push, 1)

typedef struct {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} RSDP;

typedef struct {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
} ACPISDTHeader;

typedef struct {
    ACPISDTHeader h;
    uint32_t PointerToOtherSDT[];
} RSDT;

typedef struct {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
} HPETAddress;

typedef struct {
    ACPISDTHeader h;
    uint8_t hardware_rev_id;
    uint8_t comparator_count : 5;
    uint8_t counter_size : 1;
    uint8_t reserved : 1;
    uint8_t legacy_replacement : 1;
    uint16_t pci_vendor_id;
    HPETAddress address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} HPET;

#define HPET_LEG_RT_CAP         (1 << 15)
#define HPET_LEG_RT_CNF         (1 << 1)
#define HPET_ENABLE_CNF         (1 << 0)
#define HPET_TN_TYPE_CNF        (1 << 3)
#define HPET_TN_INT_ENB_CNF     (1 << 2)
#define HPET_TN_VAL_SET_CNF     (1 << 6)
#define HPET_TN_SIZE_CAP        (1 << 5)
#define HPET_TN_PER_INT_CAP     (1 << 4)
#define HPET_TN_TIM_CONF_OFFSET 0
#define HPET_TN_TIM_COMP_OFFSET 8

typedef struct {
    uint64_t GCAP_ID;
    uint64_t rsv1;
    uint64_t GEN_CONF;
    uint64_t rsv2;
    uint64_t GINTR_STA;
    uint64_t rsv3[25];
    uint64_t MAIN_CNT;
    uint64_t rsv4;
    uint64_t TIM0_CONF;
    uint64_t TIM0_COMP;
    uint64_t TIM0_FSB;
    uint64_t rsv5;
    uint64_t TIM1_CONF;
    uint64_t TIM1_COMP;
    uint64_t TIM1_FSB;
    uint64_t rsv6;
    uint64_t TIM2_CONF;
    uint64_t TIM2_COMP;
    uint64_t TIM2_FSB;
    uint64_t rsv7;
    uint64_t rsv8[84];
} HPETRegister;

typedef struct {
    ACPISDTHeader h;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;

    /* field used in ACPI 1.0; no longer in use, for compatibility only */
    uint8_t Reserved;

    uint8_t PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t AcpiEnable;
    uint8_t AcpiDisable;
    uint8_t S4BIOS_REQ;
    uint8_t PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t PM1EventLength;
    uint8_t PM1ControlLength;
    uint8_t PM2ControlLength;
    uint8_t PMTimerLength;
    uint8_t GPE0Length;
    uint8_t GPE1Length;
    uint8_t GPE1Base;
    uint8_t CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t DutyOffset;
    uint8_t DutyWidth;
    uint8_t DayAlarm;
    uint8_t MonthAlarm;
    uint8_t Century;

    /* reserved in ACPI 1.0; used since ACPI 2.0+ */
    uint16_t BootArchitectureFlags;

    uint8_t Reserved2;
    uint32_t Flags;

    /* 12 byte structure; see below for details */
    char ResetReg[12];

    uint8_t ResetValue;
    uint8_t Reserved3[3];
} FADT;

#pragma pack(pop)

void acpi_enable(void);
RSDP *get_rsdp(void);
FADT *get_fadt(void);
HPET *get_hpet(void);

void hpet_print_struct(void);
void hpet_init(void);
void hpet_print_reg(void);
void hpet_enable_interrupts_tim0(void);
void hpet_enable_interrupts_tim1(void);
uint64_t hpet_cpu_frequency(void);
void hpet_handle_interrupts_tim0(void);
void hpet_handle_interrupts_tim1(void);

uint32_t pmtimer_get_timeval(void);
uint64_t pmtimer_cpu_frequency(void);

#define PM_FREQ 3579545

#endif
