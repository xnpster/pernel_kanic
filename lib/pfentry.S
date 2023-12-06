#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>
#include <kern/macro.h>

# Page fault upcall entrypoint.

# This is where we ask the kernel to redirect us to whenever we cause
# a page fault in user space (see the call to sys_set_pgfault_handler
# in pgfault.c).
#
# When a page fault actually occurs, the kernel switches our RSP to
# point to the user exception stack if we're not already on the user
# exception stack, and then it pushes a UTrapframe onto our user
# exception stack:
#
#  utf_rsp
#  utf_rflags
#  utf_rip
#  utf_regs.reg_rax
#  ...
#  utf_regs.reg_r15
#  utf_err (error code)
#  utf_fault_va <-- %rsp
#
# If this is a recursive fault, the kernel will reserve for us a
# blank word above the trap-time rsp for scratch work when we unwind
# the recursive call.
#
# We then have call up to the appropriate page fault handler in C
# code, pointed to by the global variable '_pgfault_handler'.

.text
.globl _pgfault_upcall
_pgfault_upcall:
    # Call the C page fault handler.
    movq  %rsp,%rdi # passing the function argument in rdi
    movabs $_handle_vectored_pagefault, %rax
    call *%rax

    # Now the C page fault handler has returned and you must return
    # to the trap time state.
    # Push trap-time %eip onto the trap-time stack.
    #
    # Explanation:
    #   We must prepare the trap-time stack for our eventual return to
    #   re-execute the instruction that faulted.
    #   Unfortunately, we can't return directly from the exception stack:
    #   We can't call 'jmp', since that requires that we load the address
    #   into a register, and all registers must have their trap-time
    #   values after the return.
    #   We can't call 'ret' from the exception stack either, since if we
    #   did, %esp would have the wrong value.
    #   So instead, we push the trap-time %eip onto the *trap-time* stack!
    #   Below we'll switch to that stack and call 'ret', which will
    #   restore %eip to its pre-fault value.
    #
    #   In the case of a recursive fault on the exception stack,
    #   note that the word we're pushing now will fit in the
    #   blank word that the kernel reserved for us.
    #
    # Throughout the remaining code, think carefully about what
    # registers are available for intermediate calculations.  You
    # may find that you have to rearrange your code in non-obvious
    # ways as registers become unavailable as scratch space.

    # Restore the trap-time registers.  After you do this, you
    # can no longer modify any general-purpose registers (use POPA macro).

    popq %r15
    popq %r15
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rsi
    popq %rdi
    popq %rbp
    popq %rdx
    popq %rcx

    movq 32(%rsp), %rbx
    movq 16(%rsp), %rax
    subq $8, %rbx
    movq %rax, (%rbx)
    movq %rbx, 32(%rsp)

    popq %rbx
    popq %rax

    # Restore rflags from the stack.  After you do this, you can
    # no longer use arithmetic operations or anything else that
    # modifies rflags.
    pushq 8(%rsp)
    popfq

    # Switch back to the adjusted trap-time stack.
    movq 16(%rsp), %rsp

    # Return to re-execute the instruction that faulted.
    ret
