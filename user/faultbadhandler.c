/* Test bad pointer for user-level fault handler
 * this is going to fault in the fault handler accessing rip (always!)
 * so eventually the kernel kills it (PFM_KILL) because
 * we outrun the stack with invocations of the user-level handler */

#include <inc/lib.h>

void
umain(int argc, char **argv) {
#ifndef __clang_analyzer__
    sys_alloc_region(0, (void *)(USER_EXCEPTION_STACK_TOP - PAGE_SIZE), PAGE_SIZE, PROT_RW);
    sys_env_set_pgfault_upcall(0, (void *)0xDEADBEEF);
    *(volatile int *)0 = 0;
#endif
}
