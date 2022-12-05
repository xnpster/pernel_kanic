#include <inc/lib.h>

void
umain(int argc, char **argv) {
    int r;

    cprintf("initsh: running sh\n");

    /* Being run directly from kernel, so no file descriptors open yet */
    close(0);
    if ((r = opencons()) < 0)
        panic("opencons: %i", r);
    if (r != 0)
        panic("first opencons used fd %d", r);
    if ((r = dup(0, 1)) < 0)
        panic("dup: %i", r);
    while (1) {
        cprintf("init: starting sh\n");
        r = spawnl("/sh", "sh", (char *)0);
        if (r < 0) {
            cprintf("init: spawn sh: %i\n", r);
            continue;
        }
        wait(r);
    }
}
