/*
 * Copyright (c) 2006 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef ASM64_H
#define ASM64_H

/* Helper macros for 64-bit mode switching */

/* Long jump to 64-bit space from 32-bit compatibility mode */
#define ENTER_64BIT_MODE()        \
    push $GD_KT;                  \
    call 1f;                      \
    1 : addl $(2f - 1b), (% esp); \
    lretl;                        \
    2 :.code64

/* Long jump to 32-bit compatibility mode from 64-bit space.
 * Effected by long return similar to ENTER_64BIT_MODE */
#define ENTER_COMPAT_MODE()       \
    call 3f;                      \
    3 : addq $(4f - 3b), (% rsp); \
    movl $GD_KT32, 4(% rsp);      \
    lretl;                        \
    4 :.code32

#endif /* ASM64_H */
