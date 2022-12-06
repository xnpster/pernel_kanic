/* Stripped-down primitive printf-style formatting routines,
 * used in common by printf, sprintf, fprintf, etc.
 * This code is also used by both the kernel and user programs. */

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/stdarg.h>
#include <inc/error.h>

/*
 * Space or zero padding and a field width are supported for the numeric
 * formats only.
 *
 * The special format %i takes an integer error code
 * and prints a string describing the error.
 * The integer may be positive or negative,
 * so that -E_NO_MEM and E_NO_MEM are equivalent.
 */

static const char *const error_string[MAXERROR] = {
        [E_UNSPECIFIED] = "unspecified error",
        [E_BAD_ENV] = "bad environment",
        [E_INVAL] = "invalid parameter",
        [E_NO_MEM] = "out of memory",
        [E_NO_FREE_ENV] = "out of environments",
        [E_BAD_DWARF] = "corrupted debug info",
        [E_FAULT] = "segmentation fault",
        [E_INVALID_EXE] = "invalid ELF image",
        [E_NO_ENT] = "entry not found",
        [E_NO_SYS] = "no such system call",
        [E_IPC_NOT_RECV] = "env is not recving",
        [E_EOF] = "unexpected end of file",
};

/*
 * Print a number (base <= 16) in reverse order,
 * using specified putch function and associated pointer putdat.
 */
static void
print_num(void (*putch)(int, void *), void *put_arg,
          uintmax_t num, unsigned base, int width, char padc, bool capital) {
    /* First recursively print all preceding (more significant) digits */
    if (num >= base) {
        print_num(putch, put_arg, num / base, base, width - 1, padc, capital);
    } else {
        /* Print any needed pad characters before first digit */
        while (--width > 0) {
            putch(padc, put_arg);
        }
    }

    const char *dig = capital ? "0123456789ABCDEF" : "0123456789abcdef";

    /* Then print this (the least significant) digit */
    putch(dig[num % base], put_arg);
}

/* Get an unsigned int of various possible sizes from a varargs list,
 * depending on the lflag parameter. */
static uintmax_t
get_unsigned(va_list *ap, int lflag, bool zflag) {
    if (zflag) return va_arg(*ap, size_t);

    switch (lflag) {
    case 0:
        return va_arg(*ap, unsigned int);
    case 1:
        return va_arg(*ap, unsigned long);
    default:
        return va_arg(*ap, unsigned long long);
    }
}

/* Same as getuint but signed - can't use getuint
 * because of sign extension */
static intmax_t
get_int(va_list *ap, int lflag, bool zflag) {
    if (zflag) return va_arg(*ap, size_t);

    switch (lflag) {
    case 0:
        return va_arg(*ap, int);
    case 1:
        return va_arg(*ap, long);
    default:
        return va_arg(*ap, long long);
    }
}

/* Main function to format and print a string. */
void printfmt(void (*putch)(int, void *), void *put_arg, const char *fmt, ...);

void
vprintfmt(void (*putch)(int, void *), void *put_arg, const char *fmt, va_list ap) {
    const unsigned char *ufmt = (unsigned char *)fmt;

    va_list aq;
    va_copy(aq, ap);

    for (;;) {
        unsigned char ch;
        while ((ch = *ufmt++) != '%') {
            if (!ch) return;
            putch(ch, put_arg);
        }

        /* Process a %-escape sequence */
        char padc = ' ';
        int width = -1, precision = -1;
        unsigned lflag = 0, base = 10;
        bool altflag = 0, zflag = 0;
        uintmax_t num = 0;
    reswitch:

        switch (ch = *ufmt++) {
        case '0': /* '-' flag to pad on the right */
        case '-': /* '0' flag to pad with 0's instead of spaces */
            padc = ch;
            goto reswitch;

        case '*': /* Indirect width field */
            precision = va_arg(aq, int);
            goto process_precision;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': /* width field */
            for (precision = 0;; ++ufmt) {
                precision = precision * 10 + ch - '0';
                if ((ch = *ufmt) - '0' > 9) break;
            }

        process_precision:
            if (width < 0) {
                width = precision;
                precision = -1;
            }
            goto reswitch;

        case '.':
            width = MAX(0, width);
            goto reswitch;

        case '#':
            altflag = 1;
            goto reswitch;

        case 'l': /* long flag (doubled for long long) */
            lflag++;
            goto reswitch;

        case 'z':
            zflag = 1;
            goto reswitch;

        case 'c': /* character */
            putch(va_arg(aq, int), put_arg);
            break;

        case 'i': /* error message */ {
            int err = va_arg(aq, int);
            const char *strerr;

            if (err < 0) err = -err;

            if (err >= MAXERROR || !(strerr = error_string[err])) {
                printfmt(putch, put_arg, "error %d", err);
            } else {
                printfmt(putch, put_arg, "%s", strerr);
            }
            break;
        }

        case 's': /* string */ {
            const char *ptr = va_arg(aq, char *);
            if (!ptr) ptr = "(null)";

            if (width > 0 && padc != '-') {
                width -= strnlen(ptr, precision);

                while (width-- > 0) putch(padc, put_arg);
            }

            for (; (ch = *ptr++) && (precision < 0 || --precision >= 0); width--) {
                putch(altflag && (ch < ' ' || ch > '~') ? '?' : ch, put_arg);
            }

            while (width-- > 0) putch(' ', put_arg);
            break;
        }

        case 'd': /* (signed) decimal */ {
            intmax_t i = get_int(&aq, lflag, zflag);
            if (i < 0) {
                putch('-', put_arg);
                i = -i;
            }
            num = i;
            /* base = 10; */
            goto number;
        }

        case 'u': /* unsigned decimal */
            num = get_unsigned(&aq, lflag, zflag);
            /* base = 10; */
            goto number;

        case 'o': /* (unsigned) octal */
            // LAB 1: Your code here:
            num = get_unsigned(&aq, lflag, zflag);
            base = 8;
            goto number;

        case 'p': /* pointer */
            putch('0', put_arg);
            putch('x', put_arg);
            num = (uintptr_t)va_arg(aq, void *);
            base = 16;
            goto number;

        case 'X': /* (unsigned) hexadecimal, uppercase */
        case 'x': /* (unsigned) hexadecimal, lowercase */
            num = get_unsigned(&aq, lflag, zflag);
            base = 16;
        number:
            print_num(putch, put_arg, num, base, width, padc, ch == 'X');
            break;

        case '%': /* escaped '%' character */
            putch(ch, put_arg);
            break;

        default: /* unrecognized escape sequence - just print it literally */
            putch('%', put_arg);
            while ((--ufmt)[-1] != '%') /* nothing */
                ;
        }
    }
}

void
printfmt(void (*putch)(int, void *), void *putdat, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vprintfmt(putch, putdat, fmt, ap);
    va_end(ap);
}

struct sprintbuf {
    char *start;
    char *end;
    int count;
};

static void
sprintputch(int ch, struct sprintbuf *state) {
    state->count++;
    if (state->start < state->end) {
        *state->start++ = ch;
    }
}

int
vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {
    struct sprintbuf state = {buf, buf + n - 1, 0};

    if (!buf || n < 1) return -E_INVAL;

    /* Print the string to the buffer */
    vprintfmt((void *)sprintputch, &state, fmt, ap);

    /* Null terminate the buffer */
    *state.start = '\0';

    return state.count;
}

int
snprintf(char *buf, size_t n, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    int rc = vsnprintf(buf, n, fmt, ap);
    va_end(ap);

    return rc;
}
