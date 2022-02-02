
#ifndef JOS_INC_CPU_H
#define JOS_INC_CPU_H

#include <inc/types.h>
#include <inc/memlayout.h>
#include <inc/mmu.h>
#include <inc/env.h>

#define NCPU 1

/* Used by x86 to find stack for interrupt */
extern struct Taskstate cpu_ts;

extern char in_intr;
extern bool in_clk_intr;

static inline bool
in_interrupt(void) {
    return !!in_intr;
}

static inline bool
in_clock_interrupt(void) {
    return in_intr && in_clk_intr;
}
#endif
