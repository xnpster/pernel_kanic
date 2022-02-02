 # See COPYRIGHT for copyright information.

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <kern/asm64.h>

# Copy "count" bytes from "src" to %rsp, using
# "tmpindex" for a scratch counter and %rax

#define COPY_STACK(src, count, tmpindex)                                     \
    mov $0, tmpindex             /* initial scratch counter */;              \
copyloop:                                                                    \
    mov 0(src,tmpindex,1), %rax  /* copy one 64-bit word from source... */;  \
    mov %rax, 0(%rsp,tmpindex,1) /* ... to stack */;                         \
    add $8, tmpindex             /* increment counter */;                    \
    cmp count, tmpindex          /* exit it stack has been copied */;        \
    jne copyloop

.code64
.text

#  void
#  pal_efi_call_in_64bit_mode_asm(uint64_t func,
#                             struct pal_efi_registers *efi_reg,
#                             void *stack_contents,
#                             size_t stack_contents_size)
# Switch from compatibility mode to long mode, and
# then execute the function pointer with the specified
# register and stack contents (based at %rsp). Afterwards,
# collect the return value, restore the original state,
# and return.

.align 2

.globl _efi_call_in_32bit_mode_asm
.type _efi_call_in_32bit_mode_asm, @function;
_efi_call_in_32bit_mode_asm:
    pushq %rbp
    movq %rsp, %rbp
    # Save non-volatile registers
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    # save parameters that we will need later
    push %rsi
    push %rcx

    # Save %rbp and align to 16-byte boundary
    # efi_reg in %rsi
    # stack_contents into %rdx
    # s_c_s into %rcx
    # s_c_s into %rcx
    push %rbp

    # Make room for stack contents
    sub %rcx, %rsp

    COPY_STACK(%rdx, %rcx, %r8)

    # Here in long-mode, with high kernel addresses,
    # but with the kernel double-mapped in the bottom 4GB.
    # We now switch to compat mode and call into EFI.
    ENTER_COMPAT_MODE()

    # call EFI runtime
    call *%edi

    ENTER_64BIT_MODE()

    # load efi_reg into %esi
    mov -48(%rbp), %rsi
    /* save RAX back */
    mov %rax, 32(%rsi)

    # load s_c_s into %rcx
    mov -56(%rbp), %rcx
    # discard stack contents
    add %rcx, %rsp
    # restore full 64-bit frame pointer
    # which the 32-bit EFI will have truncated
    # our full %rsp will be restored by EMARF
    pop %rbp

    pop %rcx
    pop %rsi
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx

    leave
    ret
