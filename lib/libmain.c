/* Called from entry.S to get us going.
 * entry.S already took care of defining envs, pages, uvpd, and uvpt */

#include <inc/lib.h>
#include <inc/x86.h>

extern void umain(int argc, char **argv);

const volatile struct Env *thisenv;
const char *binaryname = "<unknown>";

#ifdef JOS_PROG
void (*volatile sys_exit)(void);
#endif

void
libmain(int argc, char **argv) {
    /* Perform global constructor initialisation (e.g. asan)
     * This must be done as early as possible */
    extern void (*__ctors_start)(), (*__ctors_end)();
    void (**ctor)() = &__ctors_start;
    while (ctor < &__ctors_end) (*ctor++)();

    /* Set thisenv to point at our Env structure in envs[]. */

    // LAB 8: Your code here

    /* Save the name of the program so that panic() can use it */
    if (argc > 0) binaryname = argv[0];

    /* Call user main routine */
    umain(argc, argv);

#ifdef JOS_PROG
    sys_exit();
#else
    exit();
#endif
}
