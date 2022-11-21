/* Implementation of cprintf console output for user environments,
 * based on printfmt() and the sys_cputs() system call.
 *
 * cprintf is a debugging statement, not a generic output statement.
 * It is very important that it always go to the console, especially when
 * debugging file descriptor code! */

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/lib.h>

#define PRINTBUFSZ 256

/* Collect up to 256 characters into a buffer
 * and perform ONE system call to print all of them,
 * in order to make the lines output to the console atomic
 * and prevent interrupts from causing context switches
 * in the middle of a console output line and such. */
struct printbuf {
    int offset; /* current buffer index */
    int count;  /* total bytes printed so far */
    char buf[PRINTBUFSZ];
};

static void
putch(int ch, struct printbuf *state) {
    state->buf[state->offset++] = (char)ch;
    if (state->offset == PRINTBUFSZ - 1) {
        sys_cputs(state->buf, state->offset);
        state->offset = 0;
    }
    state->count++;
}

int
vcprintf(const char *fmt, va_list ap) {
    struct printbuf state = {0};

    vprintfmt((void *)putch, &state, fmt, ap);
    sys_cputs(state.buf, state.offset);

    return state.count;
}

int
cprintf(const char *fmt, ...) {
    va_list ap;
    int count;

    va_start(ap, fmt);
    count = vcprintf(fmt, ap);
    va_end(ap);

    return count;
}
