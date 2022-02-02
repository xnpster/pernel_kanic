#include <inc/mmu.h>
#include <inc/memlayout.h>

.code64

# Normal x86-64 4-level translation looks like CR3->PML4->PDP->PD->PT
# We set the PTE_MBZ bits in PDE (pgdir) to skip the last step
.set pml4,   pml4phys
.set pdpt1,  pml4 + PAGE_SIZE
.set pdpt2,  pml4 + 2*PAGE_SIZE
.set pde1,   pml4 + 3*PAGE_SIZE
.set pde2,   pml4 + 4*PAGE_SIZE

# Should be less than 4096/8 * 11
#define PML_SIZE 5632

.text
.globl _head64
_head64:
    # Disable interrupts.
    cli

    # Save Loader_block pointer from Bootloader.c in r12
    movq %rcx, %r12

    # Build an early boot pml4 at pml4phys (physical = virtual for it)

    # Initialize the page tables.
    movq $pml4, %rdi
    xorl %eax, %eax
    movq $PML_SIZE, %rcx  # moving these many words to the 11 pages
    rep stosq

    # Creating a 4G boot page table...

    # Setting the 4-level page table with only the second entry needed (PML4)
    movq $pml4, %rdi
    movq $(pdpt1 + (PTE_P | PTE_W)), %rax
    stosq
    movq %rax, (%rdi)
    movq $(pdpt2 + (PTE_P | PTE_W)), %rax
    stosq

    # Setting the 3rd level page table (PDPE)
    # 4 entries (counter in ecx), point to the next four physical pages (pgdirs)
    # pgdirs in 0xA0000--0xD000
    movq $pdpt1, %rdi
    movq $(pde1 + (PTE_P | PTE_W)), %rax
    stosq

    movq $(pdpt2 + 8), %rdi
    movq $(pde2 + (PTE_P | PTE_W)), %rax
    stosq

    # Setting the pgdir so that the LA=PA
    # Mapping first 1024mb of mem at KERN_BASE_ADDR
    movq $-PDP_ENTRY_COUNT, %rcx
    movq $pde1, %rdi
    movq $pde2, %rsi
    movl $(PTE_P|PTE_W|PTE_MBZ), %eax
1:
    movq %rax, PAGE_SIZE(%rsi,%rcx,8)
    movq %rax, PAGE_SIZE(%rdi,%rcx,8)
    addq $HUGE_PAGE_SIZE, %rax
    incq %rcx
    jnz 1b

    # Update CR3 register
    movq $pml4, %rax
    movq %rax, %cr3

    # Transition to high mem entry code and pass LoadParams address
    movabs $entry, %rax
    movq %r12, %rcx
    jmpq *%rax

.data

.p2align PAGE_SHIFT
.globl pml4phys
pml4phys:
.space 11*PAGE_SIZE

.globl pdefreestart
.set pdefreestart, pde2 + PAGE_SIZE

.globl pdefreeend
.set pdefreeend, pml4phys + 11*PAGE_SIZE
