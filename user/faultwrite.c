/* Buggy program - faults with a write to location zero */

#include <inc/lib.h>

void
umain(int argc, char **argv) {
#ifndef __clang_analyzer__
    *(volatile unsigned *)0 = 0;
#endif
}
