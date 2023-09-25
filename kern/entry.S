/* See COPYRIGHT for copyright information. */


#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <kern/macro.h>

.code64
.text

.globl entry
entry:
    # Set up kernel stack
    leaq bootstacktop(%rip),%rsp
    xor %ebp, %ebp

    # Save LoadParams in uefi_lp.
    movq %rcx, uefi_lp(%rip)

    # Invoke C code
    call i386_init
    # Should never reach this place
    jmp .

#ifdef CONFIG_KSPACE

.global save_trapframe_trap
save_trapframe_trap:
    popq save_trapframe_ret(%rip)
    popq saved_rip(%rip)
    popq saved_cs(%rip)
    popq saved_rflags(%rip)
    popq saved_rsp(%rip)
    popq saved_ss(%rip)
    jmp save_trapframe

save_trapframe_syscall:
    popq save_trapframe_ret(%rip)
    popq saved_rip(%rip)
    movq %rsp, saved_rsp(%rip)
    pushfq
    movq $GD_KD, saved_ss(%rip)
    movq $GD_KT, saved_cs(%rip)
    popq saved_rflags(%rip)
    # fallthrough

save_trapframe:
    orl $FL_IF, saved_rflags(%rip)

    # We already pushed ss,rsp,rflags,cs,rip
    # implicitly
    leaq (bootstacktop-0x28)(%rip), %rsp

    pushq $0x0
    pushq $0x0
    pushq $0x0 # %ds
    pushq $0x0 # %es

    PUSHA

    xor %ebp, %ebp
    movq %rsp, %rdi
    jmp *save_trapframe_ret(%rip)

.globl sys_yield
.type  sys_yield, @function
sys_yield:
    cli
    call save_trapframe_syscall
    call csys_yield
    jmp .

# LAB 3: Your code here:
.globl sys_exit
.type  sys_exit, @function
sys_exit:
    cli
    leaq bootstacktop(%rip), %rsp
    xor %ebp, %ebp
    call csys_exit
    jmp .

#endif


.data

# Allocate boot stack forcing page alignment.
.p2align PAGE_SHIFT
.globl bootstack
bootstack:
.space KERN_STACK_SIZE
.globl bootstacktop
bootstacktop:

# pagefault needs its own stack
.globl pfstack
pfstack:
.space KERN_PF_STACK_SIZE
.globl pfstacktop
pfstacktop:


# Provide storage for loader parameters.
.globl uefi_lp
uefi_lp:
.quad 0

#ifdef CONFIG_KSPACE
save_trapframe_ret:
.quad 0

.set saved_ss, bootstacktop-0x8
.set saved_rsp, bootstacktop-0x10
.set saved_rflags, bootstacktop-0x18
.set saved_cs, bootstacktop-0x20
.set saved_rip, bootstacktop-0x28
#endif
