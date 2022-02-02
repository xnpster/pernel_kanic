/* Simple implementation of cprintf console output for the kernel,
 * based on printfmt() and the kernel console's cputchar() */

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>

static void
putch(int ch, int *cnt) {
    cputchar(ch);
    (*cnt)++;
}

int
vcprintf(const char *fmt, va_list ap) {
    int count = 0;

    vprintfmt((void *)putch, &count, fmt, ap);

    return count;
}

int
cprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int res = vcprintf(fmt, ap);
    va_end(ap);

    return res;
}
