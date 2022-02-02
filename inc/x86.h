#ifndef JOS_INC_X86_H
#define JOS_INC_X86_H

#include <inc/types.h>

static inline void __attribute__((always_inline))
breakpoint(void) {
    asm volatile("int3");
}

static inline uint8_t __attribute__((always_inline))
inb(int port) {
    uint8_t data;
    asm volatile("inb %w1,%0"
                 : "=a"(data)
                 : "d"(port));
    return data;
}

static inline void __attribute__((always_inline))
insb(int port, void *addr, int cnt) {
    asm volatile("cld\n\trepne\n\tinsb"
                 : "=D"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "memory", "cc");
}

static inline uint16_t __attribute__((always_inline))
inw(int port) {
    uint16_t data;
    asm volatile("inw %w1,%0"
                 : "=a"(data)
                 : "d"(port));
    return data;
}

static inline void __attribute__((always_inline))
insw(int port, void *addr, int cnt) {
    asm volatile("cld\n\trepne\n\tinsw"
                 : "=D"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "memory", "cc");
}

static inline uint32_t __attribute__((always_inline))
inl(int port) {
    uint32_t data;
    asm volatile("inl %w1,%0"
                 : "=a"(data)
                 : "d"(port));
    return data;
}

static inline void __attribute__((always_inline))
insl(int port, void *addr, int cnt) {
    asm volatile("cld\n\trepne\n\tinsl"
                 : "=D"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "memory", "cc");
}

static inline void __attribute__((always_inline))
outb(int port, uint8_t data) {
    asm volatile("outb %0,%w1" ::"a"(data), "d"(port));
}

static inline void __attribute__((always_inline))
outsb(int port, const void *addr, int cnt) {
    asm volatile("cld\n\trepne\n\toutsb"
                 : "=S"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "cc");
}

static inline void __attribute__((always_inline))
outw(int port, uint16_t data) {
    asm volatile("outw %0,%w1" ::"a"(data), "d"(port));
}

static inline void __attribute__((always_inline))
outsw(int port, const void *addr, int cnt) {
    asm volatile("cld\n\trepne\n\toutsw"
                 : "=S"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "cc");
}

static inline void __attribute__((always_inline))
outsl(int port, const void *addr, int cnt) {
    asm volatile("cld\n\trepne\n\toutsl"
                 : "=S"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "cc");
}

static inline void __attribute__((always_inline))
outl(int port, uint32_t data) {
    asm volatile("outl %0,%w1" ::"a"(data), "d"(port));
}

static inline void __attribute__((always_inline))
invlpg(void *addr) {
    asm volatile("invlpg (%0)" ::"r"(addr)
                 : "memory");
}

static inline void __attribute__((always_inline))
lidt(void *p) {
    asm volatile("lidt (%0)" ::"r"(p));
}

static inline void __attribute__((always_inline))
lldt(uint16_t sel) {
    asm volatile("lldt %0" ::"r"(sel));
}

static inline void __attribute__((always_inline))
lgdt(void *p) {
    asm volatile("lgdt (%0)" ::"r"(p));
}

static inline void __attribute__((always_inline))
ltr(uint16_t sel) {
    asm volatile("ltr %0" ::"r"(sel));
}

static inline void __attribute__((always_inline))
lcr0(uint64_t val) {
    asm volatile("movq %0,%%cr0" ::"r"(val));
}

static inline uint64_t __attribute__((always_inline))
rcr0(void) {
    uint64_t val;
    asm volatile("movq %%cr0,%0"
                 : "=r"(val));
    return val;
}

static inline uint64_t __attribute__((always_inline))
rcr2(void) {
    uint64_t val;
    asm volatile("movq %%cr2,%0"
                 : "=r"(val));
    return val;
}

static inline void __attribute__((always_inline))
lcr3(uint64_t val) {
    asm volatile("movq %0,%%cr3" ::"r"(val));
}

static inline uint64_t __attribute__((always_inline))
rcr3(void) {
    uint64_t val;
    asm volatile("movq %%cr3,%0"
                 : "=r"(val));
    return val;
}

static inline void __attribute__((always_inline))
lcr4(uint64_t val) {
    asm volatile("movq %0,%%cr4" ::"r"(val));
}

static inline uint64_t __attribute__((always_inline))
rcr4(void) {
    uint64_t cr4;
    asm volatile("movq %%cr4,%0"
                 : "=r"(cr4));
    return cr4;
}

static inline uint64_t __attribute__((always_inline))
rdmsr(uint32_t msr) {
    uint64_t rax, rdx;
    asm volatile("rdmsr"
                 : "=a"(rax), "=d"(rdx)
                 : "c"(msr));
    return (rax & 0xFFFFFFFF) | (rdx << 32);
}

static inline void __attribute__((always_inline))
wrmsr(uint32_t msr, uint64_t val) {
    uint64_t rax = val & 0xFFFFFFFF, rdx = val >> 32;
    asm volatile("rdmsr"
                 : "=a"(rax), "=d"(rdx)
                 : "c"(msr));
}

static inline void __attribute__((always_inline))
tlbflush(void) {
    uint64_t cr3;
    asm volatile("movq %%cr3,%0"
                 : "=r"(cr3));
    asm volatile("movq %0,%%cr3" ::"r"(cr3));
}

static inline uint64_t __attribute__((always_inline))
read_rflags(void) {
    uint64_t rflags;
    asm volatile("pushfq; popq %0"
                 : "=r"(rflags));
    return rflags;
}

static inline void __attribute__((always_inline))
write_rflags(uint64_t rflags) {
    asm volatile("pushq %0; popfq" ::"r"(rflags));
}

static inline uint64_t __attribute__((always_inline))
read_rbp(void) {
    uint64_t rbp;
    asm volatile("movq %%rbp,%0"
                 : "=r"(rbp));
    return rbp;
}

static inline uint64_t __attribute__((always_inline))
read_rsp(void) {
    uint64_t rsp;
    asm volatile("movq %%rsp,%0"
                 : "=r"(rsp));
    return rsp;
}

static inline uint64_t __attribute__((always_inline))
read_rip(void) {
    uint64_t rip;
    asm volatile("call 1; 1: popq %0"
                 : "=r"(rip));
    return rip;
}

static inline void __attribute__((always_inline))
cpuid(uint32_t info, uint32_t *raxp, uint32_t *rbxp, uint32_t *rcxp, uint32_t *rdxp) {
    uint32_t eax, ebx, ecx, edx;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(info));
    if (raxp) *raxp = eax;
    if (rbxp) *rbxp = ebx;
    if (rcxp) *rcxp = ecx;
    if (rdxp) *rdxp = edx;
}

static inline uint64_t __attribute__((always_inline))
read_tsc(void) {
    uint32_t lo, hi;
    asm volatile("rdtsc"
                 : "=a"(lo), "=d"(hi));
    return (uint64_t)lo | ((uint64_t)hi << 32);
}

static inline uint32_t __attribute__((always_inline))
xchg(volatile uint32_t *addr, uint32_t newval) {
    uint32_t result = __atomic_exchange_n(addr, newval, __ATOMIC_ACQ_REL);
#if 0 /* Inline asm version of this fucntion */
    /* The + in "+m" denotes a read-modify-write operand. */
    asm volatile("lock; xchgl %0, %1"
                 : "+m"(*addr), "=a"(result)
                 : "1"(newval)
                 : "cc");
#endif
    return result;
}

#define CMOS_NMI_LOCK 0x80
#define CMOS_CMD      0x070 /* RTC control port */
#define CMOS_DATA     0x071 /* RTC data port */

static inline void __attribute__((always_inline))
nmi_enable(void) {
    outb(CMOS_CMD, inb(CMOS_CMD) & ~CMOS_NMI_LOCK);
}

static inline void __attribute__((always_inline))
nmi_disable(void) {
    outb(CMOS_CMD, inb(CMOS_CMD) | CMOS_NMI_LOCK);
}

#endif /* !JOS_INC_X86_H */
