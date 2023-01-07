#include <inc/lib.h>


char* c2;

void
test_fakestack_internal() {
    char c1[] = {'A', 'B', 'C', 'D', 'E'};
    c2 = (char *)(c1 + 2);
}

void
test_fakestack_internal2() {
    char c1[] = {'A', 'B', 'c', 'D', 'E'};
    for(int k = 0; k < sizeof(c1)/sizeof(*c1); k++) {
        cprintf("%c", c1[k]);
    }
    cprintf("foo!\n");
    cprintf("ch%cr is: %c\n", c1[0], *c2);
}

void
test_fakestack(bool error) {
    cprintf("Testing fakestack...\n");
    char d = 'D';
    if(error) {
        test_fakestack_internal();
        test_fakestack_internal2();
    } else
        c2 = &d;
    cprintf("char is: %c\n", *c2);
}

void
umain(int argc, char **argv) {
    int fd, n, r;
    char buf[512 + 1];

    binaryname = "icode";
    test_fakestack(true);
    cprintf("icode startup\n");

    cprintf("icode: open /motd\n");
    if ((fd = open("/motd", O_RDONLY)) < 0)
        panic("icode: open /motd: %i", fd);

    cprintf("icode: read /motd\n");
    while ((n = read(fd, buf, sizeof buf - 1)) > 0){
        sys_cputs(buf, n);
    }

    cprintf("icode: close /motd\n");
    close(fd);

    cprintf("icode: spawn /init\n");
    if ((r = spawnl("/init", "init", "initarg1", "initarg2", (char *)0)) < 0)
        panic("icode: spawn /init: %i", r);

    cprintf("icode: exiting\n");
}
