/* Test user-level fault handler -- alloc pages to fix faults
 * doesn't work because we sys_cputs instead of cprintf (exercise: why?) */

#include <inc/lib.h>

bool
handler(struct UTrapframe *utf) {
    int r;
    void *addr = (void *)utf->utf_fault_va;

    cprintf("fault %lx\n", (unsigned long)addr);
    if ((r = sys_alloc_region(0, ROUNDDOWN(addr, PAGE_SIZE),
                              PAGE_SIZE, PROT_RW)) < 0)
        panic("allocating at %lx in page fault handler: %i", (unsigned long)addr, r);
    snprintf((char *)addr, 100, "this string was faulted in at %lx", (unsigned long)addr);
    return 1;
}

void
umain(int argc, char **argv) {
    add_pgfault_handler(handler);
    sys_cputs((char *)0xDEADBEEF, 4);
}
