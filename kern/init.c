/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/uefi.h>
#include <inc/memlayout.h>

#include <kern/monitor.h>
#include <kern/tsc.h>
#include <kern/console.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/timer.h>
#include <kern/trap.h>
#include <kern/sched.h>
#include <kern/picirq.h>
#include <kern/kclock.h>
#include <kern/kdebug.h>
#include <kern/traceopt.h>

void
timers_init(void) {
    timertab[0] = timer_rtc;
    timertab[1] = timer_pit;
    timertab[2] = timer_acpipm;
    timertab[3] = timer_hpet0;
    timertab[4] = timer_hpet1;

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timertab[i].timer_init) {
            timertab[i].timer_init();
            if (trace_init) cprintf("Initialized timer %s\n", timertab[i].timer_name);
        }
    }
}

void
timers_schedule(const char *name) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timertab[i].timer_name && !strcmp(timertab[i].timer_name, name)) {
            if (!timertab[i].enable_interrupts) {
                panic("Timer %s does not support interrupts\n", name);
            }

            timer_for_schedule = &timertab[i];
            timertab[i].enable_interrupts();
            return;
        }
    }

    panic("Timer %s does not exist\n", name);
}

pde_t *
alloc_pd_early_boot(void) {
    /* Assume pde1, pde2 is already used */
    extern uintptr_t pdefreestart, pdefreeend;
    static uintptr_t pdefree = (uintptr_t)&pdefreestart;

    if (pdefree >= (uintptr_t)&pdefreeend) return NULL;

    pde_t *ret = (pde_t *)pdefree;
    pdefree += PAGE_SIZE;
    return ret;
}

void
map_addr_early_boot(uintptr_t va, uintptr_t pa, size_t sz) {
    extern uintptr_t pml4phys;

    pml4e_t *pml4 = &pml4phys;
    pdpe_t *pdp;
    pde_t *pd;

    uintptr_t vstart = ROUNDDOWN(va, HUGE_PAGE_SIZE);
    uintptr_t vend = ROUNDUP(va + sz, HUGE_PAGE_SIZE);
    uintptr_t pstart = ROUNDDOWN(pa, HUGE_PAGE_SIZE);

    pdp = (pdpe_t *)PTE_ADDR(pml4[PML4_INDEX(vstart)]);
    for (; vstart < vend; vstart += HUGE_PAGE_SIZE, pstart += HUGE_PAGE_SIZE) {
        pd = (pde_t *)PTE_ADDR(pdp[PDP_INDEX(vstart)]);
        if (!pd) {
            pd = alloc_pd_early_boot();
            pdp[PDP_INDEX(vstart)] = (uintptr_t)pd | PTE_P | PTE_W;
        }
        pd[PD_INDEX(vstart)] = pstart | PTE_P | PTE_W | PTE_PS;
    }
}

#if defined(SANITIZE_SHADOW_BASE) && LAB >= 6
void
map_shadow_early_boot(uintptr_t va, uintptr_t sz, void *page) {
    for (size_t p = ROUNDDOWN(va, HUGE_PAGE_SIZE);
         p < ROUNDUP(va + sz, HUGE_PAGE_SIZE); p += HUGE_PAGE_SIZE) {
        map_addr_early_boot(p, PADDR(page), HUGE_PAGE_SIZE);
    }
}
#endif

extern char end[];

/* Additionally maps pml4 memory so that we dont get memory errors on accessing
 * uefi_lp, MemMap, KASAN functions. */
void
early_boot_pml4_init(void) {
    map_addr_early_boot((uintptr_t)uefi_lp, (uintptr_t)uefi_lp, sizeof(LOADER_PARAMS));
    map_addr_early_boot((uintptr_t)uefi_lp->MemoryMap, (uintptr_t)uefi_lp->MemoryMap, uefi_lp->MemoryMapSize);

#if defined(SANITIZE_SHADOW_BASE) && LAB >= 6
    /* Setup early shadow memory:
     * all mapped addresses are considered valid
     * and mapped onto the same page (that is filled with 0x00s) */
    map_shadow_early_boot(SANITIZE_SHADOW_BASE - 1 * GB, SANITIZE_SHADOW_SIZE, zero_page_raw);
    map_shadow_early_boot(SHADOW_ADDR(uefi_lp), sizeof *uefi_lp, zero_page_raw);
    map_shadow_early_boot(SHADOW_ADDR(uefi_lp->MemoryMap), uefi_lp->MemoryMapSize, zero_page_raw);
    /* Kernel stack should have separate shadow memory since it
     * gets poisoned by instrumented code automatically and overlapping
     * it with other shadow memory would cause weird memory errors */
    map_shadow_early_boot(SHADOW_ADDR(KERN_STACK_TOP - KERN_STACK_SIZE), HUGE_PAGE_SIZE, one_page_raw);
#endif

#if LAB <= 6
    map_addr_early_boot(FRAMEBUFFER, uefi_lp->FrameBufferBase, uefi_lp->FrameBufferSize);
#endif
}

void
i386_init(void) {

    early_boot_pml4_init();

    /* Initialize the console.
     * Can't call cprintf until after we do this! */
    cons_init();

    tsc_calibrate();

    if (trace_init) {
        cprintf("6828 decimal is %o octal!\n", 6828);
        cprintf("END: %p\n", end);
    }

    /* Lab 6 memory management initialization functions */
    init_memory();

    pic_init();
    timers_init();

    /* Framebuffer init should be done after memory init */
    fb_init();
    if (trace_init) cprintf("Framebuffer initialised\n");

    /* User environment initialization functions */
    env_init();

    /* Choose the timer used for scheduling: hpet or pit */
    timers_schedule("hpet0");

#ifdef CONFIG_KSPACE
    /* Touch all you want */
    ENV_CREATE_KERNEL_TYPE(prog_test1);
    ENV_CREATE_KERNEL_TYPE(prog_test2);
    ENV_CREATE_KERNEL_TYPE(prog_test3);
    ENV_CREATE_KERNEL_TYPE(prog_test4);
    ENV_CREATE_KERNEL_TYPE(prog_test5);
    ENV_CREATE_KERNEL_TYPE(prog_test6);
#else

#if LAB >= 10
    ENV_CREATE(fs_fs, ENV_TYPE_FS);
#endif

#if defined(TEST)
    /* Don't touch -- used by grading script! */
    ENV_CREATE(TEST, ENV_TYPE_USER);
#else
    /* Touch all you want. */
    ENV_CREATE(user_forktree, ENV_TYPE_USER);
#endif /* TEST* */
#endif

    /* Schedule and run the first user environment! */
    sched_yield();
}

/* Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic. */
const char *panicstr = NULL;

/* Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor. */
_Noreturn void
_panic(const char *file, int line, const char *fmt, ...) {
    va_list ap;

    if (panicstr) goto dead;
    panicstr = fmt;

    /* Be extra sure that the machine is in as reasonable state */
    asm volatile("cli; cld");

    va_start(ap, fmt);
    cprintf("kernel panic at %s:%d: ", file, line);
    vcprintf(fmt, ap);
    cprintf("\n");
    va_end(ap);

dead:
    /* Break into the kernel monitor */
    for (;;) monitor(NULL);
}

/* Like panic, but don't */
void
_warn(const char *file, int line, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    cprintf("kernel warning at %s:%d: ", file, line);
    vcprintf(fmt, ap);
    cprintf("\n");
    va_end(ap);
}
