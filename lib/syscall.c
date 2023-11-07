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
