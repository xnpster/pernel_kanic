/*
 * Institute for System Programming of the Russian Academy of Sciences
 * Copyright (C) 2017 ISPRAS
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, Version 3.
 *
 * This program is distributed in the hope # that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License version 3 for more details.
 */

#include "asan.h"
#include "asan_internal.h"
#include "asan_memintrinsics.h"
#include "inc/memlayout.h"
#include "inc/lib.h"

#if !defined(SANITIZE_USER_SHADOW_BASE) || !defined(SANITIZE_USER_SHADOW_SIZE) || !defined(SANITIZE_USER_SHADOW_OFF)
#error "You are to define SANITIZE_USER_SHADOW_BASE and SANITIZE_USER_SHADOW_SIZE for shadow memory support!"
#endif

/* FIXME: Workaround terrible decision to use -nostdinc for building userspace... */
#define __ASSEMBLER__
#include <inc/mmu.h>
#include <inc/memlayout.h>
#undef __ASSEMBLER__

/* FIXME: We need to wrap all the custom allocators we use and track the allocated memory. */
/* There should probably be more of them (especially in the kernel). */

extern uint8_t __data_start;
extern uint8_t __data_end;

extern uint8_t __rodata_start;
extern uint8_t __rodata_end;

extern uint8_t __bss_start;
extern uint8_t __bss_end;

extern uint8_t __text_start;
extern uint8_t __text_end;

void
        NORETURN
        _panic(const char *file, int line, const char *fmt, ...);

void
platform_abort() {
    _panic("asan", 0, "platform_abort");
}

#if LAB > 8

#define SHADOW_STEP 8192

/*
 * Allocate memory for shadow memory if utf->utf_fault_va
 * is inside shadow memory (but not for shadow memory describing itself)
 * (Use asan_internal_shadow_start/asan_internal_shadow_end and SHADOW_FOR_ADDRESS
 *  and sys_alloc_region to allocate region filled with 0xFF -- ALLOC_ONE)
 * Returns one if allocated something to stop chaining user PF handlers
 */

static bool
asan_shadow_allocator(struct UTrapframe *utf) {
    // LAB 9: Your code here
    (void)utf;
    return 1;
}
#endif


/* envs and vsyscall page shadow */
#define SANITIZE_USER_EXTRA_SHADOW_BASE (ROUNDDOWN(MIN(UENVS, UVSYS) >> 3, PAGE_SIZE) + SANITIZE_USER_SHADOW_OFF)
#define SANITIZE_USER_EXTRA_SHADOW_SIZE (ROUNDUP(MAX(UVSYS + PAGE_SIZE, UENVS + NENV * sizeof(struct Env)) >> 3, PAGE_SIZE) + SANITIZE_USER_SHADOW_OFF - SANITIZE_USER_EXTRA_SHADOW_BASE)

/* UVPT is located at another specific address space */
#define SANITIZE_USER_VPT_SHADOW_BASE (ROUNDDOWN(UVPT >> 3, PAGE_SIZE) + SANITIZE_USER_SHADOW_OFF)
#define SANITIZE_USER_VPT_SHADOW_SIZE (ROUNDUP((UVPML4 + HUGE_PAGE_SIZE) >> 3, PAGE_SIZE) + SANITIZE_USER_SHADOW_OFF - SANITIZE_USER_VPT_SHADOW_BASE)

void
platform_asan_unpoison(void *addr, size_t size) {
    asan_internal_fill_range((uptr)addr, size, 0);
}

void
platform_asan_poison(void *addr, size_t size) {
    asan_internal_fill_range((uptr)addr, size, ASAN_GLOBAL_RZ);
}

static int
asan_unpoison_shared_region(void *start, void *end, void *arg) {
    (void)start, (void)end, (void)arg;
    // LAB 8: Your code here
    return 0;
}

void
platform_asan_init(void) {
    asan_internal_shadow_start = (uint8_t *)SANITIZE_USER_SHADOW_BASE;
    asan_internal_shadow_end = (uint8_t *)SANITIZE_USER_SHADOW_BASE + SANITIZE_USER_SHADOW_SIZE;
    asan_internal_shadow_off = (uint8_t *)SANITIZE_USER_SHADOW_OFF;

#if LAB > 8
    add_pgfault_handler(asan_shadow_allocator);
#endif

    /* Unpoison the vital areas:
     *(fill with 0's using platform_asan_unpoison())*/

    /* 1. Program segments (text, data, rodata, bss) */
    // LAB 8: Your code here

    /* 2. Stacks (USER_EXCEPTION_STACK_TOP, USER_STACK_TOP) */
    // LAB 8: Your code here

    /* 3. Kernel exposed info (UENVS, UVSYS (only for lab 12)) */
    // LAB 8: Your code here

    // TODO NOTE: LAB 12 code may be here
#if LAB >= 12
    platform_asan_unpoison((void *)UVSYS, NVSYSCALLS * sizeof(int));
#endif

    /* 4. Shared pages
     * HINT: Use foreach_shared_region() with asan_unpoison_shared_region() */
    // LAB 8: Your code here
    // TODO NOTE: LAB 11 code may be here
}

void
platform_asan_fatal(const char *msg, uptr p, size_t width, unsigned access_type) {
    ASAN_LOG("Fatal error: %s (addr 0x%lx within i/o size 0x%lx of type %u), tracing:",
             msg, (long)p, (long)width, access_type);

    ASAN_DEBUG_BREAK();

    DUMP_STACK_AT_LEVEL(p, 0);
    DUMP_STACK_AT_LEVEL(p, 1);
    DUMP_STACK_AT_LEVEL(p, 2);
    DUMP_STACK_AT_LEVEL(p, 3);
    DUMP_STACK_AT_LEVEL(p, 4);
    DUMP_STACK_AT_LEVEL(p, 5);
    DUMP_STACK_AT_LEVEL(p, 6);
    DUMP_STACK_AT_LEVEL(p, 7);
    DUMP_STACK_AT_LEVEL(p, 8);
    DUMP_STACK_AT_LEVEL(p, 9);
    DUMP_STACK_AT_LEVEL(p, 10);
    DUMP_STACK_AT_LEVEL(p, 11);
    DUMP_STACK_AT_LEVEL(p, 12);
    DUMP_STACK_AT_LEVEL(p, 13);
    DUMP_STACK_AT_LEVEL(p, 14);
    DUMP_STACK_AT_LEVEL(p, 15);

    ASAN_ABORT();
}

bool
platform_asan_fakestack_enter(uint32_t *thread_id) {
    // TODO: implement!
    return true;
}

void
platform_asan_fakestack_leave() {
    // TODO: implement!
}
