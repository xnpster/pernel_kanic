/** @file

OcGuardLib

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http:// opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

/* The header file for UBSAN support in JOS based on the UBSAN porting example which was given with the task. */

#ifndef UBSAN_H
#define UBSAN_H

#include <inc/assert.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/stdarg.h>
#include <inc/types.h>

/*  Mark long double as supported. */
#ifndef __HAVE_LONG_DOUBLE
#define __HAVE_LONG_DOUBLE
#endif

/*  Printing macros are not supported in JOS. */
#ifndef PRIx8
#define PRIx8  "hhx"
#define PRIx16 "hx"
#define PRIx32 "x"
#define PRIx64 "lx"
#define PRId32 "d"
#define PRId64 "ld"
#define PRIu32 "u"
#define PRIu64 "lu"
#endif

/*  Limits for unsigned types. */
#ifndef UINT16_MAX
#define UINT8_MAX  0xffU
#define UINT16_MAX 0xffffU
#define UINT32_MAX 0xffffffffU
#define UINT64_MAX 0xffffffffffffffffULL
#endif

/*  PATH_MAX is not defined in JOS. */
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

/*  CHAR_BIT is not defined in JOS. */
#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

/*  Bit manipulation is not present in JOS. */
#ifndef __BIT
#define __BIT(__n)                                            \
    (((uintptr_t)(__n) >= CHAR_BIT * sizeof(uintptr_t)) ? 0 : \
                                                          ((uintptr_t)1 << (uintptr_t)((__n) & (CHAR_BIT * sizeof(uintptr_t) - 1))))
#endif

/*  Extended bit manipulation is also not present in JOS. */
#ifndef __LOWEST_SET_BIT
/* find least significant bit that is set */
#define __LOWEST_SET_BIT(__mask) ((((__mask)-1) & (__mask)) ^ (__mask))
#define __SHIFTOUT(__x, __mask)  (((__x) & (__mask)) / __LOWEST_SET_BIT(__mask))
#define __SHIFTIN(__x, __mask)   ((__x)*__LOWEST_SET_BIT(__mask))
#define __SHIFTOUT_MASK(__mask)  __SHIFTOUT((__mask), (__mask))
#endif

/*  BSD-like bit manipulation is not present in JOS. */
#ifndef CLR
#define SET(t, f)   ((t) |= (f))
#define ISSET(t, f) ((t) & (f))
#define CLR(t, f)   ((t) &= ~(f))
#endif

/* Return the number of elements in a statically allocated array */
#ifndef __arraycount
#define __arraycount(__x) (sizeof(__x) / sizeof(__x[0]))
#endif

#endif /* UBSAN_H */
