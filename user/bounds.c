/* Test for UBSAN support - accessing array element with an out-of-borders index */

#include <inc/lib.h>

void
umain(int argc, char **argv) {
    /* Defining a statically allocated array of 4 32-bit integers */
    int a[4] = {0};
    /* Trying to print the value of the fifth element of the array (which causes undefined behavior).
     * The "cprintf" function is sanitized by UBSAN because lib/Makefrag accesses the USER_SAN_CFLAGS variable.
     * The access operator ([]) is not used because it will trigger -Warray-bounds option of Clang,
     * which will make this test unrunnable because of -Werror flag which is specified in GNUmakefile. */
    cprintf("%d\n", *(a + 5));
}
