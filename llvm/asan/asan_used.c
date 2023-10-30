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

/* FIXME: This is a very ugly hack currently needed to workaround lld not wrapping
 * methods that are only referenced in static libraries. Most likely it is a bug
 * but it is not unlikely that it cannot be easily fixed.
 * If this file is not passed as a separate "asan_used.o" file to the linker
 * of the final executable most wrapped functions will be defined as external
 * and will require dynamic loader regardless of -static or similar arguments. */

/* NOTE: to compile this file you obviously need -ffreestanding. */

#include "asan_config.h"

void __attribute__((used)) __attribute__((optnone))
asan_force_use_wrapped_lib_symbols() {

#define USE(x)    \
    do {          \
        void x(); \
        x();      \
    } while (0)

#ifdef PLATFORM_ASAN_HAVE_MEMCPY
    USE(memcpy);
#endif
#ifdef PLATFORM_ASAN_HAVE_MEMSET
    USE(memset);
#endif
#ifdef PLATFORM_ASAN_HAVE_MEMMOVE
    USE(memmove);
#endif
#ifdef PLATFORM_ASAN_HAVE_BCOPY
    USE(bcopy);
#endif
#ifdef PLATFORM_ASAN_HAVE_BZERO
    USE(bzero);
#endif
#ifdef PLATFORM_ASAN_HAVE_BCMP
    USE(bcmp);
#endif
#ifdef PLATFORM_ASAN_HAVE_MEMCMP
    USE(memcmp);
#endif
#ifdef PLATFORM_ASAN_HAVE_STRCAT
    USE(strcat);
#endif
#ifdef PLATFORM_ASAN_HAVE_STRCPY
    USE(strcpy);
#endif
#ifdef PLATFORM_ASAN_HAVE_STRLCPY
    USE(strlcpy);
#endif
#ifdef PLATFORM_ASAN_HAVE_STRLCAT
    USE(strncpy);
#endif
#ifdef PLATFORM_ASAN_HAVE_STRNCAT
    USE(strlcat);
#endif
#ifdef PLATFORM_ASAN_HAVE_STRNCPY
    USE(strncat);
#endif
#ifdef PLATFORM_ASAN_HAVE_STRNLEN
    USE(strnlen);
#endif
#ifdef PLATFORM_ASAN_HAVE_STRLEN
    USE(strlen);
#endif

#undef USE
}
