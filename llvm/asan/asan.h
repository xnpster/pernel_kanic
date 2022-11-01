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

#ifndef __ASAN_H__
#define __ASAN_H__

#include <stddef.h>
#include <stdint.h>

typedef uintptr_t uptr;

/* shadow map byte values */
#define ASAN_VALID          0x00
#define ASAN_PARTIAL1       0x01
#define ASAN_PARTIAL2       0x02
#define ASAN_PARTIAL3       0x03
#define ASAN_PARTIAL4       0x04
#define ASAN_PARTIAL5       0x05
#define ASAN_PARTIAL6       0x06
#define ASAN_PARTIAL7       0x07
#define ASAN_ARRAY_COOKIE   0xac
#define ASAN_STACK_RZ       0xf0
#define ASAN_STACK_LEFT_RZ  0xf1
#define ASAN_STACK_MID_RZ   0xf2
#define ASAN_STACK_RIGHT_RZ 0xf3
#define ASAN_STACK_FREED    0xf5
#define ASAN_GLOBAL_RZ      0xf9
#define ASAN_HEAP_RZ        0xe9
#define ASAN_HEAP_LEFT_RZ   0xfa
#define ASAN_HEAP_RIGHT_RZ  0xfb
#define ASAN_HEAP_FREED     0xfd

/* This structure is used to describe the source location of a place where
 * global was defined.
 */
typedef struct {
    const char *filename;
    int line_no;
    int column_no;
} __asan_global_source_location;

/* This structure describes an instrumented global variable.
 */
typedef struct {
    uptr beg;                                /* The address of the global. */
    uptr size;                               /* The original size of the global. */
    uptr size_with_redzone;                  /* The size with the redzone. */
    const char *name;                        /* Name as a C string. */
    const char *module_name;                 /* Module name as a C string. This pointer is a *
                                              * unique identifier of a module. */
    uptr has_dynamic_init;                   /* Non-zero if the global has dynamic initializer. */
    __asan_global_source_location *location; /* Source location of a global, *
                                              * or NULL if it is unknown. */
    uptr odr_indicator;                      /* The address of the ODR indicator symbol. */
} __asan_global;

/* ASAN callbacks - inserted by the compiler
 */

extern int __asan_option_detect_stack_use_after_return;

void __asan_report_load1(uptr p);
void __asan_report_load2(uptr p);
void __asan_report_load4(uptr p);
void __asan_report_load8(uptr p);
void __asan_report_load16(uptr p);
void __asan_report_store1(uptr p);
void __asan_report_store2(uptr p);
void __asan_report_store4(uptr p);
void __asan_report_store8(uptr p);
void __asan_report_store16(uptr p);
void __asan_report_load_n(uptr p, unsigned long size);
void __asan_report_store_n(uptr p, unsigned long size);
void __asan_handle_no_return(void);
uptr __asan_stack_malloc_0(size_t s);
uptr __asan_stack_malloc_1(size_t s);
uptr __asan_stack_malloc_2(size_t s);
uptr __asan_stack_malloc_3(size_t s);
uptr __asan_stack_malloc_4(size_t s);
uptr __asan_stack_malloc_5(size_t s);
uptr __asan_stack_malloc_6(size_t s);
uptr __asan_stack_malloc_7(size_t s);
uptr __asan_stack_malloc_8(size_t s);
uptr __asan_stack_malloc_9(size_t s);
uptr __asan_stack_malloc_10(size_t s);
void __asan_stack_free_0(uptr p, size_t s);
void __asan_stack_free_1(uptr p, size_t s);
void __asan_stack_free_2(uptr p, size_t s);
void __asan_stack_free_3(uptr p, size_t s);
void __asan_stack_free_4(uptr p, size_t s);
void __asan_stack_free_5(uptr p, size_t s);
void __asan_stack_free_6(uptr p, size_t s);
void __asan_stack_free_7(uptr p, size_t s);
void __asan_stack_free_8(uptr p, size_t s);
void __asan_stack_free_9(uptr p, size_t s);
void __asan_stack_free_10(uptr p, size_t s);
void __asan_poison_cxx_array_cookie(uptr p);
uptr __asan_load_cxx_array_cookie(uptr *p);
void __asan_poison_stack_memory(uptr addr, size_t size);
void __asan_unpoison_stack_memory(uptr addr, size_t size);
void __asan_alloca_poison(uptr addr, uptr size);
void __asan_allocas_unpoison(uptr top, uptr bottom);
void __asan_load1(uptr p);
void __asan_load2(uptr p);
void __asan_load4(uptr p);
void __asan_load8(uptr p);
void __asan_load16(uptr p);
void __asan_loadN(uptr p, size_t s);
void __asan_store1(uptr p);
void __asan_store2(uptr p);
void __asan_store4(uptr p);
void __asan_store8(uptr p);
void __asan_store16(uptr p);
void __asan_storeN(uptr p, size_t s);
void __sanitizer_ptr_sub(uptr a, uptr b);
void __sanitizer_ptr_cmp(uptr a, uptr b);
void __sanitizer_annotate_contiguous_container(const void *beg, const void *end, const void *old_mid, const void *new_mid);

void __asan_set_shadow_00(uptr p, size_t s);
void __asan_set_shadow_f1(uptr p, size_t s);
void __asan_set_shadow_f2(uptr p, size_t s);
void __asan_set_shadow_f3(uptr p, size_t s);
void __asan_set_shadow_f5(uptr p, size_t s);
void __asan_set_shadow_f8(uptr p, size_t s);

void __asan_init_v5(void);
void __asan_before_dynamic_init(uptr p);
void __asan_after_dynamic_init(void);
void __asan_unregister_globals(uptr globals, uptr n);
void __asan_register_globals(uptr globals, uptr n);
void __asan_init(void);
void __asan_unregister_image_globals(uptr p);
void __asan_register_image_globals(uptr p);

void __asan_version_mismatch_check_v8(void);

#endif /* __ASAN_H__ */
