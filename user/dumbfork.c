/* Ping-pong a counter between two processes.
 * Only need to start one of these -- splits into two, crudely. */

#include <inc/string.h>
#include <inc/lib.h>

envid_t dumbfork(void);

void
umain(int argc, char **argv) {
    envid_t who;

    /* Fork a child process */
    who = dumbfork();

    /* Print a message and yield to the other a few times */
    for (int i = 0; i < (who ? 10 : 20); i++) {
        cprintf("%d: I am the %s!\n", i, who ? "parent" : "child");
        sys_yield();
    }
}

envid_t
dumbfork(void) {
    envid_t envid;
    int r;

    /* Allocate a new child environment.
     * The kernel will initialize it with a copy of our register state,
     * so that the child will appear to have called sys_exofork() too -
     * except that in the child, this "fake" call to sys_exofork()
     * will return 0 instead of the envid of the child. */
    envid = sys_exofork();
    if (envid < 0)
        panic("sys_exofork: %i", envid);
    if (envid == 0) {
        /* We're the child.
         * The copied value of the global variable 'thisenv'
         * is no longer valid (it refers to the parent!).
         * Fix it and return 0. */
        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }

    /* We're the parent.
     * Eagerly lazily copy our entire address space into the child. */
    sys_map_region(0, NULL, envid, NULL, MAX_USER_ADDRESS, PROT_ALL | PROT_LAZY | PROT_COMBINE);

    /* Start the child environment running */
    if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
        panic("sys_env_set_status: %i", r);

    return envid;
}
