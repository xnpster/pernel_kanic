/* Buggy program - faults with a read from location zero */

#include <inc/lib.h>

void
umain(int argc, char **argv) {
#ifndef __clang_analyzer__
    cprintf("I read %08x from location 0!\n", *(volatile unsigned *)0);
#endif
}
