
#include <inc/string.h>
#include <inc/lib.h>

void
cputchar(int ch) {
    char c = (char)ch;

    /* Unlike standard Unix's putchar,
     * the cputchar function _always_ outputs to the system console. */
    sys_cputs(&c, 1);
}

int
getchar(void) {
    int c;

    /* sys_cgetc does not block, but getchar should. */
    while (!(c = sys_cgetc()))
        ;

    return c;
}

int
iscons(int fdnum) {
    return 1;
}
