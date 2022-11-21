/* Test evil pointer for user-level fault handler */

#include <inc/lib.h>

void
umain(int argc, char **argv) {
#ifndef __clang_analyzer__
    sys_alloc_region(0, (void *)(USER_EXCEPTION_STACK_TOP - PAGE_SIZE), PAGE_SIZE, PROT_RW);
    sys_env_set_pgfault_upcall(0, (void *)0xF0100020);
    *(volatile int *)0 = 0;
#endif
}
