/* Concurrent version of prime sieve of Eratosthenes.
 * Invented by Doug McIlroy, inventor of Unix pipes.
 * See http://swtch.com/~rsc/thread/.
 * The picture halfway down the page and the text surrounding it
 * explain what's going on here.
 *
 * Since NENVS is 1024, we can print 1022 primes before running out.
 * The remaining two environments are the integer generator at the bottom
 * of main and user/idle. */

#include <inc/lib.h>

unsigned
primeproc(int fd) {
    int id, p, pfd[2], wfd, r;

    /* Fetch a prime from our left neighbor */
top:
    if ((r = readn(fd, &p, 4)) != 4)
        panic("primeproc could not read initial prime: %d, %i", r, r >= 0 ? 0 : r);

    cprintf("%d\n", p);

    /* Fork a right neighbor to continue the chain */
    if ((r = pipe(pfd)) < 0)
        panic("pipe: %i", r);
    if ((id = fork()) < 0)
        panic("fork: %i", id);
    if (id == 0) {
        close(fd);
        close(pfd[1]);
        fd = pfd[0];
        goto top;
    }

    close(pfd[0]);
    wfd = pfd[1];

    /* Filter out multiples of our prime */
    for (int i;;) {
        if ((r = readn(fd, &i, 4)) != 4)
            panic("primeproc %d readn %d %d %i", p, fd, r, r >= 0 ? 0 : r);
        if (i % p)
            if ((r = write(wfd, &i, 4)) != 4)
                panic("primeproc %d write: %d %i", p, r, r >= 0 ? 0 : r);
    }
}

void
umain(int argc, char **argv) {
    int id, p[2], r;

    binaryname = "primespipe";

    if ((r = pipe(p)) < 0)
        panic("pipe: %i", r);

    /* Fork the first prime process in the chain */
    if ((id = fork()) < 0)
        panic("fork: %i", id);

    if (id == 0) {
        close(p[1]);
        primeproc(p[0]);
    }

    close(p[0]);

    /* Feed all the integers through */
    for (int i = 2;; i++)
        if ((r = write(p[1], &i, 4)) != 4)
            panic("generator write: %d, %i", r, r >= 0 ? 0 : r);
}
