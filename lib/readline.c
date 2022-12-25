#include <inc/stdio.h>
#include <inc/error.h>
#include <inc/types.h>
#include <inc/string.h>

#define BUFLEN 1024

static char buf[BUFLEN];

char *
readline(const char *prompt) {
    if (prompt) {
#if JOS_KERNEL
        cprintf("%s", prompt);
#else
        fprintf(1, "%s", prompt);
#endif
    }

    bool echo = iscons(0);

    for (size_t i = 0;;) {
        int c = getchar();

        if (c < 0) {
            if (c != -E_EOF)
                cprintf("read error: %i\n", c);
            return NULL;
        } else if ((c == '\b' || c == '\x7F')) {
            if (i) {
                if (echo) {
                    cputchar('\b');
                    cputchar(' ');
                    cputchar('\b');
                }
                i--;
            }
        } else if (c >= ' ') {
            if (i < BUFLEN - 1) {
                if (echo) {
                    cputchar(c);
                }
                buf[i++] = (char)c;
            }
        } else if (c == '\n' || c == '\r') {
            if (echo) {
                cputchar('\n');
            }
            buf[i] = 0;
            return buf;
        }
    }
}
