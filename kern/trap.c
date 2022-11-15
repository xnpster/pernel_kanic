#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>
#include <inc/string.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/timer.h>
#include <kern/traceopt.h>

static struct Taskstate ts;

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case */
static struct Trapframe *last_tf;

/* Interrupt descriptor table  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records) */
struct Gatedesc idt[256] = {{0}};
struct Pseudodesc idt_pd = {sizeof(idt) - 1, (uint64_t)idt};

/* Global descriptor table.
 *
 * Set up global descriptor table (GDT) with separate segments for
 * kernel mode and user mode.  Segments serve many purposes on the x86.
 * We don't use any of their memory-mapping capabilities, but we need
 * them to switch privilege levels.
 *
 * The kernel and user segments are identical except for the DPL.
 * To load the SS register, the CPL must equal the DPL.  Thus,
 * we must duplicate the segments for the user and the kernel.
 *
 * In particular, the last argument to the SEG macro used in the
 * definition of gdt specifies the Descriptor Privilege Level (DPL)
 * of that descriptor: 0 for kernel and 3 for user. */
struct Segdesc32 gdt[2 * NCPU + 7] = {
        /* 0x0 - unused (always faults -- for trapping NULL far pointers) */
        SEG_NULL,
        /* 0x8 - kernel code segment */
        [GD_KT >> 3] = SEG64(STA_X | STA_R, 0x0, 0xFFFFFFFF, 0),
        /* 0x10 - kernel data segment */
        [GD_KD >> 3] = SEG64(STA_W, 0x0, 0xFFFFFFFF, 0),
        /* 0x18 - kernel code segment 32bit */
        [GD_KT32 >> 3] = SEG32(STA_X | STA_R, 0x0, 0xFFFFFFFF, 0),
        /* 0x20 - kernel data segment 32bit */
        [GD_KD32 >> 3] = SEG32(STA_W, 0x0, 0xFFFFFFFF, 0),
        /* 0x28 - user code segment */
        [GD_UT >> 3] = SEG64(STA_X | STA_R, 0x0, 0xFFFFFFFF, 3),
        /* 0x30 - user data segment */
        [GD_UD >> 3] = SEG64(STA_W, 0x0, 0xFFFFFFFF, 3),
        /* Per-CPU TSS descriptors (starting from GD_TSS0) are initialized
     * in trap_init_percpu() */
        [GD_TSS0 >> 3] = SEG_NULL,
        [(GD_TSS0 >> 3) + 1] = SEG_NULL, /* last 8 bytes of the tss since tss is 16 bytes long */
};

struct Pseudodesc gdt_pd = {sizeof(gdt) - 1, (unsigned long)gdt};

static const char *
trapname(int trapno) {
    static const char *const excnames[] = {
            "Divide error",
            "Debug",
            "Non-Maskable Interrupt",
            "Breakpoint",
            "Overflow",
            "BOUND Range Exceeded",
            "Invalid Opcode",
            "Device Not Available",
            "Double Fault",
            "Coprocessor Segment Overrun",
            "Invalid TSS",
            "Segment Not Present",
            "Stack Fault",
            "General Protection",
            "Page Fault",
            "(unknown trap)",
            "x87 FPU Floating-Point Error",
            "Alignment Check",
            "Machine-Check",
            "SIMD Floating-Point Exception"};

    if (trapno < sizeof(excnames) / sizeof(excnames[0])) return excnames[trapno];
    if (trapno == T_SYSCALL) return "System call";
    if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16) return "Hardware Interrupt";

    return "(unknown trap)";
}

void
trap_init(void) {
    // LAB 4: Your code here
    // LAB 5: Your code here


    /* Insert trap handlers into IDT */
    // LAB 8: Your code here

    /* Setup #PF handler dedicated stack
     * It should be switched on #PF because
     * #PF is the only kind of exception that
     * can legally happen during normal kernel
     * code execution */
    idt[T_PGFLT].gd_ist = 1;

    /* Per-CPU setup */
    trap_init_percpu();
}

/* Initialize and load the per-CPU TSS and IDT */
void
trap_init_percpu(void) {
    /* Load GDT and segment descriptors. */

    lgdt(&gdt_pd);

    /* The kernel never uses GS or FS,
     * so we leave those set to the user data segment
     *
     * For good measure, clear the local descriptor table (LDT),
     * since we don't use it */
    asm volatile(
            "movw %%dx,%%gs\n\t"
            "movw %%dx,%%fs\n\t"
            "movw %%ax,%%es\n\t"
            "movw %%ax,%%ds\n\t"
            "movw %%ax,%%ss\n\t"
            "xorl %%eax,%%eax\n\t"
            "lldt %%ax\n\t"
            "pushq %%rcx\n\t"
            "movabs $1f,%%rax\n\t"
            "pushq %%rax\n\t"
            "lretq\n"
            "1:\n" ::"a"(GD_KD),
            "d"(GD_UD | 3), "c"(GD_KT)
            : "cc", "memory");

    /* Setup a TSS so that we get the right stack
     * when we trap to the kernel. */
    ts.ts_rsp0 = KERN_STACK_TOP;
    ts.ts_ist1 = KERN_PF_STACK_TOP;

    /* Initialize the TSS slot of the gdt. */
    *(volatile struct Segdesc64 *)(&gdt[(GD_TSS0 >> 3)]) = SEG64_TSS(STS_T64A, ((uint64_t)&ts), sizeof(struct Taskstate), 0);

    /* Load the TSS selector (like other segment selectors, the
     * bottom three bits are special; we leave them 0) */
    ltr(GD_TSS0);

    /* Load the IDT */
    lidt(&idt_pd);
}

void
print_trapframe(struct Trapframe *tf) {
    cprintf("TRAP frame at %p\n", tf);
    print_regs(&tf->tf_regs);
    cprintf("  es   0x----%04x\n", tf->tf_es);
    cprintf("  ds   0x----%04x\n", tf->tf_ds);
    cprintf("  trap 0x%08lx %s\n", (unsigned long)tf->tf_trapno, trapname(tf->tf_trapno));

    /* If this trap was a page fault that just happened
     * (so %cr2 is meaningful), print the faulting linear address */
    if (tf == last_tf && tf->tf_trapno == T_PGFLT)
        cprintf("  cr2  0x%08lx\n", (unsigned long)rcr2());

    cprintf("  err  0x%08lx", (unsigned long)tf->tf_err);

    /* For page faults, print decoded fault error code:
     *     U/K=fault occurred in user/kernel mode
     *     W/R=a write/read caused the fault
     *     PR=a protection violation caused the fault (NP=page not present) */
    if (tf->tf_trapno == T_PGFLT) {
        cprintf(" [%s, %s, %s]\n",
                tf->tf_err & FEC_U ? "user" : "kernel",
                tf->tf_err & FEC_W ? "write" : tf->tf_err & FEC_I ? "execute" :
                                                                    "read",
                tf->tf_err & FEC_P ? "protection" : "not-present");
    } else
        cprintf("\n");

    cprintf("  rip  0x%08lx\n", (unsigned long)tf->tf_rip);
    cprintf("  cs   0x----%04x\n", tf->tf_cs);
    cprintf("  flag 0x%08lx\n", (unsigned long)tf->tf_rflags);
    cprintf("  rsp  0x%08lx\n", (unsigned long)tf->tf_rsp);
    cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

void
print_regs(struct PushRegs *regs) {
    cprintf("  r15  0x%08lx\n", (unsigned long)regs->reg_r15);
    cprintf("  r14  0x%08lx\n", (unsigned long)regs->reg_r14);
    cprintf("  r13  0x%08lx\n", (unsigned long)regs->reg_r13);
    cprintf("  r12  0x%08lx\n", (unsigned long)regs->reg_r12);
    cprintf("  r11  0x%08lx\n", (unsigned long)regs->reg_r11);
    cprintf("  r10  0x%08lx\n", (unsigned long)regs->reg_r10);
    cprintf("  r9   0x%08lx\n", (unsigned long)regs->reg_r9);
    cprintf("  r8   0x%08lx\n", (unsigned long)regs->reg_r8);
    cprintf("  rdi  0x%08lx\n", (unsigned long)regs->reg_rdi);
    cprintf("  rsi  0x%08lx\n", (unsigned long)regs->reg_rsi);
    cprintf("  rbp  0x%08lx\n", (unsigned long)regs->reg_rbp);
    cprintf("  rbx  0x%08lx\n", (unsigned long)regs->reg_rbx);
    cprintf("  rdx  0x%08lx\n", (unsigned long)regs->reg_rdx);
    cprintf("  rcx  0x%08lx\n", (unsigned long)regs->reg_rcx);
    cprintf("  rax  0x%08lx\n", (unsigned long)regs->reg_rax);
}

static void
trap_dispatch(struct Trapframe *tf) {
    switch (tf->tf_trapno) {
    case T_SYSCALL:
        tf->tf_regs.reg_rax = syscall(
                tf->tf_regs.reg_rax,
                tf->tf_regs.reg_rdx,
                tf->tf_regs.reg_rcx,
                tf->tf_regs.reg_rbx,
                tf->tf_regs.reg_rdi,
                tf->tf_regs.reg_rsi,
                tf->tf_regs.reg_r8);
        return;
    case T_BRKPT:
        // LAB 8: Your code here
        return;
    case IRQ_OFFSET + IRQ_SPURIOUS:
        /* Handle spurious interrupts
         * The hardware sometimes raises these because of noise on the
         * IRQ line or other reasons, we don't care */
        if (trace_traps) {
            cprintf("Spurious interrupt on irq 7\n");
            print_trapframe(tf);
        }
        return;
    case IRQ_OFFSET + IRQ_TIMER:
    case IRQ_OFFSET + IRQ_CLOCK:
        // LAB 5: Your code here
        // LAB 4: Your code here
        return;
    default:
        print_trapframe(tf);
        if (!(tf->tf_cs & 3))
            panic("Unhandled trap in kernel");
        env_destroy(curenv);
    }
}

/* We do not support recursive page faults in-kernel */
bool in_page_fault;

_Noreturn void
trap(struct Trapframe *tf) {
    /* The environment may have set DF and some versions
     * of GCC rely on DF being clear */
    asm volatile("cld" ::
                         : "cc");

    /* Halt the CPU if some other CPU has called panic() */
    extern char *panicstr;
    if (panicstr) asm volatile("hlt");

    /* Check that interrupts are disabled.  If this assertion
     * fails, DO NOT be tempted to fix it by inserting a "cli" in
     * the interrupt path */
    assert(!(read_rflags() & FL_IF));

    if (trace_traps) cprintf("Incoming TRAP[%ld] frame at %p\n", tf->tf_trapno, tf);
    if (trace_traps_more) print_trapframe(tf);

    /* #PF should be handled separately */
    if (tf->tf_trapno == T_PGFLT) {
        assert(current_space);
        assert(!in_page_fault);
        in_page_fault = 1;

        uintptr_t va = rcr2();

#if defined(SANITIZE_USER_SHADOW_BASE) && LAB == 8
        /* NOTE: Hack!
         * This is an early user address sanitizer memory allocation
         * hook until proper memory allocation syscalls
         * and userspace pagefault handlers are implemented */
        if ((tf->tf_err & ~FEC_W) == FEC_U && curenv && SANITIZE_USER_SHADOW_BASE <= va &&
            va < SANITIZE_USER_SHADOW_BASE + SANITIZE_USER_SHADOW_SIZE) {
            int res = map_region(&curenv->address_space, ROUNDDOWN(va, PAGE_SIZE),
                                 NULL, 0, PAGE_SIZE, ALLOC_ONE | PROT_R | PROT_W | PROT_USER_);
            assert(!res);
        }
#endif

        /* If #PF was caused by write it can be lazy copying/allocation (fast path)
         * It is required to be handled here because of in-kernel page faults
         * which can happen with curenv == NULL */

        /* Read processor's CR2 register to find the faulting address */
        int res = force_alloc_page(current_space, va, MAX_ALLOCATION_CLASS);
        if (trace_pagefaults) {
            bool can_redir = false;
            cprintf("<%p> Page fault ip=%08lX va=%08lX err=%c%c%c%c%c -> %s\n", current_space, tf->tf_rip, va,
                    tf->tf_err & FEC_P ? 'P' : '-',
                    tf->tf_err & FEC_U ? 'U' : '-',
                    tf->tf_err & FEC_W ? 'W' : '-',
                    tf->tf_err & FEC_R ? 'R' : '-',
                    tf->tf_err & FEC_I ? 'I' : '-',
                    res ? can_redir ? "redirected to user" : "fault" : "resolved by kernel");
        }
        if (!res) {
            in_page_fault = 0;
            env_pop_tf(tf);
        }
    }

    assert(curenv);

    /* Copy trap frame (which is currently on the stack)
     * into 'curenv->env_tf', so that running the environment
     * will restart at the trap point */
    curenv->env_tf = *tf;
    /* The trapframe on the stack should be ignored from here on */
    tf = &curenv->env_tf;

    /* Record that tf is the last real trapframe so
     * print_trapframe can print some additional information */
    last_tf = tf;

    /* Dispatch based on what type of trap occurred */
    trap_dispatch(tf);

    /* If we made it to this point, then no other environment was
     * scheduled, so we should return to the current environment
     * if doing so makes sense */
    if (curenv && curenv->env_status == ENV_RUNNING)
        env_run(curenv);
    else
        sched_yield();
}
