; @file
; Copyright (c) 2020, ISP RAS. All rights reserved.
; SPDX-License-Identifier: BSD-3-Clause

bits 32

SECTION .text
    align 4

%macro GDT_DESC 2
    dw 0xFFFF, 0
    db 0, %1, %2, 0
%endmacro

GDT_BASE:
    dq  0x0             ; NULL segment
LINEAR_CODE_SEL:        equ $ - GDT_BASE
    GDT_DESC 0x9A, 0xCF
LINEAR_DATA_SEL:        equ $ - GDT_BASE
    GDT_DESC 0x92, 0xCF
LINEAR_CODE64_SEL:      equ $ - GDT_BASE
    GDT_DESC 0x9A, 0xAF
LINEAR_DATA64_SEL:      equ $ - GDT_BASE
    GDT_DESC 0x92, 0xCF

GDT_DESCRIPTOR:
    dw 0x28 - 1 
    dd GDT_BASE
    dd 0x0

KERNEL_ENTRY:
    dd 0

LOADER_PARAMS:
    dd 0
    
PAGE_TABLE:
    dd 0

global ASM_PFX(IsCpuidSupportedAsm)
ASM_PFX(IsCpuidSupportedAsm):
    ; Store original EFLAGS for later comparison.
    pushf
    ; Store current EFLAGS.
    pushf
    ; Invert the ID bit in stored EFLAGS.
    xor dword [esp], 0x200000
    ; Load stored EFLAGS (with ID bit inverted).
    popf
    ; Store EFLAGS again (ID bit may or may not be inverted).
    pushf
    ; Read modified EFLAGS (ID bit may or may not be inverted).
    pop eax
    ; Enable bits in RAX to whichver bits in EFLAGS were changed.
    xor eax, [esp]
    ; Restore stack pointer.
    popf
    ; Leave only the ID bit EFLAGS change result in RAX.
    and eax, 0x200000
    ; Shift it to the lowest bit be boolean compatible.
    shr eax, 21
    ; Return.
    ret

global ASM_PFX(CallKernelThroughGateAsm)
ASM_PFX(CallKernelThroughGateAsm):
    ; Transitioning from protected mode to long mode is described in Intel SDM
    ; 9.8.5 Initializing IA-32e Mode. More detailed explanation why paging needs
    ; to be disabled is explained in 4.1.2 Paging-Mode Enabling.

    ; Disable interrupts.
    cli

    ; Drop return pointer as we no longer need it.
    pop ecx

    ; Save kernel entry point passed by the bootloader.
    pop ecx
    mov eax, KERNEL_ENTRY
    mov [eax], ecx

    ; Save loading params address passed by the bootloader.
    pop ecx
    mov eax, LOADER_PARAMS
    mov [eax], ecx

    ; Save identity page table passed by the bootloader.
    pop ecx
    mov eax, PAGE_TABLE
    mov [eax], ecx

    ; 1. Disable paging.
    ; LAB 2: Your code here:
    mov eax, cr0
    and eax, 0x7FFFFFFF
    mov cr0, eax

    ; 2. Switch to our GDT that supports 64-bit mode and update CS to LINEAR_CODE_SEL.
    ; LAB 2: Your code here:
    lgdt [GDT_DESCRIPTOR]
    jmp LINEAR_CODE_SEL:AsmWithOurGdt

AsmWithOurGdt:

    ; 3. Reset all the data segment registers to linear mode (LINEAR_DATA_SEL).
    ; LAB 2: Your code here:
    mov eax, LINEAR_DATA_SEL
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 4. Enable PAE/PGE in CR4, which is required to transition to long mode.
    ; This may already be enabled by the firmware but is not guaranteed.
    ; LAB 2: Your code here:
    mov eax, cr4
    or eax, 1 << 5
    or eax, 1 << 7
    mov cr4, eax

    ; 5. Update page table address register (C3) right away with the supplied PAGE_TABLE.
    ; This does nothing as paging is off at the moment as paging is disabled.
    ; LAB 2: Your code here:
    mov eax, [PAGE_TABLE]
    mov cr3, eax

    ; 6. Enable long mode (LME) and execute protection (NXE) via the EFER MSR register.
    ; LAB 2: Your code here:
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    or eax, 1 << 11
    wrmsr

    ; 7. Enable paging as it is required in 64-bit mode.
    ; LAB 2: Your code here:
    mov eax, cr0
    or eax, 1 
    or eax, 1 << 16
    or eax, 1 << 18
    or eax, 1 << 31
    mov cr0, eax

    ; 8. Transition to 64-bit mode by updating CS with LINEAR_CODE64_SEL.
    ; LAB 2: Your code here:
    jmp LINEAR_CODE64_SEL:AsmInLongMode

AsmInLongMode:
    BITS 64

    ; 9. Reset all the data segment registers to linear 64-bit mode (LINEAR_DATA64_SEL).
    ; LAB 2: Your code here:
    mov rax, LINEAR_DATA64_SEL
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 10. Jump to the kernel code.
    mov ecx, [REL LOADER_PARAMS]
    mov ebx, [REL KERNEL_ENTRY]
    jmp rbx

noreturn:
    hlt
    jmp noreturn