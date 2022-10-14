/* Mutual exclusion spin locks. */

#include <inc/types.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/memlayout.h>
#include <inc/string.h>
#include <kern/spinlock.h>
#include <kern/kdebug.h>
#include <kern/traceopt.h>

/* The big kernel lock */
struct spinlock kernel_lock = {
#if trace_spinlock
        .name = "kernel_lock"
#endif
};

#if trace_spinlock
/* Record the current call stack in pcs[] by following the %rbp chain. */
static void
get_caller_pcs(uint64_t pcs[]) {
    uintptr_t *rbp = (uintptr_t *)read_rbp();
    int i = 0;

    for (; i < 10; i++) {
        if (rbp == 0 || rbp < (uintptr_t *)MAX_USER_READABLE) break;
        pcs[i] = rbp[1];           /* saved %rip */
        rbp = (uintptr_t *)rbp[0]; /* saved %rbp */
    }
    while (i < 10) pcs[i++] = 0;
}

/* Check whether this CPU is holding the lock. */
static int
holding(struct spinlock *lock) {
    return lock->locked;
}
#endif

void
__spin_initlock(struct spinlock *lk, char *name) {
    lk->locked = 0;
#if trace_spinlock
    lk->name = name;
#endif
}

/* Acquire the lock.
 * Loops (spins) until the lock is acquired.
 * Holding a lock for a long time may cause
 * other CPUs to waste time spinning to acquire it. */
void
spin_lock(struct spinlock *lk) {
#if trace_spinlock
    if (holding(lk)) panic("Cannot acquire %s: already holding", lk->name);
#endif

    /* The xchg is atomic.
     * It also serializes, so that reads after acquire are not
     * reordered before it. */
    while (xchg(&lk->locked, 1)) asm volatile("pause");

        /* Record info about lock acquisition for debugging. */
#if trace_spinlock
    get_caller_pcs(lk->pcs);
#endif
}

/* Release the lock. */
void
spin_unlock(struct spinlock *lk) {
#if trace_spinlock
    if (!holding(lk)) {
        uintptr_t pcs[10];
        /* Nab the acquiring EIP chain before it gets released */
        memmove(pcs, lk->pcs, sizeof pcs);
        cprintf("Cannot release %s\nAcquired at:", lk->name);
        for (int i = 0; i < 10 && pcs[i]; i++) {
            struct Ripdebuginfo info;
            if (debuginfo_rip(pcs[i], &info) >= 0) {
                cprintf("  %08lx %s:%d: %.*s+%lx\n", pcs[i],
                        info.rip_file, info.rip_line,
                        info.rip_fn_namelen, info.rip_fn_name,
                        pcs[i] - info.rip_fn_addr);
            } else {
                cprintf("  %08lx\n", pcs[i]);
            }
        }
        panic("spin_unlock");
    }

    lk->pcs[0] = 0;
#endif

    /* The xchg serializes, so that reads before release are
     * not reordered after it.  The 1996 PentiumPro manual (Volume 3,
     * 7.2) says reads can be carried out speculatively and in
     * any order, which implies we need to serialize here.
     * But the 2007 Intel 64 Architecture Memory Ordering White
     * Paper says that Intel 64 and IA-32 will not move a load
     * after a store. So lock->locked = 0 would work here.
     * The xchg being asm volatile ensures gcc emits it after
     * the above assignments (and after the critical section). */
    xchg(&lk->locked, 0);
}
