#include <inc/error.h>
#include <inc/stdio.h>
#include <inc/memlayout.h>
#include <inc/uefi.h>

extern void _efi_call_in_32bit_mode_asm(uint32_t func, efi_registers *efi_reg, void *stack_contents, size_t stack_contents_size);

/* stack_contents_size is 16-byte multiple */
int
efi_call_in_32bit_mode(uint32_t func, efi_registers *efi_reg, void *stack_contents, size_t stack_contents_size, uint32_t *efi_status) {

    if (!func || !efi_reg || !stack_contents || (stack_contents_size & 15)) return -E_INVAL;

    /* We need to set up kernel data segments for 32 bit mode
     * before calling asm. */
    asm volatile("movw %%ax,%%es\n\t"
                 "movw %%ax,%%ds\n\t"
                 "movw %%ax,%%ss" ::"a"(GD_KD32));

    _efi_call_in_32bit_mode_asm(func, efi_reg, stack_contents, stack_contents_size);

    /* Restore 64 bit kernel data segments */
    asm volatile("movw %%ax,%%es\n\t"
                 "movw %%ax,%%ds\n\t"
                 "movw %%ax,%%ss" ::"a"(GD_KD));

    *efi_status = (uint32_t)efi_reg->rax;

    return 0;
}
