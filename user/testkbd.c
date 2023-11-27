
#include <inc/lib.h>

void
umain(int argc, char **argv) {
    int r;

    /* Spin for a bit to let the console quiet */
    for (int i = 0; i < 10; ++i)
        sys_yield();

    close(0);
    if ((r = opencons()) < 0)
        panic("opencons: %i", r);
    if (r != 0)
        panic("first opencons used fd %d", r);
    if ((r = dup(0, 1)) < 0)
        panic("dup: %i", r);

    for (;;) {
        char *buf;

        buf = readline("Type a line: ");
        if (buf != NULL)
            fprintf(1, "%s\n", buf);
        else
            fprintf(1, "(end of file received)\n");
    }
}
