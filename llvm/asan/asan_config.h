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

#ifndef __ASAN_CONFIG_H__
#define __ASAN_CONFIG_H__

/* Supported C-like functions used in ASAN. */
#define PLATFORM_ASAN_HAVE_PRINTF
#define PLATFORM_ASAN_HAVE_ABORT
/*#define PLATFORM_ASAN_HAVE_DEBUG_BREAK */

/* Actual symbol names for supported C-like functions. */
#define PLATFORM_ASAN_PRINTF      cprintf
#define PLATFORM_ASAN_ABORT       platform_abort
#define PLATFORM_ASAN_DEBUG_BREAK debug_break

/* Enable or disable use after return check: */
/* https://github.com/google/sanitizers/wiki/AddressSanitizerUseAfterReturn */
#define PLATFORM_ASAN_USE_AFTER_RETURN 0

#define PLATFORM_ASAN_USE_REPORT_GLOBALS 1

/* Max number of threads for fakestack to track */
/* TODO: this should be properly determined at compile time. */
#define PLATFORM_ASAN_FASESTACK_THREAD_MAX 0

/* Max fakestack entries of each class */
#define PLATFORM_ASAN_FAKESTACK_CLASS_0_N  0
#define PLATFORM_ASAN_FAKESTACK_CLASS_1_N  0
#define PLATFORM_ASAN_FAKESTACK_CLASS_2_N  0
#define PLATFORM_ASAN_FAKESTACK_CLASS_3_N  0
#define PLATFORM_ASAN_FAKESTACK_CLASS_4_N  0
#define PLATFORM_ASAN_FAKESTACK_CLASS_5_N  0
#define PLATFORM_ASAN_FAKESTACK_CLASS_6_N  0
#define PLATFORM_ASAN_FAKESTACK_CLASS_7_N  0
#define PLATFORM_ASAN_FAKESTACK_CLASS_8_N  0
#define PLATFORM_ASAN_FAKESTACK_CLASS_9_N  0
#define PLATFORM_ASAN_FAKESTACK_CLASS_10_N 0

/* Redzone size to the left of the allocated fake stack */
#define PLATFORM_ASAN_FAKESTACK_LEFT_RED_SIZE 16

/* Are we guaranteed to know the value our stack is initialised with? */
#define PLATFORM_ASAN_HAVE_STACK_DUMP

/* Last value to guarantee that no more stack frames are available */
#define PLATFORM_ASAN_STACK_LAST 0

/* These functions allow you to wrap existing intrinsics and compile
 * them without asan enabled (which is generally more reliable and
 * better for performance). */

#define PLATFORM_ASAN_HAVE_MEMCPY
#define PLATFORM_ASAN_HAVE_MEMSET
#define PLATFORM_ASAN_HAVE_MEMMOVE
// #define PLATFORM_ASAN_HAVE_BCOPY
// #define PLATFORM_ASAN_HAVE_BZERO
// #define PLATFORM_ASAN_HAVE_BCMP
#define PLATFORM_ASAN_HAVE_MEMCMP
#define PLATFORM_ASAN_HAVE_STRCAT
#define PLATFORM_ASAN_HAVE_STRCPY
// #define PLATFORM_ASAN_HAVE_STRLCPY
// #define PLATFORM_ASAN_HAVE_STRLCAT
// #define PLATFORM_ASAN_HAVE_STRNCAT
#define PLATFORM_ASAN_HAVE_STRNCPY
#define PLATFORM_ASAN_HAVE_STRNLEN
#define PLATFORM_ASAN_HAVE_STRLEN

#endif /* __ASAN_CONFIG_H__ */
