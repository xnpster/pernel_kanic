/* Buggy program - faults with a write to a kernel location */

#include <inc/lib.h>

void
umain(int argc, char **argv) {
    *(volatile unsigned *)0x8040000000 = 0;
}
