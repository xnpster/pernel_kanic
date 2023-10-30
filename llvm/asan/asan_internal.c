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
#include "inc/x86.h"
#include "inc/memlayout.h"

bool asan_internal_initialised = false;

/* According to AddressSanitizer.cpp shadow scale and offset can be
 * passed via the following parameters:
 * -mllvm -asan-mapping-scale=3           # default 3 (map 8 bytes into 1)
 * -mllvm -asan-mapping-offset=0x1000000000 # default 0x20000000
 * TODO: implement scale customisation if it is proven to be necessary. */

uint8_t *asan_internal_shadow_start = NULL;
uint8_t *asan_internal_shadow_end = NULL;
uint8_t *asan_internal_shadow_off = NULL;

bool
asan_internal_range_poisoned(uptr base, size_t size, uptr *first_invalid, bool all) {
    /* Assume that only shadow-aligned ranges make sense. */
    base &= ~(uptr)SHADOW_MASK;
    size += base & SHADOW_MASK;

    uint8_t *shadow = SHADOW_FOR_ADDRESS(base);
    size_t limit = (size + SHADOW_MASK) / SHADOW_ALIGN;

    bool failed_once = false;

    for (size_t i = 0; i < limit; i++, size -= SHADOW_ALIGN) {
        /* FIXME: assert(size > 0); */

        if (!SHADOW_ADDRESS_VALID(shadow + i)) {
            /* We are not aborting due to userspace memory! */
            // platform_asan_fatal("check out of shadow range", base, size, 0);
            if (!failed_once) {
                // ASAN_LOG("Ignoring addr [%p:%p), out of shadow!", (void *)base, (void *)(base+size));
                failed_once = true;
            }
            continue;
        }

        uint8_t s = shadow[i];

        /* All 8 bytes in qword are unpoisoned (i.e. addressable). The shadow value is 0.
         * All 8 bytes in qword are poisoned (i.e. not addressable). The shadow value is negative.
         * First k bytes are unpoisoned, the rest 8-k are poisoned. The shadow value is k. */

        /* All is used to check if the entire range is poisoned. */
        if ((!all && s != 0 && (size > SHADOW_ALIGN || s < size || s > SHADOW_MASK)) ||
            (all && s <= SHADOW_ALIGN && s < size)) {
            /* The exact first byte that failed */
            if (first_invalid)
                *first_invalid = base + i * SHADOW_ALIGN;
            return !all;
        }
    }

    return all;
}

void
asan_internal_fill_range(uptr base, size_t size, uint8_t value) {
    /* By design we cannot poison last X bytes. */
    size_t preceding = SHADOW_ALIGN - (base & SHADOW_MASK);
    if (preceding != SHADOW_ALIGN) {
        if (size > preceding) {
            base += preceding;
            size -= preceding;
        } else {
            /* Not much we can do for too small chunks. */
            return;
        }
    }

    bool failed_once = false;

    uint8_t *shadow = SHADOW_FOR_ADDRESS(base);
    size_t limit = (size + SHADOW_MASK) / SHADOW_ALIGN;

    for (size_t i = 0; i < limit; i++, size -= SHADOW_ALIGN) {
        if (!SHADOW_ADDRESS_VALID(shadow + i)) {
            /* We are not aborting due to kernelspace memory! */
            // platform_asan_fatal("poison out of shadow range", base, size, 0);
            if (!failed_once) {
                ASAN_LOG("Ignoring addr [%p:%p), out of shadow!", (void *)base, (void *)(base + size));
                failed_once = true;
            }
            continue;
        }

        if (size >= SHADOW_ALIGN || value == 0)
            shadow[i] = value;
        else
            shadow[i] = size;
    }
}

void
asan_internal_check_range(const void *x, size_t size, unsigned access_type) {
    if (asan_internal_initialised) {
        uptr invalid;
        if (asan_internal_range_poisoned((uptr)x, size, &invalid, false))
            asan_internal_invalid_access(invalid, size, access_type);
    }
}

void
asan_internal_alloc_init(size_t *size, size_t *align) {
    /* Trap on null size allocations right away */
    if (*size == 0)
        asan_internal_unsupported("0 byte allocation");

    /* If our size is not a multiple of shadow, increase it. */
    *size = (*size + SHADOW_MASK) & ~(size_t)SHADOW_MASK;

    /* All the allocations are required to be at least shadow-aligned. */
    if (*align < SHADOW_ALIGN)
        *align = SHADOW_ALIGN;
}

void
asan_internal_alloc_poison(uptr p, size_t osize, size_t nsize, size_t align) {
    /* FIXME: No free is available, so we don't care about tracking our allocations.
     * Furthermore, we ignore our left padding being possibly bigger than SHADOW_ALIGN.
     * However, when free exists either free should know both size and alignment (to be able
     * to calculate the substraction offset) or the alignment must never exceed SHADOW_ALIGN. */

    uptr invalid;
    if (!asan_internal_range_poisoned(p, nsize + 2 * align, &invalid, true))
        platform_asan_fatal("alloced over", invalid, nsize + 2 * align, 0);

    asan_internal_fill_range(p, align, ASAN_HEAP_RZ);
    asan_internal_fill_range(p + align, osize, 0);
    asan_internal_fill_range(p + align + osize, align + nsize - osize, ASAN_HEAP_RZ);
}

uptr
asan_internal_fakestack_alloc(asan_fakestack_config *configs, uint8_t *entries, size_t entryn, size_t entrysz, size_t realsz) {
    uint32_t thread_id = 0;
    if (!platform_asan_fakestack_enter(&thread_id))
        return 0;

    configs = asan_internal_fakestack_get_t_configs(configs, thread_id, entryn);
    entries = asan_internal_fakestack_get_t_entries(entries, thread_id, entryn, entrysz);

    size_t free = 0;
    while (free < entryn) {
        if (!configs[free].used)
            break;

        free++;
    }

    /* No free entries exist atm */
    if (free == entryn) {
        platform_asan_fakestack_leave();
        return 0;
    }

    uptr entry = (uptr)entries + free * entrysz;

    configs[free].used = true;
    configs[free].realsz = realsz;

    uptr pad = PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE;

    asan_internal_fill_range(entry, pad, ASAN_STACK_LEFT_RZ);
    asan_internal_fill_range(entry + pad, realsz, 0);
    asan_internal_fill_range(entry + pad + realsz,
                             entrysz - pad - realsz, ASAN_STACK_RIGHT_RZ);

    /* See SavedFlagPtr */
    bool **used = (bool **)(entry + entrysz - sizeof(uptr));
    *used = &configs[free].used;

    platform_asan_fakestack_leave();

    return entry + pad;
}

void
asan_internal_fakestack_free(size_t entrysz, uptr dst, size_t realsz) {
    /* This one is normally inlined. */

    bool **used = (bool **)(dst + entrysz - sizeof(uptr) - PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE);
    **used = false;

    asan_internal_fill_range(dst, realsz, ASAN_STACK_FREED);
}

void
asan_internal_fakestack_unpoison() {
    /* TODO: this should unpoison all the stack entries per thread,
     * not too important since it is only called for noreturn functions. */

    uintptr_t rsp = read_rsp();
#ifdef JOS_USER
    uintptr_t stack_begin = rsp <= USER_STACK_TOP ? USER_STACK_TOP : USER_EXCEPTION_STACK_TOP;
#else
    if (rsp > KERN_STACK_TOP) return;
    uintptr_t stack_begin = rsp <= KERN_PF_STACK_TOP ? KERN_PF_STACK_TOP : KERN_STACK_TOP;
#endif

    /* Unpoison whole stack */
    asan_internal_fill_range(rsp - PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
                             stack_begin - rsp + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE, 0);
}
