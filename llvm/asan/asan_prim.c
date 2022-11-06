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

#define ASM 1

/* Not sanitised functions needed for asan itself */

#ifdef ASM

void *
__nosan_memset(void *v, int c, size_t n) {
    uint8_t *ptr = v;
    intptr_t ni = n;

    if (__builtin_expect((ni -= ((8 - ((uintptr_t)v & 7))) & 7) < 0, 0)) {
        while (n-- > 0) *ptr++ = c;
        return v;
    }

    uint64_t k = 0x101010101010101ULL * (c & 0xFFU);

    if (__builtin_expect((uintptr_t)ptr & 7, 0)) {
        if ((uintptr_t)ptr & 1) *ptr = k, ptr += 1;
        if ((uintptr_t)ptr & 2) *(uint16_t *)ptr = k, ptr += 2;
        if ((uintptr_t)ptr & 4) *(uint32_t *)ptr = k, ptr += 4;
    }

    if (__builtin_expect(ni / 8, 1)) {
        asm volatile("cld; rep stosq\n" ::"D"(ptr), "a"(k), "c"(ni / 8)
                     : "cc", "memory");
        ni &= 7;
    }

    if (__builtin_expect(ni, 0)) {
        if (ni & 4) *(uint32_t *)ptr = k, ptr += 4;
        if (ni & 2) *(uint16_t *)ptr = k, ptr += 2;
        if (ni & 1) *ptr = k;
    }

    return v;
}

void *
__nosan_memcpy(void *dst, const void *src, size_t n) {
    const char *s = src;
    char *d = dst;

    if (!(((intptr_t)s & 7) | ((intptr_t)d & 7) | (n & 7))) {
        asm volatile("cld; rep movsq\n" ::"D"(d), "S"(s), "c"(n / 8)
                     : "cc", "memory");
    } else {
        asm volatile("cld; rep movsb\n" ::"D"(d), "S"(s), "c"(n)
                     : "cc", "memory");
    }
    return dst;
}

#else
void *
__nosan_memset(void *src, int c, size_t sz) {
    /* We absolutely must implement this for ASAN functioning. */
    volatile char *vptr = (volatile char *)(src);
    while (sz--) {
        *vptr++ = c;
    }
    return src;
}

void *
__nosan_memcpy(void *dst, const void *src, size_t sz) {
    const char *s = src;
    char *d = dst;

    if (s < (char *)dst && s + n > (char *)dst) {
        s += n, d += n;
        while (n-- > 0) *--d = *--s;
    } else {
        while (n-- > 0) *d++ = *s++;
    }

    return dst;
}
#endif

void *
__nosan_memmove(void *dst, const void *src, size_t sz) {
    return __nosan_memcpy(dst, src, sz);
}
