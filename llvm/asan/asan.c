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

int __asan_option_detect_stack_use_after_return = PLATFORM_ASAN_USE_AFTER_RETURN;

uptr
__asan_load_cxx_array_cookie(uptr *UNUSED p) {
    /* Do we even support C++? */
    asan_internal_unsupported(__func__);
}

void
__asan_poison_cxx_array_cookie(uptr UNUSED p) {
    /* Do we even support C++? */
    asan_internal_unsupported(__func__);
}

void NOINLINE
__asan_handle_no_return(void) {
    asan_internal_fakestack_unpoison();
}

#define ACCESS_CHECK_DECLARE(type, sz, access_type)                     \
    void __asan_##type##sz(uptr addr) {                                 \
        asan_internal_check_range((const void *)addr, sz, access_type); \
    }

ACCESS_CHECK_DECLARE(load, 1, TYPE_LOAD);
ACCESS_CHECK_DECLARE(load, 2, TYPE_LOAD);
ACCESS_CHECK_DECLARE(load, 4, TYPE_LOAD);
ACCESS_CHECK_DECLARE(load, 8, TYPE_LOAD);
ACCESS_CHECK_DECLARE(load, 16, TYPE_LOAD);
ACCESS_CHECK_DECLARE(store, 1, TYPE_STORE);
ACCESS_CHECK_DECLARE(store, 2, TYPE_STORE);
ACCESS_CHECK_DECLARE(store, 4, TYPE_STORE);
ACCESS_CHECK_DECLARE(store, 8, TYPE_STORE);
ACCESS_CHECK_DECLARE(store, 16, TYPE_STORE);

#define REPORT_DECLARE(n)                               \
    void __asan_report_load##n(uptr p) {                \
        asan_internal_invalid_access(p, n, TYPE_LOAD);  \
    }                                                   \
    void __asan_report_store##n(uptr p) {               \
        asan_internal_invalid_access(p, n, TYPE_STORE); \
    }

REPORT_DECLARE(1)
REPORT_DECLARE(2)
REPORT_DECLARE(4)
REPORT_DECLARE(8)
REPORT_DECLARE(16)

void
__asan_report_load_n(uptr p, unsigned long sz) {
    asan_internal_invalid_access(p, sz, TYPE_LOAD);
}

void
__asan_report_store_n(uptr p, unsigned long sz) {
    asan_internal_invalid_access(p, sz, TYPE_STORE);
}

void
__asan_loadN(uptr addr, size_t sz) {
    asan_internal_check_range((const void *)addr, sz, TYPE_LOAD);
}

void
__asan_storeN(uptr addr, size_t sz) {
    asan_internal_check_range((const void *)addr, sz, TYPE_STORE);
}

#define SET_SHADOW_DECLARE(val)                          \
    void __asan_set_shadow_##val(uptr addr, size_t sz) { \
        __nosan_memset((void *)addr, 0x##val, sz);       \
    }

SET_SHADOW_DECLARE(00)
SET_SHADOW_DECLARE(f1)
SET_SHADOW_DECLARE(f2)
SET_SHADOW_DECLARE(f3)
SET_SHADOW_DECLARE(f5)
SET_SHADOW_DECLARE(f8)

#define FAKESTACK_DECLARE(szclass)                                                                                     \
    static uint8_t asan_fakestack_buffer_##szclass[PLATFORM_ASAN_FASESTACK_THREAD_MAX *                                \
                                                           PLATFORM_ASAN_FAKESTACK_CLASS_##szclass##_N *               \
                                                           FAKESTACK_CLASS_##szclass##_S +                             \
                                                   1]                                                                  \
            __attribute__((aligned(SHADOW_ALIGN)));                                                                    \
    static asan_fakestack_config asan_fakestack_config_##szclass[PLATFORM_ASAN_FASESTACK_THREAD_MAX *                  \
                                                                         PLATFORM_ASAN_FAKESTACK_CLASS_##szclass##_N + \
                                                                 1];                                                   \
    uptr __asan_stack_malloc_##szclass(size_t sz) {                                                                    \
        return asan_internal_fakestack_alloc(                                                                          \
                &asan_fakestack_config_##szclass[0],                                                                   \
                &asan_fakestack_buffer_##szclass[0],                                                                   \
                PLATFORM_ASAN_FAKESTACK_CLASS_##szclass##_N,                                                           \
                FAKESTACK_CLASS_##szclass##_S, sz);                                                                    \
    }                                                                                                                  \
    void __asan_stack_free_##szclass(uptr dst, size_t sz) {                                                            \
        asan_internal_fakestack_free(                                                                                  \
                FAKESTACK_CLASS_##szclass##_S, dst, sz);                                                               \
    }

FAKESTACK_DECLARE(0)
FAKESTACK_DECLARE(1)
FAKESTACK_DECLARE(2)
FAKESTACK_DECLARE(3)
FAKESTACK_DECLARE(4)
FAKESTACK_DECLARE(5)
FAKESTACK_DECLARE(6)
FAKESTACK_DECLARE(7)
FAKESTACK_DECLARE(8)
FAKESTACK_DECLARE(9)
FAKESTACK_DECLARE(10)

/*
 * Initialisation routines
 */

void
__asan_init(void) {
    if (!asan_internal_initialised) {
        platform_asan_init();
        asan_internal_initialised = true;
    }
}

void
__asan_register_globals(uptr g, uptr n) {
    if (PLATFORM_ASAN_USE_REPORT_GLOBALS) {
        __asan_global *globals = (__asan_global *)g;
        for (uptr i = 0; i < n; i++) {
            asan_internal_fill_range(globals[i].beg + globals[i].size,
                                     globals[i].size_with_redzone - globals[i].size, ASAN_GLOBAL_RZ);
        }
    }
}

void
__asan_unregister_globals(uptr g, uptr n) {
    if (PLATFORM_ASAN_USE_REPORT_GLOBALS) {
        __asan_global *globals = (__asan_global *)g;
        for (uptr i = 0; i < n; i++) {
            asan_internal_fill_range(globals[i].beg + globals[i].size,
                                     globals[i].size_with_redzone - globals[i].size, 0);
        }
    }
}

/* These functions can be called on some platforms to find globals in the same
 * loaded image as `flag' and apply __asan_(un)register_globals to them,
 * filtering out redundant calls. */
void
__asan_register_image_globals(uptr UNUSED ptr) {
}

void
__asan_unregister_image_globals(uptr UNUSED ptr) {
}

void
__asan_init_v5(void) {
}

void
__asan_before_dynamic_init(uptr UNUSED arg) {
}

void
__asan_after_dynamic_init(void) {
}

void
__asan_version_mismatch_check_v8(void) {
}

/*
 * TODO: implement these (why do we need them???)
 */

void
__asan_alloca_poison(uptr UNUSED addr, uptr UNUSED size) {
}

void
__asan_allocas_unpoison(uptr UNUSED top, uptr UNUSED bottom) {
}

void
__sanitizer_ptr_sub(uptr UNUSED a, uptr UNUSED b) {
}

void
__sanitizer_ptr_cmp(uptr UNUSED a, uptr UNUSED b) {
}

void
__asan_poison_stack_memory(uptr UNUSED addr, size_t UNUSED size) {
}

void
__asan_unpoison_stack_memory(uptr UNUSED addr, size_t UNUSED size) {
}

void
__sanitizer_annotate_contiguous_container(
        const void *UNUSED beg,
        const void *UNUSED end,
        const void *UNUSED old_mid,
        const void *UNUSED new_mid) {
}
