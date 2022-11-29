#include <inc/lib.h>

/* Waits until 'envid' exits. */
void
wait(envid_t envid) {
    assert(envid != 0);

    const volatile struct Env *env = &envs[ENVX(envid)];

    while (env->env_id == envid &&
           env->env_status != ENV_FREE) {
        sys_yield();
    }
}
