/* Test for UBSAN support - implicit conversion */

#include <inc/lib.h>

void
signed_impl_conv() {
    int a = 2000000;
    short b = a;

    (void)b;

    a = 12000;
    b = a;

    a = 257;
    char c = a;

    (void)b;
    (void)c;
}

void
unsigned_impl_conv() {
    unsigned long long a = 400000000000;
    unsigned int b = a;

    (void)b;
}

void
umain(int argc, char **argv) {
    signed_impl_conv();
    unsigned_impl_conv();
}
