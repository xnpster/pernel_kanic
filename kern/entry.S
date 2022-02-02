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

.data

# Allocate boot stack forcing page alignment.
.p2align PAGE_SHIFT
.globl bootstack
bootstack:
.space KERN_STACK_SIZE
.globl bootstacktop
bootstacktop:

# Provide storage for loader parameters.
.globl uefi_lp
uefi_lp:
.quad 0
