#ifndef JOS_INC_MEMLAYOUT_H
#define JOS_INC_MEMLAYOUT_H

#ifndef __ASSEMBLER__
#include <inc/types.h>
#include <inc/vsyscall.h>
#include <inc/mmu.h>
#endif /* not __ASSEMBLER__ */

/*
 * This file contains definitions for memory management in our OS,
 * which are relevant to both the kernel and user-mode software.
 */

/* Global descriptor numbers */
#define GD_KT   0x08 /* kernel text */
#define GD_KD   0x10 /* kernel data */
#define GD_KT32 0x18 /* kernel text 32bit */
#define GD_KD32 0x20 /* kernel data 32bit */
#define GD_UT   0x28 /* user text */
#define GD_UD   0x30 /* user data */
#define GD_TSS0 0x38 /* Task segment selector for CPU 0 */

/*
 * Virtual memory map:                                Permissions
 *                                                    kernel/user
 *
 *     1 TB -------->  +------------------------------+
 *                     |                              | RW/--
 *                     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                     :              .               : 0x1
 *                     :              .               :
 *                     :              .               :
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~| RW/--
 *                     |                              | RW/--
 *                     |   Remapped Physical Memory   | RW/--
 *                     |                              | RW/--
 * KERN_BASE_ADDR,-->  +------------------------------+ 0x8040000000          <--+
 * KERN_STACK_TOP      |     CPU0's Kernel Stack      | RW/--  KERN_STACK_SIZE   |
 *                     | - - - - - - - - - - - - - - -|                          |
 *                     |      Invalid Memory (*)      | --/--  KERN_STACK_GAP    |
 *                     +------------------------------+                          |
 * KERN_PF_STACK_TOP-> |       CPU0's #PF Stack       | RW/--  KERN_PF_STACK_SIZE|
 *                     | - - - - - - - - - - - - - - -|                          |
 *                     |      Invalid Memory (*)      | --/--  KERN_STACK_GAP    |
 *                     +------------------------------+                          |
 *                     |     CPU1's Kernel Stack      | RW/--  KERN_STACK_SIZE   |
 *                     | - - - - - - - - - - - - - - -|                   HUGE_PAGE_SIZE
 *                     |      Invalid Memory (*)      | --/--  KERN_STACK_GAP    |
 *                     +------------------------------+                          |
 *                     :              .               :                          |
 *                     :              .               :                          |
 *  KERN_HEAP_END -->  +------------------------------+ 0x803fe00000          <--+
 *                     |       Memory-mapped I/O      | RW/--  HUGE_PAGE_SIZE
 * MAX_USER_READABLE, KERN_HEAP_START -->  +------------------------------+ 0x803fc00000
 *                     |          RO PAGES            | R-/R-
 *                     .                              .
 *                     .                              .        400 * HUGE_PAGE_SIZE
 *                     .                              .
 *                     |                              |
 * UENVS ----------->  +------------------------------+ 0x8000da0000
 *                     |                              |
 *                     |                              |
 *                     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                     .                              .
 *                     .                              .
 * MAX_USER_ADDRESS,               .                              .
 *USER_EXCEPTION_STACK_TOP +--------------------------+ 0x8000000000
 *                     |     User Exception Stack     | RW/RW  PAGE_SIZE
 *                     +------------------------------+ 0x7ffffff000
 *                     |       Empty Memory (*)       | --/--  PAGE_SIZE
 * USER_STACK_TOP -->  +------------------------------+ 0x7fffffe000
 *                     |      Normal User Stack       | RW/RW  PAGE_SIZE
 *                     +------------------------------+ 0x7fffffd000
 *                     |                              |
 *                     |                              |
 *                     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                     .                              .
 *                     .                              .
 *                     .                              .
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 *                     |     Program Data & Heap      |
 *    UTEXT -------->  +------------------------------+ 0x00800000
 *                     |       Empty Memory (*)       |        2* HUGE_PAGE_SIZE
 *                     |                              |
 *    UTEMP -------->  +------------------------------+ 0x00400000      --+
 *                     |       Empty Memory (*)       |                   |
 *                     | - - - - - - - - - - - - - - -|                   |
 *                     |  User STAB Data (optional)   |                2* HUGE_PAGE_SIZE
 *    USTABDATA ---->  +------------------------------+ 0x00200000        |
 *                     |       Empty Memory (*)       |                   |
 *    0 ------------>  +------------------------------+                 --+
 *
 * (*) Note: The kernel ensures that "Invalid Memory" (MAX_USER_READABLE) is *never*
 *     mapped.  "Empty Memory" is normally unmapped, but user programs may
 *     map pages there if desired.  JOS user programs map pages temporarily
 *     at UTEMP.
 */

/* All physical memory mapped at this address
 * (this *should* be defined as a literal number) */
#define KERN_BASE_ADDR 0x8040000000
/* Kernel main core base address in physical memory
 * (this *should* be defined as a literal number) */
#define KERN_START_OFFSET 0x02100000

/* At IOPHYSMEM (640K) there is a 384K hole for I/O.  From the kernel,
 * IOPHYSMEM can be addressed at KERN_BASE_ADDR + IOPHYSMEM.  The hole ends
 * at physical address EXTPHYSMEM. */
#define IOPHYSMEM  0x0A0000
#define EXTPHYSMEM 0x100000

/* Amount of memory mapped by entrypgdir */
#define BOOT_MEM_SIZE (1024 * 1024 * 1024ULL)

/* Kernel stack */
#define KERN_STACK_TOP     KERN_BASE_ADDR
#define PROG_STACK_SIZE    (2 * PAGE_SIZE)                                     /* size of a process stack */
#define KERN_STACK_SIZE    (16 * PAGE_SIZE)                                    /* size of a kernel stack */
#define KERN_PF_STACK_SIZE (16 * PAGE_SIZE)                                    /* size of a kernel stack */
#define KERN_STACK_GAP     (8 * PAGE_SIZE)                                     /* size of a kernel stack guard */
#define KERN_PF_STACK_TOP  (KERN_STACK_TOP - KERN_STACK_SIZE - KERN_STACK_GAP) /* size of page fault handler stack size */

/* Memory-mapped IO */
#define KERN_HEAP_END   (KERN_STACK_TOP - HUGE_PAGE_SIZE)
#define KERN_HEAP_START (KERN_HEAP_END - HUGE_PAGE_SIZE * 256) /* Max size of kernel heap is 512MB */

/* Memory-mapped FrameBuffer */
#define FRAMEBUFFER KERN_HEAP_START

#define MAX_USER_READABLE FRAMEBUFFER

/*
 * User read-only mappings! Anything below here til MAX_USER_ADDRESS are readonly to user.
 * They are global pages mapped in at env allocation time.
 */

/* User read-only virtual page table (see 'uvpt' below) */
#define UVPT_INDEX 255ULL
#define UVPT       (UVPT_INDEX << PML4_SHIFT)
#define UVPD       (UVPT + (UVPT_INDEX << PDP_SHIFT))
#define UVPDP      (UVPD + (UVPT_INDEX << PD_SHIFT))
#define UVPML4     (UVPDP + (UVPT_INDEX << PT_SHIFT))

/* Read-only copies of the global env structures */
#define UENVS_SIZE HUGE_PAGE_SIZE
#define UENVS      (MAX_USER_READABLE - UENVS_SIZE)

/* Virtual syscall page */
#define UVSYS_SIZE PAGE_SIZE
#define UVSYS      (UENVS - UVSYS_SIZE)

/*
 * Top of user VM. User can manipulate VA from MAX_USER_ADDRESS-1 and down!
 */

/* Top of user-accessible VM */
#define MAX_USER_ADDRESS 0x8000000000
/* Top of one-page user exception stack */
#define USER_EXCEPTION_STACK_TOP MAX_USER_ADDRESS
/* Size of exception stack (must be one page for now) */
#define USER_EXCEPTION_STACK_SIZE (8 * PAGE_SIZE)
/* Top of normal user stack */
/* Next page left invalid to guard against exception stack overflow; then: */
#define USER_STACK_TOP (USER_EXCEPTION_STACK_TOP - USER_EXCEPTION_STACK_SIZE - PAGE_SIZE)
/* Stack size (variable) */
#define USER_STACK_SIZE (16 * PAGE_SIZE)
/* Max number of open files in the file system at once */
#define MAXOPEN   512
#define FILE_BASE 0x200000000

#ifdef SAN_ENABLE_KASAN
/* (this *should* be defined as a literal number) */
#define SANITIZE_SHADOW_BASE 0xA000000000
/* SANITIZE_SHADOW_SIZE of 2 GB allows 16GB of addressible memory (due to byte granularity). */
#define SANITIZE_SHADOW_SIZE 0x80000000
#define SANITIZE_SHADOW_OFF  (SANITIZE_SHADOW_BASE - (KERN_BASE_ADDR >> 3))
#endif

#ifdef SAN_ENABLE_UASAN
/* (this *should* be defined as a literal number)
 * Cannot use lower addresses since ASAN instrumented code
 * use or insn instead of add to add base addreses and this prevents
 * us from mapping UVPT shadow memory correctly */
#define SANITIZE_USER_SHADOW_BASE 0x4000000000
/* Minimal amount of shadow memory to cover whole
 * user addressible memory (513GB, up to MAX_USER_READABLE) */
#define SANITIZE_USER_SHADOW_SIZE 0x1008000000
#define SANITIZE_USER_SHADOW_OFF  SANITIZE_USER_SHADOW_BASE
#endif

/* Where user programs generally begin */
#define UTEXT (4 * HUGE_PAGE_SIZE)

/* Used for temporary page mappings.  Typed 'void*' for convenience */
#define UTEMP ((void *)(2 * HUGE_PAGE_SIZE))

#define MAX_LOW_ADDR_KERN_SIZE 0x3200000

#ifndef __ASSEMBLER__

typedef uint64_t pml4e_t;
typedef uint64_t pdpe_t;
typedef uint64_t pde_t;
typedef uint64_t pte_t;

#endif /* !__ASSEMBLER__ */
#endif /* !JOS_INC_MEMLAYOUT_H */
