/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>
#include <kern/macro.h>

#include <kern/picirq.h>

#ifdef CONFIG_KSPACE

.globl clock_thdlr
.type clock_thdlr, @function
clock_thdlr:
    call save_trapframe_trap
    # Set trap code for trapframe
    movl $(IRQ_OFFSET + IRQ_CLOCK), 136(%rsp)
    call trap
    jmp .

.globl timer_thdlr
.type timer_thdlr, @function
timer_thdlr:
    call save_trapframe_trap
    # Set trap code for trapframe
    movl $(IRQ_OFFSET + IRQ_TIMER), 136(%rsp)
    call trap
    jmp .

#else

# TRAPHANDLER defines a globally-visible function for handling a trap.
# It pushes a trap number onto the stack, then jumps to _alltraps.
# Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
#
# You shouldn't call a TRAPHANDLER function from C, but you may
# need to _declare_ one in C (for instance, to get a function pointer
# during IDT setup).  You can declare the function with
#   void NAME();
# where NAME is the argument passed to TRAPHANDLER.

#define TRAPHANDLER(name, num) \
.globl name;                   \
.type name, @function;         \
.align 2;                      \
name:                          \
    pushq $(num);              \
    jmp _alltraps

# Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
# It pushes a 0 in place of the error code, so the trap frame has the same
# format in either case.

#define TRAPHANDLER_NOEC(name, num) \
.globl name;                        \
.type name, @function;              \
.align 2;                           \
name:                               \
    pushq $0;                       \
    pushq $(num);                   \
    jmp _alltraps

.text

.globl _alltraps
.type _alltraps, @function;
.align 2
_alltraps:
  subq $16,%rsp
  movw %ds,8(%rsp)
  movw %es,(%rsp)
  PUSHA
  movl $GD_KD,%eax
  movw %ax,%ds
  movw %ax,%es
  movq %rsp, %rdi
  call trap
  jmp .

# LAB 8: Your code here
# Use TARPHANDLER or TRAPHANDLER_NOEC to setup
# all trap handlers' entry points
# (Look up their list in inc/trap.h)
TRAPHANDLER_NOEC(divide_thdlr, T_DIVIDE)
TRAPHANDLER_NOEC(debug_thdlr, T_DEBUG)
TRAPHANDLER_NOEC(nmi_thdlr, T_NMI)
TRAPHANDLER_NOEC(brkpt_thdlr, T_BRKPT)
TRAPHANDLER_NOEC(oflow_thdlr, T_OFLOW)
TRAPHANDLER_NOEC(bound_thdlr, T_BOUND)
TRAPHANDLER_NOEC(illop_thdlr, T_ILLOP)
TRAPHANDLER_NOEC(device_thdlr, T_DEVICE)
TRAPHANDLER(dblflt_thdlr, T_DBLFLT)
TRAPHANDLER(tss_thdlr, T_TSS)
TRAPHANDLER(segnp_thdlr, T_SEGNP)
TRAPHANDLER(stack_thdlr, T_STACK)
TRAPHANDLER(gpflt_thdlr, T_GPFLT)
TRAPHANDLER(pgflt_thdlr, T_PGFLT)
TRAPHANDLER_NOEC(fperr_thdlr, T_FPERR)
TRAPHANDLER(align_thdlr, T_ALIGN)
TRAPHANDLER_NOEC(mchk_thdlr, T_MCHK)
TRAPHANDLER_NOEC(simderr_thdlr, T_SIMDERR)
TRAPHANDLER_NOEC(syscall_thdlr, T_SYSCALL)

TRAPHANDLER_NOEC(clock_thdlr, IRQ_OFFSET + IRQ_CLOCK)
TRAPHANDLER_NOEC(timer_thdlr, IRQ_OFFSET + IRQ_TIMER)

TRAPHANDLER_NOEC(kbd_thdlr, IRQ_OFFSET + IRQ_KBD)
TRAPHANDLER_NOEC(serial_thdlr, IRQ_OFFSET + IRQ_SERIAL)
#endif