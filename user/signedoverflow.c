/* Test for UBSAN support - signed integer overflow */

#include <inc/lib.h>

void
umain(int argc, char **argv) {
    /* Creating a 32-bit integer variable with the maximum integer value it can contain */
    int a = 2147483647;
    /* Trying to add 1 to the "a" variable and print its contents (which causes undefined behavior). */
    /* The "cprintf" function is sanitized by UBSAN because lib/Makefrag accesses the USER_SAN_CFLAGS variable. */
    cprintf("%d\n", a + 1);
}
