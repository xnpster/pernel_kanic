/* Test user fault handler being called with no exception stack mapped */

#include <inc/lib.h>

void _pgfault_upcall();

void
umain(int argc, char **argv) {
#ifndef __clang_analyzer__
    sys_env_set_pgfault_upcall(0, (void *)_pgfault_upcall);
    *(volatile int *)0 = 0;
#endif
}
