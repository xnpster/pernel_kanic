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

#ifndef __ASAN_MEMINTRINSICS_H__
#define __ASAN_MEMINTRINSICS_H__

#include <stddef.h>

/* TODO: It might be a good idea to add more. */

/*
 * Sanitized intrinsics
 */
void *__asan_memcpy(void *src, const void *dst, size_t sz);
void *__asan_memset(void *src, int c, size_t sz);
void *__asan_memmove(void *src, const void *dst, size_t sz);
void __asan_bcopy(const void *src, void *dst, size_t sz);
void __asan_bzero(void *dst, size_t sz);
int __asan_bcmp(const void *a, const void *b, size_t sz);
int __asan_memcmp(const void *a, const void *b, size_t sz);

char *__asan_strcat(char *dst, const char *src);
char *__asan_strcpy(char *dst, const char *src);
size_t __asan_strlcpy(char *dst, const char *src, size_t sz);
char *__asan_strncpy(char *dst, const char *src, size_t sz);
size_t __asan_strlcat(char *dst, const char *src, size_t sz);
char *__asan_strncat(char *dst, const char *src, size_t sz);
size_t __asan_strnlen(const char *src, size_t sz);
size_t __asan_strlen(const char *src);

/*
 * Aliases for original methods
 */
void *__real_memcpy(void *src, const void *dst, size_t sz);
void *__real_memset(void *src, int c, size_t sz);
void *__real_memmove(void *src, const void *dst, size_t sz);
void __real_bcopy(const void *src, void *dst, size_t sz);
void __real_bzero(void *dst, size_t sz);
int __real_bcmp(const void *a, const void *b, size_t sz);
int __real_memcmp(const void *a, const void *b, size_t sz);

char *__real_strcat(char *dst, const char *src);
char *__real_strcpy(char *dst, const char *src);
size_t __real_strlcpy(char *dst, const char *src, size_t sz);
char *__real_strncpy(char *dst, const char *src, size_t sz);
size_t __real_strlcat(char *dst, const char *src, size_t sz);
char *__real_strncat(char *dst, const char *src, size_t sz);
size_t __real_strnlen(const char *src, size_t sz);
size_t __real_strlen(const char *src);

/*
 *  Not sanitised functions needed for asan itself
 */
void *__nosan_memset(void *src, int c, size_t sz);
void *__nosan_memcpy(void *dst, const void *src, size_t sz);
void *__nosan_memmove(void *dst, const void *src, size_t sz);

#endif /* __ASAN_MEMINTRINSICS_H__ */
