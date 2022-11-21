/* Test user-level fault handler -- alloc pages to fix faults */

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
    cprintf("%s\n", (char *)0xBeefDead);
    cprintf("%s\n", (char *)0xCafeBffe);
}
