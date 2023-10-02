
int (*volatile cprintf)(const char *fmt, ...);

void
umain(int argc, char **argv) {
    cprintf("TEST4 LOADED.\n");

    for (;;)
        ;
}
