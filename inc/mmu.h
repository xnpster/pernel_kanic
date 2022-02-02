#ifndef JOS_INC_MMU_H
#define JOS_INC_MMU_H

/* This file contains definitions for the x86 memory management unit (MMU),
 * including paging- and segmentation-related data structures and constants,
 * the %cr0, %cr4, and %rflags registers, and traps.
 */

/*
 * Part 1.  Paging data structures and constants.
 *
 *
 * A linear address 'la' has a three-part structure as follows:
 *
 * +-------9--------+-------9--------+--------9-------+--------9-------+----------12---------+
 * |Page Map Level 4|Page Directory  | Page Directory |   Page Table   | Offset within Page  |
 * |      Index     |  Pointer Index |      Index     |      Index     |                     |
 * +----------------+----------------+----------------+--------------------------------------+
 * \-PML4_INDEX(la)-/\-PDP_INDEX(la)-/\--PD_INDEX(la)-/\--PT_INDEX(la)-/\---PAGE_OFFSET(la)--/
 * \---------------------------PAGE_NUMBER(la)-------------------------/
 * \------------------------------VPT(la)------------------------------/
 * \-------------------VPD(la)------------------------/
 * \-----------VPDP(la)--------------/
 * \---VPML4(la)----/
 *
 * The PML4_INDEX, PDP_INDEX, PD_INDEX, PT_INDEX, VPT, VPD, VPDP, VPML4
 * and PAGE_OFFSET macros decompose linear addresses as shown.
 *
 * To construct a linear address la from PML4_INDEX(la), PDP_INEDX(la), PD_INDEX(la), PT_INDEX(la), and PAGE_OFFSET(la),
 * use MAKE_ADDR(PML4_INDEX(la), PDP_INDEX(la), PD_INDEX(la), PT_INDEX(la), PAGE_OFFSET(la))
 *
 * PML4_INDEX(), PDP_INDEX(), PD_INDEX() and PT_INDEX() are used to index real page tables.
 * VPT(), VPD(), VPDP() and VPML4() are used to index user virtual page tables
 */

/* Page directory and page table constants */

#define PAGE_SHIFT 12
#define PAGE_SIZE  (1LL << PAGE_SHIFT) /* Bytes mapped by a page */

#define PT_ENTRY_COUNT   512 /* Page table entries per page table */
#define PT_ENTRY_SHIFT   9
#define PD_ENTRY_COUNT   512 /* Page directory entries per page directory */
#define PD_ENTRY_SHIFT   9
#define PDP_ENTRY_COUNT  512 /* Page directory pointer entries per page directory pointer table */
#define PDP_ENTRY_SHIFT  9
#define PML4_ENTRY_COUNT 512 /* Page PML4 entries per PML4 */
#define PML4_ENTRY_SHIFT 9

#define PT_SHIFT   PAGE_SHIFT                     /* offset of PT index in a linear address */
#define PD_SHIFT   (PT_SHIFT + PT_ENTRY_SHIFT)    /* offset of PD index in a linear address */
#define PDP_SHIFT  (PD_SHIFT + PD_ENTRY_SHIFT)    /* offset of PDPX index in a linear address */
#define PML4_SHIFT (PDP_SHIFT + PML4_ENTRY_SHIFT) /* offset of PML4 index in a linear address */

#define HUGE_PAGE_SHIFT (PAGE_SHIFT + PT_ENTRY_SHIFT)
#define HUGE_PAGE_SIZE  (1LL << HUGE_PAGE_SHIFT) /* Bytes mapped by a page directory entry/huge page */

/* Linear page number for 4K pages */
#define PAGE_NUMBER(pa) (((uintptr_t)(pa)) >> PAGE_SHIFT)

/* Offset in page */
#define PAGE_OFFSET(la) (((uintptr_t)(la)) & (PAGE_SIZE - 1))
/* Page table index */
#define PT_INDEX(la) ((((uintptr_t)(la)) >> PT_SHIFT) & (PT_ENTRY_COUNT - 1))
/* Page directory index */
#define PD_INDEX(la) ((((uintptr_t)(la)) >> PD_SHIFT) & (PD_ENTRY_COUNT - 1))
/* Page directory pointer index */
#define PDP_INDEX(la) ((((uintptr_t)(la)) >> PDP_SHIFT) & (PDP_ENTRY_COUNT - 1))
/* Page map level 4 index */
#define PML4_INDEX(la) ((((uintptr_t)(la)) >> PML4_SHIFT) & (PML4_ENTRY_COUNT - 1))

/* Construct linear address from indexes and offset */
#define MAKE_ADDR(m, p, d, t, o) ((void*)((uintptr_t)(m) << PML4_SHIFT | (uintptr_t)(p) << PDP_SHIFT | (uintptr_t)(d) << PD_SHIFT | (uintptr_t)(t) << PT_SHIFT | (o)))

#define VPT(la)   PAGE_NUMBER(la)                   /* used to index into vpt[] */
#define VPD(la)   (((uintptr_t)(la)) >> PD_SHIFT)   /* used to index into vpd[] */
#define VPDP(la)  (((uintptr_t)(la)) >> PDP_SHIFT)  /* used to index into vpdp[] */
#define VPML4(la) (((uintptr_t)(la)) >> PML4_SHIFT) /* used to index into vpml4[] */


/* Page table/directory entry flags */
#define PTE_P   0x001        /* Present */
#define PTE_W   0x002        /* Writeable */
#define PTE_U   0x004        /* User */
#define PTE_PWT 0x008        /* Write-Through */
#define PTE_PCD 0x010        /* Cache-Disable */
#define PTE_A   0x020        /* Accessed */
#define PTE_D   0x040        /* Dirty */
#define PTE_PS  0x080        /* Page Size */
#define PTE_G   0x100        /* Global */
#define PTE_NX  (1ULL << 63) /* Not executable */

#define PTE_MBZ   0x180 /* Bits must be zero */
#define PTE_SHARE 0x400

/* The PTE_AVAIL bits aren't used by the kernel or interpreted by the
 * hardware, so user processes are allowed to set them arbitrarily */
#define PTE_AVAIL 0xE00 /* Available for software use */

/* Flags in PTE_SYSCALL may be used in system calls  (Others may not) */
#define PTE_SYSCALL (PTE_AVAIL | PTE_P | PTE_W | PTE_U)

/* Address in page table or page directory entry */
#define PTE_ADDR(pte) ((physaddr_t)(pte) & ~(PTE_NX | (PAGE_SIZE - 1)))


/* Control Register flags */
#define CR0_PE 0x00000001 /* Protection Enable */
#define CR0_MP 0x00000002 /* Monitor coProcessor */
#define CR0_EM 0x00000004 /* Emulation */
#define CR0_TS 0x00000008 /* Task Switched */
#define CR0_ET 0x00000010 /* Extension Type */
#define CR0_NE 0x00000020 /* Numeric Errror */
#define CR0_WP 0x00010000 /* Write Protect */
#define CR0_AM 0x00040000 /* Alignment Mask */
#define CR0_NW 0x20000000 /* Not Writethrough */
#define CR0_CD 0x40000000 /* Cache Disable */
#define CR0_PG 0x80000000 /* Paging */

#define CR4_VME        0x00000001 /* V86 Mode Extensions */
#define CR4_PVI        0x00000002 /* Protected-Mode Virtual Interrupts */
#define CR4_TSD        0x00000004 /* Time Stamp Disable */
#define CR4_DE         0x00000008 /* Debugging Extensions */
#define CR4_PSE        0x00000010 /* Page Size Extensions */
#define CR4_PAE        0x00000020 /* Physical address extension */
#define CR4_MCE        0x00000040 /* Machine Check Enable */
#define CR4_PGE        0x00000080 /* Page Global Enabled */
#define CR4_PCE        0x00000100 /* Performance counter enable */
#define CR4_OSFXSR     0x00000200 /* FXSAVE/FXRSTOR/SSE enable */
#define CR4_OSXMMEXCPT 0x00000400 /* Enable SSE exceptions */
#define CR4_UMIP       0x00000800 /* User-Mode Instruction Prevention */
#define CR4_LA57       0x00001000 /* Level-5 paging */
#define CR4_VMXE       0x00002000 /* Virtual Machine Extensions Enable */
#define CR4_SMXE       0x00004000 /* Safer Mode Extensions Enable */
#define CR4_FSGSBASE   0x00010000 /* Enable FSGSBASE instructions */
#define CR4_PCIDE      0x00020000 /* PCID Enable */
#define CR4_OSXSAVE    0x00040000 /* XSAVE Enable */
#define CR4_SMEP       0x00100000 /* SMEP Enable */
#define CR4_SMAP       0x00200000 /* SMAP Enable */
#define CR4_PKE        0x00400000 /* Protected Key Enable */

/* x86_64 related changes */
#define EFER_MSR 0xC0000080
#define EFER_LME (1ULL << 8)
#define EFER_LMA (1ULL << 10)
#define EFER_NXE (1ULL << 11)

/* RFLAGS register */
#define FL_CF        0x00000001 /* Carry Flag */
#define FL_PF        0x00000004 /* Parity Flag */
#define FL_AF        0x00000010 /* Auxiliary carry Flag */
#define FL_ZF        0x00000040 /* Zero Flag */
#define FL_SF        0x00000080 /* Sign Flag */
#define FL_TF        0x00000100 /* Trap Flag */
#define FL_IF        0x00000200 /* Interrupt Flag */
#define FL_DF        0x00000400 /* Direction Flag */
#define FL_OF        0x00000800 /* Overflow Flag */
#define FL_IOPL_MASK 0x00003000 /* I/O Privilege Level bitmask */
#define FL_IOPL_0    0x00000000 /*   IOPL == 0 */
#define FL_IOPL_1    0x00001000 /*   IOPL == 1 */
#define FL_IOPL_2    0x00002000 /*   IOPL == 2 */
#define FL_IOPL_3    0x00003000 /*   IOPL == 3 */
#define FL_NT        0x00004000 /* Nested Task */
#define FL_RF        0x00010000 /* Resume Flag */
#define FL_VM        0x00020000 /* Virtual 8086 mode */
#define FL_AC        0x00040000 /* Alignment Check */
#define FL_VIF       0x00080000 /* Virtual Interrupt Flag */
#define FL_VIP       0x00100000 /* Virtual Interrupt Pending */
#define FL_ID        0x00200000 /* ID flag */

/* Page fault error codes */
#define FEC_P 0x1  /* Page fault caused by protection violation */
#define FEC_W 0x2  /* Page fault caused by a write */
#define FEC_U 0x4  /* Page fault occured while in user mode */
#define FEC_R 0x8  /* Page fault occured with reserved bits set in page table */
#define FEC_I 0x10 /* Page fault caused by instruction fetch */

/*
 *
 *  Part 2.  Segmentation data structures and constants.
 *
 */

#ifdef __ASSEMBLER__

/* Macros to build GDT entries in assembly. */
#define SEG_NULL \
    .word 0, 0;  \
    .byte 0, 0, 0, 0
#define SEG(type, base, lim)                        \
    .word(((lim) >> 12) & 0xFFFF), ((base)&0xFFFF); \
    .byte(((base) >> 16) & 0xFF), (0x90 | (type)),  \
            (0xC0 | (((lim) >> 28) & 0xF)), (((base) >> 24) & 0xFF)

#define SEG64(type, base, lim)                      \
    .word(((lim) >> 12) & 0xFFFF), ((base)&0xFFFF); \
    .byte(((base) >> 16) & 0xFF), (0x90 | (type)),  \
            (0xA0 | (((lim) >> 28) & 0xF)), (((base) >> 24) & 0xFF)

#define SEG64USER(type, base, lim)                  \
    .word(((lim) >> 12) & 0xFFFF), ((base)&0xFFFF); \
    .byte(((base) >> 16) & 0xFF), (0xf0 | (type)),  \
            (0xA0 | (((lim) >> 28) & 0xF)), (((base) >> 24) & 0xFF)

#else /* not __ASSEMBLER__ */

#include <inc/types.h>

/* Segment Descriptors */
struct Segdesc32 {
    unsigned sd_lim_15_0 : 16;  /* Low bits of segment limit */
    unsigned sd_base_15_0 : 16; /* Low bits of segment base address */
    unsigned sd_base_23_16 : 8; /* Middle bits of segment base address */
    unsigned sd_type : 4;       /* Segment type (see STS_ constants) */
    unsigned sd_s : 1;          /* 0 = system, 1 = application */
    unsigned sd_dpl : 2;        /* Descriptor Privilege Level */
    unsigned sd_p : 1;          /* Present */
    unsigned sd_lim_19_16 : 4;  /* High bits of segment limit */
    unsigned sd_avl : 1;        /* Unused (available for software use) */
    unsigned sd_l : 1;          /* Long mode */
    unsigned sd_db : 1;         /* 0 = 16-bit segment, 1 = 32-bit segment */
                                /* Has to be set to 0 in longmode. */
    unsigned sd_g : 1;          /* Granularity: limit scaled by 4K when set */
    unsigned sd_base_31_24 : 8; /* High bits of segment base address */
} __attribute__((packed));

struct Segdesc64 {
    unsigned sd_lim_15_0 : 16;
    unsigned sd_base_15_0 : 16;
    unsigned sd_base_23_16 : 8;
    unsigned sd_type : 4;
    unsigned sd_s : 1;          /* 0 = system, 1 = application */
    unsigned sd_dpl : 2;        /* Descriptor Privilege Level */
    unsigned sd_p : 1;          /* Present */
    unsigned sd_lim_19_16 : 4;  /* High bits of segment limit */
    unsigned sd_avl : 1;        /* Unused (available for software use) */
    unsigned sd_rsv1 : 2;       /* Reserved */
    unsigned sd_g : 1;          /* Granularity: limit scaled by 4K when set */
    unsigned sd_base_31_24 : 8; /* High bits of segment base address */
    uint32_t sd_base_63_32;
    unsigned sd_res1 : 8;
    unsigned sd_clear : 8;
    unsigned sd_res2 : 16;
} __attribute__((packed));

#define SEG64_TSS(type, base, lim, dpl)                         \
    (struct Segdesc64) {                                        \
        .sd_lim_15_0 = (uint64_t)(lim)&0xFFFF,                  \
        .sd_base_15_0 = (uint64_t)(base)&0xFFFF,                \
        .sd_base_23_16 = ((uint64_t)(base) >> 16) & 0xFF,       \
        .sd_type = type,                                        \
        .sd_s = 0,                                              \
        .sd_dpl = 0,                                            \
        .sd_p = 1,                                              \
        .sd_lim_19_16 = ((uint64_t)(lim) >> 16) & 0xF,          \
        .sd_avl = 0,                                            \
        .sd_rsv1 = 0,                                           \
        .sd_g = 0,                                              \
        .sd_base_31_24 = ((uint64_t)(base) >> 24) & 0xFF,       \
        .sd_base_63_32 = ((uint64_t)(base) >> 32) & 0xFFFFFFFF, \
        .sd_res1 = 0,                                           \
        .sd_clear = 0,                                          \
        .sd_res2 = 0,                                           \
    }

/* Null segment */
#define SEG_NULL                              \
    (struct Segdesc32) {                      \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 \
    }

/* Segment that is loadable but faults when used */
#define SEG_FAULT                             \
    (struct Segdesc32) {                      \
        0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0 \
    }

/* Normal segment */
#define SEG64(type, base, lim, dpl)                                   \
    (struct Segdesc32) {                                              \
        ((lim) >> 12) & 0xFFFF, (base)&0xFFFF, ((base) >> 16) & 0xFF, \
                type, 1, dpl, 1, (unsigned)(lim) >> 28, 0, 1, 0, 1,   \
                (unsigned)(base) >> 24                                \
    }

#define SEG32(type, base, lim, dpl)                                   \
    (struct Segdesc32) {                                              \
        ((lim) >> 12) & 0xFFFF, (base)&0xFFFF, ((base) >> 16) & 0xFF, \
                type, 1, dpl, 1, (unsigned)(lim) >> 28, 0, 0, 1, 1,   \
                (unsigned)(base) >> 24                                \
    }

#define SEG16(type, base, lim, dpl)                                 \
    (struct Segdesc32) {                                            \
        (lim) & 0xFFFF, (base)&0xFFFF, ((base) >> 16) & 0xFF,       \
                type, 1, dpl, 1, (unsigned)(lim) >> 16, 0, 0, 1, 0, \
                (unsigned)(base) >> 24                              \
    }


#endif /* !__ASSEMBLER__ */

/* Application segment type bits */
#define STA_X 0x8 /* Executable segment */
#define STA_E 0x4 /* Expand down (non-executable segments) */
#define STA_C 0x4 /* Conforming code segment (executable only) */
#define STA_W 0x2 /* Writeable (non-executable segments) */
#define STA_R 0x2 /* Readable (executable segments) */
#define STA_A 0x1 /* Accessed */

/* System segment type bits */
#define STS_T16A 0x1 /* Available 16-bit TSS */
#define STS_LDT  0x2 /* Local Descriptor Table */
#define STS_T16B 0x3 /* Busy 16-bit TSS */
#define STS_CG16 0x4 /* 16-bit Call Gate */
#define STS_TG   0x5 /* Task Gate / Coum Transmitions */
#define STS_IG16 0x6 /* 16-bit Interrupt Gate */
#define STS_TG16 0x7 /* 16-bit Trap Gate */
#define STS_T64A 0x9 /* Available 64-bit TSS */
#define STS_T64B 0xB /* Busy 64-bit TSS */
#define STS_CG64 0xC /* 64-bit Call Gate */
#define STS_IG64 0xE /* 64-bit Interrupt Gate */
#define STS_TG64 0xF /* 64-bit Trap Gate */

/*
 *
 *  Part 3.  Traps.
 *
 */

#ifndef __ASSEMBLER__

/* Task state segment format (as described by the Pentium architecture book) */
struct Taskstate {
    uint32_t ts_res1;  /* -- reserved in long mode */
    uintptr_t ts_rsp0; /* Stack pointers and segment selectors */
    uintptr_t ts_rsp1;
    uintptr_t ts_rsp2;
    uint64_t ts_res2;
    uint64_t ts_ist1; /* fields used in for interrupt stack switching */
    uint64_t ts_ist2;
    uint64_t ts_ist3;
    uint64_t ts_ist4;
    uint64_t ts_ist5;
    uint64_t ts_ist6;
    uint64_t ts_ist7;
    uint64_t ts_res3;
    uint16_t ts_res4;
    uint16_t ts_iomb; /* I/O map base address */
} __attribute__((packed));

/* Gate descriptors for interrupts and traps */
struct Gatedesc {
    unsigned gd_off_15_0 : 16;  /* low 16 bits of offset in segment */
    unsigned gd_ss : 16;        /* segment selector */
    unsigned gd_ist : 3;        /* # args, 0 for interrupt/trap gates */
    unsigned gd_rsv1 : 5;       /* reserved(should be zero I guess) */
    unsigned gd_type : 4;       /* type(STS_{TG,IG32,TG32}) */
    unsigned gd_s : 1;          /* must be 0 (system) */
    unsigned gd_dpl : 2;        /* descriptor(meaning new) privilege level */
    unsigned gd_p : 1;          /* Present */
    unsigned gd_off_31_16 : 16; /* high bits of offset in segment */
    uint32_t gd_off_32_63;
    uint32_t gd_rsv2;
} __attribute__((packed));

/* Set up a normal interrupt/trap gate descriptor.
 * - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
 *   see section 9.6.1.3 of the i386 reference: "The difference between
 *   an interrupt gate and a trap gate is in the effect on IF (the
 *   interrupt-enable flag). An interrupt that vectors through an
 *   interrupt gate resets IF, thereby preventing other interrupts from
 *   interfering with the current interrupt handler. A subsequent IRET
 *   instruction restores IF to the value in the EFLAGS image on the
 *   stack. An interrupt through a trap gate does not change IF."
 * - sel: Code segment selector for interrupt/trap handler
 * - off: Offset in code segment for interrupt/trap handler
 * - dpl: Descriptor Privilege Level -
 *    the privilege level required for software to invoke
 *    this interrupt/trap gate explicitly using an int instruction. */
#define GATE(istrap, sel, off, dpl)                           \
    (struct Gatedesc) {                                       \
        .gd_off_15_0 = (uint64_t)(off)&0xFFFF,                \
        .gd_ss = (sel),                                       \
        .gd_ist = 0,                                          \
        .gd_rsv1 = 0,                                         \
        .gd_type = (istrap) ? STS_TG64 : STS_IG64,            \
        .gd_s = 0,                                            \
        .gd_dpl = (dpl),                                      \
        .gd_p = 1,                                            \
        .gd_off_31_16 = ((uint64_t)(off) >> 16) & 0xFFFF,     \
        .gd_off_32_63 = ((uint64_t)(off) >> 32) & 0xFFFFFFFF, \
        .gd_rsv2 = 0,                                         \
    }

/* Set up a call gate descriptor */
#define CALLGATE(ss, off, dpl)                                      \
    (struct Gatedesc) {                                             \
        (gate).gd_off_15_0 = (uint32_t)(off)&0xFFFF,                \
        (gate).gd_ss = (ss),                                        \
        (gate).gd_ist = 0,                                          \
        (gate).gd_rsv1 = 0,                                         \
        (gate).gd_type = STS_CG64,                                  \
        (gate).gd_s = 0,                                            \
        (gate).gd_dpl = (dpl),                                      \
        (gate).gd_p = 1,                                            \
        (gate).gd_off_31_16 = ((uint32_t)(off) >> 16) & 0xFFFF,     \
        (gate).gd_off_32_63 = ((uint64_t)(off) >> 32) & 0xFFFFFFFF, \
        (gate).gd_rsv2 = 0,                                         \
    }

/* Pseudo-descriptors used for LGDT, LLDT and LIDT instructions */
struct Pseudodesc {
    uint16_t pd_lim;  /* Limit */
    uint64_t pd_base; /* Base address */
} __attribute__((packed));

#endif /* !__ASSEMBLER__ */

#endif /* !JOS_INC_MMU_H */
