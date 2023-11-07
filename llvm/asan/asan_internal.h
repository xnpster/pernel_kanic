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

#ifndef __ASAN_INTERNAL_H__
#define __ASAN_INTERNAL_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "asan_config.h"

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

#ifndef NOINLINE
#define NOINLINE __attribute__((noinline))
#endif

#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif

#ifndef SYMBOL_ALIAS
#define SYMBOL_ALIAS(x) __attribute__((weak, alias(x)))
#endif

#define SHADOW_SCALE 3
#define SHADOW_ALIGN (1 << SHADOW_SCALE)
#define SHADOW_MASK  (SHADOW_ALIGN - 1)

#define ADDRESS_FOR_SHADOW(x)   (((x) - (uptr)asan_internal_shadow_off) << SHADOW_SCALE)
#define SHADOW_FOR_ADDRESS(x)   (uint8_t *)(((x) >> SHADOW_SCALE) + (uptr)asan_internal_shadow_off)
#define SHADOW_ADDRESS_VALID(x) (((uint8_t *)(x) >= asan_internal_shadow_start && (uint8_t *)(x) < asan_internal_shadow_end))

#ifdef PLATFORM_ASAN_HAVE_PRINTF
int PLATFORM_ASAN_PRINTF(const char *format, ...);
#define ASAN_LOG(fmt, ...) PLATFORM_ASAN_PRINTF("[ASAN] " fmt "\n", ##__VA_ARGS__)
#else
#define ASAN_LOG(fmt, ...) \
    do {                   \
    } while (0)
#endif

#ifdef PLATFORM_ASAN_HAVE_ABORT
/* This should work for most of C prototypes. */
void NORETURN PLATFORM_ASAN_ABORT();
#define ASAN_ABORT() PLATFORM_ASAN_ABORT()
#else
#define ASAN_ABORT() \
    do {             \
    } while (1)
#endif

#ifdef PLATFORM_ASAN_HAVE_DEBUG_BREAK
/* This should work for most of C prototypes. */
void PLATFORM_ASAN_DEBUG_BREAK();
#define ASAN_DEBUG_BREAK() PLATFORM_ASAN_DEBUG_BREAK()
#else
#define ASAN_DEBUG_BREAK() \
    do {                   \
    } while (0)
#endif

#ifdef PLATFORM_ASAN_HAVE_STACK_DUMP
#define DUMP_STACK_AT_LEVEL(p, x)                                                          \
    do {                                                                                   \
        if (((p) != PLATFORM_ASAN_STACK_LAST && __builtin_frame_address(x)) || (x) == 0) { \
            (p) = (uintptr_t)__builtin_return_address(x);                                  \
            if ((p) != PLATFORM_ASAN_STACK_LAST) {                                         \
                (p) = (uintptr_t)__builtin_extract_return_addr((void *)(p));               \
                PLATFORM_ASAN_PRINTF("%02d: 0x%08lx\n", (x), (long)(p));                   \
            }                                                                              \
        } else                                                                             \
            (p) = 0;                                                                       \
    } while (0)
#else
#define DUMP_STACK_AT_LEVEL(p, x) \
    do {                          \
    } while (0)
#endif

#define BIT(x) (1U << (x))

enum asan_access_type {
    /* exactly one of these bits must be set */
    TYPE_LOAD = BIT(0),
    TYPE_STORE = BIT(1),
    TYPE_KFREE = BIT(2),
    TYPE_ZFREE = BIT(3),
    TYPE_FSFREE = BIT(4),    /* fakestack free */
    TYPE_MEMLD = BIT(5),     /* memory intrinsic - load */
    TYPE_MEMSTR = BIT(6),    /* memory intrinsic - store */
    TYPE_STRINGLD = BIT(7),  /* string intrinsic - load */
    TYPE_STRINGSTR = BIT(8), /* string intrinsic - store */
    TYPE_TEST = BIT(15),

    /* masks */
    TYPE_LDSTR = TYPE_LOAD | TYPE_STORE, /* regular loads and stores */
    TYPE_FREE = TYPE_KFREE | TYPE_ZFREE | TYPE_FSFREE,
    TYPE_MEM = TYPE_MEMLD | TYPE_MEMSTR,
    TYPE_STRING = TYPE_STRINGLD | TYPE_STRINGSTR,
    TYPE_LOAD_ALL = TYPE_LOAD | TYPE_MEMLD | TYPE_STRINGLD,
    TYPE_STORE_ALL = TYPE_STORE | TYPE_MEMSTR | TYPE_STRINGSTR,
    TYPE_ALL = ~0U
};

enum asan_fakestack_class_size {
    FAKESTACK_CLASS_0_S = (1 << 6) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
    FAKESTACK_CLASS_1_S = (1 << 7) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
    FAKESTACK_CLASS_2_S = (1 << 8) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
    FAKESTACK_CLASS_3_S = (1 << 9) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
    FAKESTACK_CLASS_4_S = (1 << 10) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
    FAKESTACK_CLASS_5_S = (1 << 11) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
    FAKESTACK_CLASS_6_S = (1 << 12) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
    FAKESTACK_CLASS_7_S = (1 << 13) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
    FAKESTACK_CLASS_8_S = (1 << 14) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
    FAKESTACK_CLASS_9_S = (1 << 15) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE,
    FAKESTACK_CLASS_10_S = (1 << 16) + PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE
};

typedef struct {
    bool used;
    uint32_t realsz;
} asan_fakestack_config;

void platform_asan_init(void);
void NORETURN platform_asan_fatal(const char *msg, uptr p, size_t width, unsigned access_type);
bool platform_asan_fakestack_enter(uint32_t *thread_id);
void platform_asan_fakestack_leave(void);

extern bool asan_internal_initialised;
extern uint8_t *asan_internal_shadow_start;
extern uint8_t *asan_internal_shadow_end;
extern uint8_t *asan_internal_shadow_off;

static inline void NORETURN
asan_internal_unsupported(const char *msg) {
    platform_asan_fatal(msg, 0, 0, 0);
}

static inline void NORETURN
asan_internal_invalid_access(uptr p, size_t width, unsigned access_type) {
    platform_asan_fatal("invalid access", p, width, access_type);
}

void asan_internal_alloc_init(size_t *size, size_t *align);
void asan_internal_alloc_poison(uptr p, size_t osize, size_t nsize, size_t align);
void asan_internal_check_range(const void *x, size_t sz, unsigned access_type);
bool asan_internal_range_poisoned(uptr base, size_t size, uptr *first_invalid, bool all);
void asan_internal_fill_range(uptr base, size_t size, uint8_t value);

static inline asan_fakestack_config *
asan_internal_fakestack_get_t_configs(asan_fakestack_config *config, uint32_t thread_id, size_t entryn) {
    return &config[thread_id * entryn];
}

static inline uint8_t *
asan_internal_fakestack_get_t_entries(uint8_t *entries, uint32_t thread_id, size_t entryn, size_t entrysz) {
    return entries + thread_id * entrysz * entryn;
}

uptr asan_internal_fakestack_alloc(asan_fakestack_config *configs, uint8_t *entries, size_t entryn, size_t entrysz, size_t realsz);
void asan_internal_fakestack_free(size_t entrysz, uptr dst, size_t realsz);
void asan_internal_fakestack_unpoison();

#endif /* __ASAN_INTERNAL_H__ */
