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

/* Aliases for wrapped methods */

void *__wrap_memcpy(void *dst, const void *src, size_t sz) SYMBOL_ALIAS("__asan_memcpy");
void *__wrap_memset(void *src, int c, size_t sz) SYMBOL_ALIAS("__asan_memset");
void *__wrap_memmove(void *src, const void *dst, size_t sz) SYMBOL_ALIAS("__asan_memmove");
void __wrap_bcopy(const void *src, void *dst, size_t sz) SYMBOL_ALIAS("__asan_bcopy");
void __wrap_bzero(void *dst, size_t sz) SYMBOL_ALIAS("__asan_bzero");
int __wrap_bcmp(const void *a, const void *b, size_t sz) SYMBOL_ALIAS("__asan_bcmp");
int __wrap_memcmp(const void *a, const void *b, size_t sz) SYMBOL_ALIAS("__asan_memcmp");

char *__wrap_strcat(char *dst, const char *src) SYMBOL_ALIAS("__asan_strcat");
char *__wrap_strcpy(char *dst, const char *src) SYMBOL_ALIAS("__asan_strcpy");
size_t __wrap_strlcpy(char *dst, const char *src, size_t sz) SYMBOL_ALIAS("__asan_strlcpy");
char *__wrap_strncpy(char *dst, const char *src, size_t sz) SYMBOL_ALIAS("__asan_strncpy");
size_t __wrap_strlcat(char *dst, const char *src, size_t sz) SYMBOL_ALIAS("__asan_strlcat");
char *__wrap_strncat(char *dst, const char *src, size_t sz) SYMBOL_ALIAS("__asan_strncat");
size_t __wrap_strnlen(const char *src, size_t sz) SYMBOL_ALIAS("__asan_strnlen");
size_t __wrap_strlen(const char *src) SYMBOL_ALIAS("__asan_strlen");

/* Implementations for memory functions */

void
__asan_bcopy(const void *src, void *dst, size_t sz) {
    asan_internal_check_range(src, sz, TYPE_MEMLD);
    asan_internal_check_range(dst, sz, TYPE_MEMSTR);
    __real_bcopy(src, dst, sz);
}

#ifndef PLATFORM_ASAN_HAVE_BCOPY
void
__real_bcopy(const void *UNUSED src, void *UNUSED dst, size_t UNUSED sz) {
    asan_internal_unsupported(__func__);
}
#endif

void *
__asan_memmove(void *src, const void *dst, size_t sz) {
    asan_internal_check_range(src, sz, TYPE_MEMLD);
    asan_internal_check_range(dst, sz, TYPE_MEMSTR);
    return __real_memmove(src, dst, sz);
}

#ifndef PLATFORM_ASAN_HAVE_MEMMOVE
void *
__real_memmove(void *UNUSED src, const void *UNUSED dst, size_t UNUSED sz) {
    asan_internal_unsupported(__func__);
}
#endif

void *
__asan_memcpy(void *dst, const void *src, size_t sz) {
    asan_internal_check_range(src, sz, TYPE_MEMLD);
    asan_internal_check_range(dst, sz, TYPE_MEMSTR);
    return __real_memcpy(dst, src, sz);
}

#ifndef PLATFORM_ASAN_HAVE_MEMCPY
void *
__real_memcpy(void *UNUSED src, const void *UNUSED dst, size_t UNUSED sz) {
    asan_internal_unsupported(__func__);
}
#endif

void *
__asan_memset(void *dst, int c, size_t sz) {
    asan_internal_check_range(dst, sz, TYPE_MEMSTR);
    return __real_memset(dst, c, sz);
}

#ifndef PLATFORM_ASAN_HAVE_MEMSET
void *
__real_memset(void *src, int c, size_t sz) {
    /* We absolutely must implement this for ASAN functioning. */
    return __nosan_memset(src, c, sz);
}
#endif

void
__asan_bzero(void *dst, size_t sz) {
    asan_internal_check_range(dst, sz, TYPE_MEMSTR);
    __real_bzero(dst, sz);
}

#ifndef PLATFORM_ASAN_HAVE_BZERO
void
__real_bzero(void *UNUSED dst, size_t UNUSED sz) {
    asan_internal_unsupported(__func__);
}
#endif

int
__asan_bcmp(const void *a, const void *b, size_t len) {
    asan_internal_check_range(a, len, TYPE_MEMLD);
    asan_internal_check_range(b, len, TYPE_MEMLD);
    return __real_bcmp(a, b, len);
}

#ifndef PLATFORM_ASAN_HAVE_BCMP
int
__real_bcmp(const void *UNUSED a, const void *UNUSED b, size_t UNUSED len) {
    asan_internal_unsupported(__func__);
}
#endif

int
__asan_memcmp(const void *a, const void *b, size_t n) {
    asan_internal_check_range(a, n, TYPE_MEMLD);
    asan_internal_check_range(b, n, TYPE_MEMLD);
    return __real_memcmp(a, b, n);
}

#ifndef PLATFORM_ASAN_HAVE_MEMCMP
int
__real_memcmp(const void *UNUSED a, const void *UNUSED b, size_t UNUSED sz) {
    asan_internal_unsupported(__func__);
}
#endif

/*
 * Implementations for string functions
 */

char *
__asan_strcat(char *dst, const char *src) {
    asan_internal_check_range(dst, __real_strlen(dst) + __real_strlen(src) + 1, TYPE_STRINGSTR);
    return __real_strcat(dst, src);
}

#ifndef PLATFORM_ASAN_HAVE_STRCPY
char *
__real_strcat(char *UNUSED dst, const char *UNUSED src) {
    asan_internal_unsupported(__func__);
}
#endif

char *
__asan_strcpy(char *dst, const char *src) {
    asan_internal_check_range(dst, __real_strlen(src) + 1, TYPE_STRINGSTR);
    return __real_strcpy(dst, src);
}

#ifndef PLATFORM_ASAN_HAVE_STRCPY
char *
__real_strcpy(char *UNUSED dst, const char *UNUSED src) {
    asan_internal_unsupported(__func__);
}
#endif

size_t
__asan_strlcpy(char *dst, const char *src, size_t sz) {
    asan_internal_check_range(dst, sz, TYPE_STRINGSTR);
    return __real_strlcpy(dst, src, sz);
}

#ifndef PLATFORM_ASAN_HAVE_STRLCPY
size_t
__real_strlcpy(char *UNUSED dst, const char *UNUSED src, size_t UNUSED sz) {
    asan_internal_unsupported(__func__);
}
#endif

size_t
__asan_strlcat(char *dst, const char *src, size_t sz) {
    asan_internal_check_range(dst, sz, TYPE_STRINGSTR);
    return __real_strlcat(dst, src, sz);
}

#ifndef PLATFORM_ASAN_HAVE_STRLCAT
size_t
__real_strlcat(char *UNUSED dst, const char *UNUSED src, size_t UNUSED sz) {
    asan_internal_unsupported(__func__);
}
#endif

char *
__asan_strncpy(char *dst, const char *src, size_t sz) {
    asan_internal_check_range(dst, sz, TYPE_STRINGSTR);
    return __real_strncpy(dst, src, sz);
}

#ifndef PLATFORM_ASAN_HAVE_STRNCPY
char *
__real_strncpy(char *UNUSED dst, const char *UNUSED src, size_t UNUSED sz) {
    asan_internal_unsupported(__func__);
}
#endif

char *
__asan_strncat(char *dst, const char *src, size_t sz) {
    asan_internal_check_range(dst, __real_strlen(dst) + sz + 1, TYPE_STRINGSTR);
    /* error: undefined symbol: strncat */
    // return __real_strncat(dst, src, sz);
    asan_internal_unsupported(__func__);
}

#ifndef PLATFORM_ASAN_HAVE_STRNCAT
char *
__real_strncat(char *UNUSED dst, const char *UNUSED src, size_t UNUSED sz) {
    asan_internal_unsupported(__func__);
}
#endif

size_t
__asan_strnlen(const char *src, size_t sz) {
    asan_internal_check_range(src, sz, TYPE_STRINGLD);
    return __real_strnlen(src, sz);
}

#ifndef PLATFORM_ASAN_HAVE_STRNLEN
size_t
__real_strnlen(const char *UNUSED src, size_t UNUSED sz) {
    asan_internal_unsupported(__func__);
}
#endif

size_t
__asan_strlen(const char *src) {
    size_t sz = __real_strlen(src);
    asan_internal_check_range(src, sz + 1, TYPE_STRINGLD);
    return sz;
}

#ifndef PLATFORM_ASAN_HAVE_STRLEN
size_t
__real_strlen(const char *UNUSED src) {
    asan_internal_unsupported(__func__);
}
#endif
