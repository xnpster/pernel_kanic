/* Test user-level fault handler -- just exit when we fault */

#include <inc/lib.h>

bool
handler(struct UTrapframe *utf) {
    void *addr = (void *)utf->utf_fault_va;
    uint64_t err = utf->utf_err;
    cprintf("i faulted at va %lx, err %x\n",
            (unsigned long)addr, (unsigned)(err & 7));
    sys_env_destroy(sys_getenvid());
    return 1;
}

void
umain(int argc, char **argv) {
    add_pgfault_handler(handler);
    *(volatile int *)0xDEADBEEF = 0;
}
