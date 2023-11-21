/* System call stubs. */

#include <inc/syscall.h>
#include <inc/lib.h>

static inline int64_t __attribute__((always_inline))
syscall(uintptr_t num, bool check, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6) {
    intptr_t ret;

    /* Generic system call.
     * Pass system call number in RAX,
     * Up to six parameters in RDX, RCX, RBX, RDI, RSI and R8.
     *
     * Registers are assigned using GCC externsion
     */

    register uintptr_t _a0 asm("rax") = num,
                           _a1 asm("rdx") = a1, _a2 asm("rcx") = a2,
                           _a3 asm("rbx") = a3, _a4 asm("rdi") = a4,
                           _a5 asm("rsi") = a5, _a6 asm("r8") = a6;

    /* Interrupt kernel with T_SYSCALL.
     *
     * The "volatile" tells the assembler not to optimize
     * this instruction away just because we don't use the
     * return value.
     *
     * The last clause tells the assembler that this can
     * potentially change the condition codes and arbitrary
     * memory locations. */

    asm volatile("int %1\n"
                 : "=a"(ret)
                 : "i"(T_SYSCALL), "r"(_a0), "r"(_a1), "r"(_a2), "r"(_a3), "r"(_a4), "r"(_a5), "r"(_a6)
                 : "cc", "memory");

    if (check && ret > 0) {
        panic("syscall %zd returned %zd (> 0)", num, ret);
    }

    return ret;
}

void
sys_cputs(const char *s, size_t len) {
    syscall(SYS_cputs, 0, (uintptr_t)s, len, 0, 0, 0, 0);
}

int
sys_cgetc(void) {
    return syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(envid_t envid) {
    return syscall(SYS_env_destroy, 1, envid, 0, 0, 0, 0, 0);
}

envid_t
sys_getenvid(void) {
    return syscall(SYS_getenvid, 0, 0, 0, 0, 0, 0, 0);
}

void
sys_yield(void) {
    syscall(SYS_yield, 0, 0, 0, 0, 0, 0, 0);
}

int
sys_region_refs(void *va, size_t size) {
    return syscall(SYS_region_refs, 0, (uintptr_t)va, size, MAX_USER_ADDRESS, 0, 0, 0);
}

int
sys_region_refs2(void *va, size_t size, void *va2, size_t size2) {
    return syscall(SYS_region_refs, 0, (uintptr_t)va, size, (uintptr_t)va2, size2, 0, 0);
}

int
sys_alloc_region(envid_t envid, void *va, size_t size, int perm) {
    int res = syscall(SYS_alloc_region, 1, envid, (uintptr_t)va, size, perm, 0, 0);
#ifdef SANITIZE_USER_SHADOW_BASE
    /* Unpoison the allocated page */
    if (!res && thisenv && envid == CURENVID && ((uintptr_t)va < SANITIZE_USER_SHADOW_BASE || (uintptr_t)va >= SANITIZE_USER_SHADOW_SIZE + SANITIZE_USER_SHADOW_BASE)) {
        platform_asan_unpoison(va, size);
    }
#endif

    return res;
}

int
sys_map_region(envid_t srcenv, void *srcva, envid_t dstenv, void *dstva, size_t size, int perm) {
    int res = syscall(SYS_map_region, 1, srcenv, (uintptr_t)srcva, dstenv, (uintptr_t)dstva, size, perm);
#ifdef SANITIZE_USER_SHADOW_BASE
    if (!res && dstenv == CURENVID)
        platform_asan_unpoison(dstva, size);
#endif
    return res;
}

int
sys_map_physical_region(uintptr_t pa, envid_t dstenv, void *dstva, size_t size, int perm) {
    int res = syscall(SYS_map_physical_region, 1, pa, dstenv, (uintptr_t)dstva, size, perm, 0);
#ifdef SANITIZE_USER_SHADOW_BASE
    platform_asan_unpoison(dstva, size);
#endif
    return res;
}

int
sys_unmap_region(envid_t envid, void *va, size_t size) {
    int res = syscall(SYS_unmap_region, 1, envid, (uintptr_t)va, size, 0, 0, 0);
#ifdef SANITIZE_USER_SHADOW_BASE
    if (!res && ((uintptr_t)va < SANITIZE_USER_SHADOW_BASE ||
                 (uintptr_t)va >= SANITIZE_USER_SHADOW_SIZE + SANITIZE_USER_SHADOW_BASE)) {
        platform_asan_poison(va, size);
    }
#endif
    return res;
}

/* sys_exofork is inlined in lib.h */

int
sys_env_set_status(envid_t envid, int status) {
    return syscall(SYS_env_set_status, 1, envid, status, 0, 0, 0, 0);
}

int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf) {
    return syscall(SYS_env_set_trapframe, 1, envid, (uintptr_t)tf, 0, 0, 0, 0);
}

int
sys_env_set_pgfault_upcall(envid_t envid, void *upcall) {
    return syscall(SYS_env_set_pgfault_upcall, 1, envid, (uintptr_t)upcall, 0, 0, 0, 0);
}

int
sys_ipc_try_send(envid_t envid, uintptr_t value, void *srcva, size_t size, int perm) {
    return syscall(SYS_ipc_try_send, 0, envid, value, (uintptr_t)srcva, size, perm, 0);
}

int
sys_ipc_recv(void *dstva, size_t size) {
    int res = syscall(SYS_ipc_recv, 1, (uintptr_t)dstva, size, 0, 0, 0, 0);
#ifdef SANITIZE_USER_SHADOW_BASE
    if (!res) platform_asan_unpoison(dstva, thisenv->env_ipc_maxsz);
#endif
    return res;
}
