#include <inc/x86.h>
#include <inc/lib.h>

void wrong(int rfd, int kfd, int off, char c1, char c2);

void
umain(int argc, char **argv) {
    char c1, c2;
    int r, rfd, wfd, kfd, n1, n2, off;
    int pfds[2];

    close(0);
    close(1);
    opencons();
    opencons();

    if ((rfd = open("testshell.sh", O_RDONLY)) < 0)
        panic("open testshell.sh: %i", rfd);
    if ((wfd = pipe(pfds)) < 0)
        panic("pipe: %i", wfd);
    wfd = pfds[1];

    cprintf("running sh -x < testshell.sh | cat\n");
    if ((r = fork()) < 0)
        panic("fork: %i", r);
    if (r == 0) {
        dup(rfd, 0);
        dup(wfd, 1);
        close(rfd);
        close(wfd);
        if ((r = spawnl("/sh", "sh", "-x", 0)) < 0)
            panic("spawn: %i", r);
        close(0);
        close(1);
        wait(r);
        exit();
    }
    close(rfd);
    close(wfd);

    rfd = pfds[0];
    if ((kfd = open("testshell.key", O_RDONLY)) < 0)
        panic("open testshell.key for reading: %i", kfd);

    for (off = 0;; off++) {
        n1 = read(rfd, &c1, 1);
        n2 = read(kfd, &c2, 1);
        if (n1 < 0)
            panic("reading testshell.out: %i", n1);
        if (n2 < 0)
            panic("reading testshell.key: %i", n2);
        if (n1 == 0 && n2 == 0)
            break;
        if (n1 != 1 || n2 != 1 || c1 != c2) {
            wrong(rfd, kfd, off, c1, c2);
        }
    }
    cprintf("shell ran correctly\n");

    breakpoint();
}

void
wrong(int rfd, int kfd, int off, char c1, char c2) {
    char buf[100];
    int n;

    seek(rfd, off);
    seek(kfd, off);

    cprintf("shell produced incorrect output.\n");
    cprintf("expected:\n===\n%c", c1);
    while ((n = read(kfd, buf, sizeof buf - 1)) > 0)
        sys_cputs(buf, n);
    cprintf("===\ngot:\n===\n%c", c2);
    while ((n = read(rfd, buf, sizeof buf - 1)) > 0)
        sys_cputs(buf, n);
    cprintf("===\n");
    exit();
}
