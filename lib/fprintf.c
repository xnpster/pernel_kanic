#include <inc/lib.h>

#define PRINTBUFSZ 256

/* Collect up to PRINTBUFSZ characters into a buffer
 * and perform ONE system call to print all of them,
 * in order to make the lines output to the console atomic
 * and prevent interrupts from causing context switches
 * in the middle of a console output line and such. */
struct printbuf {
    int fd;         /* File descriptor */
    int offset;     /* Current buffer index */
    ssize_t result; /* Accumulated results from write */
    int error;      /* First error that occurred */
    char buf[PRINTBUFSZ];
};

static void
writebuf(struct printbuf *state) {
    if (state->error > 0) {
        ssize_t result = write(state->fd, state->buf, state->offset);
        if (result > 0) state->result += result;

        /* Error, or wrote less than supplied */
        if (result != state->offset)
            state->error = MIN(0, result);
    }
}

static void
putch(int ch, void *arg) {
    struct printbuf *state = (struct printbuf *)arg;
    state->buf[state->offset++] = ch;
    if (state->offset == PRINTBUFSZ) {
        writebuf(state);
        state->offset = 0;
    }
}

int
vfprintf(int fd, const char *fmt, va_list ap) {
    struct printbuf state;
    state.fd = fd;
    state.offset = 0;
    state.result = 0;
    state.error = 1;

    vprintfmt(putch, &state, fmt, ap);
    if (state.offset > 0) writebuf(&state);

    return (state.result ? state.result : state.error);
}

int
fprintf(int fd, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int res = vfprintf(fd, fmt, ap);
    va_end(ap);

    return res;
}

int
printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int res = vfprintf(1, fmt, ap);
    va_end(ap);

    return res;
}
