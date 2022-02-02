/*    $NetBSD: ubsan.c,v 1.3 2018/08/03 16:31:04 kamil Exp $    */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* The implementation of UBSAN for JOS based on NetBSD micro UBSAN implementation which was given with the task. */

#include "ubsan.h"

#define ASSERT(x) assert(x)

#define REINTERPRET_CAST(__dt, __st) ((__dt)(__st))
#define STATIC_CAST(__dt, __st)      ((__dt)(__st))

#define ACK_REPORTED __BIT(31)

#define MUL_STRING    "*"
#define PLUS_STRING   "+"
#define MINUS_STRING  "-"
#define DIVREM_STRING "divrem"

#define CFI_VCALL         0
#define CFI_NVCALL        1
#define CFI_DERIVEDCAST   2
#define CFI_UNRELATEDCAST 3
#define CFI_ICALL         4
#define CFI_NVMFCALL      5
#define CFI_VMFCALL       6

#define NUMBER_MAXLEN   128
#define LOCATION_MAXLEN (PATH_MAX + 32 /* ':LINE:COLUMN' */)

#define WIDTH_8   8
#define WIDTH_16  16
#define WIDTH_32  32
#define WIDTH_64  64
#define WIDTH_80  80
#define WIDTH_96  96
#define WIDTH_128 128

#define NUMBER_SIGNED_BIT 1U

#if __SIZEOF_INT128__
typedef __int128 longest;
typedef unsigned __int128 ulongest;
#else
typedef int64_t longest;
typedef uint64_t ulongest;
#endif

/* Undefined Behavior specific defines and structures */

#define KIND_INTEGER 0
#define KIND_FLOAT   1
#define KIND_UNKNOWN UINT16_MAX

struct CSourceLocation {
    char *mFilename;
    uint32_t mLine;
    uint32_t mColumn;
};

struct CTypeDescriptor {
    uint16_t mTypeKind;
    uint16_t mTypeInfo;
    uint8_t mTypeName[1];
};

struct COverflowData {
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mType;
};

struct CUnreachableData {
    struct CSourceLocation mLocation;
};

struct CCFICheckFailData {
    uint8_t mCheckKind;
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mType;
};

struct CDynamicTypeCacheMissData {
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mType;
    void *mTypeInfo;
    uint8_t mTypeCheckKind;
};

struct CFunctionTypeMismatchData {
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mType;
};

struct CImplicitConversionData {
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mFromType;
    struct CTypeDescriptor *mToType;
    uint8_t mKind;
};

struct CInvalidBuiltinData {
    struct CSourceLocation mLocation;
    uint8_t mKind;
};

struct CInvalidValueData {
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mType;
};

struct CNonNullArgData {
    struct CSourceLocation mLocation;
    struct CSourceLocation mAttributeLocation;
    int mArgIndex;
};

struct CNonNullReturnData {
    struct CSourceLocation mAttributeLocation;
};

struct COutOfBoundsData {
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mArrayType;
    struct CTypeDescriptor *mIndexType;
};

struct CPointerOverflowData {
    struct CSourceLocation mLocation;
};

struct CShiftOutOfBoundsData {
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mLHSType;
    struct CTypeDescriptor *mRHSType;
};

struct CTypeMismatchData {
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mType;
    unsigned long mLogAlignment;
    uint8_t mTypeCheckKind;
};

struct CTypeMismatchData_v1 {
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mType;
    uint8_t mLogAlignment;
    uint8_t mTypeCheckKind;
};

struct CVLABoundData {
    struct CSourceLocation mLocation;
    struct CTypeDescriptor *mType;
};

struct CFloatCastOverflowData {
    struct CSourceLocation mLocation; /* This field exists in this struct since 2015 August 11th */
    struct CTypeDescriptor *mFromType;
    struct CTypeDescriptor *mToType;
};

/* Local utility functions */
__attribute__((__format__(__printf__, 2, 3))) static void report(bool isFatal, const char *pFormat, ...);
static bool is_already_reported(struct CSourceLocation *pLocation);
static size_t z_deserialize_type_width(struct CTypeDescriptor *pType);
static void deserialize_location(char *pBuffer, size_t zBUfferLength, struct CSourceLocation *pLocation);
#ifdef __SIZEOF_INT128__
static void deserialize_uint128_t(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, __uint128_t U128);
#endif
static void deserialize_number_signed(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, longest L);
static void deserialize_number_unsigned(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, ulongest L);
static void deserialize_float_over_pointer(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long *pNumber);
static void deserialize_float_inlined(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long ulNumber);
static longest llli_get_number(char *szLocation, struct CTypeDescriptor *pType, unsigned long ulNumber);
static ulongest lllu_get_number(char *szLocation, struct CTypeDescriptor *pType, unsigned long ulNumber);
static void deserialize_number_float(char *szLocation, char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long ulNumber);
static void deserialize_number(char *szLocation, char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long ulNumber);
static const char *deserialize_type_check_kind(uint8_t hhuTypeCheckKind);
static const char *deserialize_builtin_check_kind(uint8_t hhuBuiltinCheckKind);
static const char *deserialize_cfi_check_kind(uint8_t hhuCFICheckKind);
static const char *deserialize_implicit_conversion_check_kind(uint8_t hhuImplicitConversionCheckKind);
static bool is_negative_number(char *szLocation, struct CTypeDescriptor *pType, unsigned long ulNumber);
static bool is_shift_exponent_too_large(char *szLocation, struct CTypeDescriptor *pType, unsigned long ulNumber, size_t zWidth);

/* Unused in this implementation, emitted by the C++ check dynamic type cast. */
intptr_t __ubsan_vptr_type_cache[128];

/* Public symbols used in the instrumentation of the code generation part */
void __ubsan_handle_add_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_add_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_builtin_unreachable(struct CUnreachableData *pData);
void __ubsan_handle_cfi_bad_type(struct CCFICheckFailData *pData, unsigned long ulVtable, bool bValidVtable, bool FromUnrecoverableHandler, unsigned long ProgramCounter, unsigned long FramePointer);
void __ubsan_handle_cfi_check_fail(struct CCFICheckFailData *pData, unsigned long ulValue, unsigned long ulValidVtable);
void __ubsan_handle_cfi_check_fail_abort(struct CCFICheckFailData *pData, unsigned long ulValue, unsigned long ulValidVtable);
void __ubsan_handle_divrem_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_divrem_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_dynamic_type_cache_miss(struct CDynamicTypeCacheMissData *pData, unsigned long ulPointer, unsigned long ulHash);
void __ubsan_handle_dynamic_type_cache_miss_abort(struct CDynamicTypeCacheMissData *pData, unsigned long ulPointer, unsigned long ulHash);
void __ubsan_handle_float_cast_overflow(struct CFloatCastOverflowData *pData, unsigned long ulFrom);
void __ubsan_handle_float_cast_overflow_abort(struct CFloatCastOverflowData *pData, unsigned long ulFrom);
void __ubsan_handle_function_type_mismatch(struct CFunctionTypeMismatchData *pData, unsigned long ulFunction);
void __ubsan_handle_function_type_mismatch_abort(struct CFunctionTypeMismatchData *pData, unsigned long ulFunction);
void __ubsan_handle_implicit_conversion(struct CImplicitConversionData *pData, unsigned long ulFrom, unsigned long ulTo);
void __ubsan_handle_implicit_conversion_abort(struct CImplicitConversionData *pData, unsigned long ulFrom, unsigned long ulTo);
void __ubsan_handle_invalid_builtin(struct CInvalidBuiltinData *pData);
void __ubsan_handle_invalid_builtin_abort(struct CInvalidBuiltinData *pData);
void __ubsan_handle_load_invalid_value(struct CInvalidValueData *pData, unsigned long ulVal);
void __ubsan_handle_load_invalid_value_abort(struct CInvalidValueData *pData, unsigned long ulVal);
void __ubsan_handle_missing_return(struct CUnreachableData *pData);
void __ubsan_handle_mul_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_mul_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_negate_overflow(struct COverflowData *pData, unsigned long ulOldVal);
void __ubsan_handle_negate_overflow_abort(struct COverflowData *pData, unsigned long ulOldVal);
void __ubsan_handle_nonnull_arg(struct CNonNullArgData *pData);
void __ubsan_handle_nonnull_arg_abort(struct CNonNullArgData *pData);
void __ubsan_handle_nonnull_return_v1(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer);
void __ubsan_handle_nonnull_return_v1_abort(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer);
void __ubsan_handle_nullability_arg(struct CNonNullArgData *pData);
void __ubsan_handle_nullability_arg_abort(struct CNonNullArgData *pData);
void __ubsan_handle_nullability_return_v1(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer);
void __ubsan_handle_nullability_return_v1_abort(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer);
void __ubsan_handle_out_of_bounds(struct COutOfBoundsData *pData, unsigned long ulIndex);
void __ubsan_handle_out_of_bounds_abort(struct COutOfBoundsData *pData, unsigned long ulIndex);
void __ubsan_handle_pointer_overflow(struct CPointerOverflowData *pData, unsigned long ulBase, unsigned long ulResult);
void __ubsan_handle_pointer_overflow_abort(struct CPointerOverflowData *pData, unsigned long ulBase, unsigned long ulResult);
void __ubsan_handle_shift_out_of_bounds(struct CShiftOutOfBoundsData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_shift_out_of_bounds_abort(struct CShiftOutOfBoundsData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_sub_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_sub_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_type_mismatch(struct CTypeMismatchData *pData, unsigned long ulPointer);
void __ubsan_handle_type_mismatch_abort(struct CTypeMismatchData *pData, unsigned long ulPointer);
void __ubsan_handle_type_mismatch_v1(struct CTypeMismatchData_v1 *pData, unsigned long ulPointer);
void __ubsan_handle_type_mismatch_v1_abort(struct CTypeMismatchData_v1 *pData, unsigned long ulPointer);
void __ubsan_handle_vla_bound_not_positive(struct CVLABoundData *pData, unsigned long ulBound);
void __ubsan_handle_vla_bound_not_positive_abort(struct CVLABoundData *pData, unsigned long ulBound);
void __ubsan_get_current_report_data(const char **ppOutIssueKind, const char **ppOutMessage, const char **ppOutFilename, uint32_t *pOutLine, uint32_t *pOutCol, char **ppOutMemoryAddr);

static void HandleOverflow(bool isFatal, struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS, const char *szOperation);
static void HandleNegateOverflow(bool isFatal, struct COverflowData *pData, unsigned long ulOldValue);
static void HandleBuiltinUnreachable(bool isFatal, struct CUnreachableData *pData);
static void HandleTypeMismatch(bool isFatal, struct CSourceLocation *mLocation, struct CTypeDescriptor *mType, unsigned long mLogAlignment, uint8_t mTypeCheckKind, unsigned long ulPointer);
static void HandleVlaBoundNotPositive(bool isFatal, struct CVLABoundData *pData, unsigned long ulBound);
static void HandleOutOfBounds(bool isFatal, struct COutOfBoundsData *pData, unsigned long ulIndex);
static void HandleShiftOutOfBounds(bool isFatal, struct CShiftOutOfBoundsData *pData, unsigned long ulLHS, unsigned long ulRHS);
static void HandleImplicitConversion(bool isFatal, struct CImplicitConversionData *pData, unsigned long ulFrom, unsigned long ulTo);
static void HandleLoadInvalidValue(bool isFatal, struct CInvalidValueData *pData, unsigned long ulValue);
static void HandleInvalidBuiltin(bool isFatal, struct CInvalidBuiltinData *pData);
static void HandleFunctionTypeMismatch(bool isFatal, struct CFunctionTypeMismatchData *pData, unsigned long ulFunction);
static void HandleCFIBadType(bool isFatal, struct CCFICheckFailData *pData, unsigned long ulVtable, bool *bValidVtable, bool *FromUnrecoverableHandler, unsigned long *ProgramCounter, unsigned long *FramePointer);
static void HandleDynamicTypeCacheMiss(bool isFatal, struct CDynamicTypeCacheMissData *pData, unsigned long ulPointer, unsigned long ulHash);
static void HandleFloatCastOverflow(bool isFatal, struct CFloatCastOverflowData *pData, unsigned long ulFrom);
static void HandleMissingReturn(bool isFatal, struct CUnreachableData *pData);
static void HandleNonnullArg(bool isFatal, struct CNonNullArgData *pData);
static void HandleNonnullReturn(bool isFatal, struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer);
static void HandlePointerOverflow(bool isFatal, struct CPointerOverflowData *pData, unsigned long ulBase, unsigned long ulResult);

static void
HandleOverflow(bool isFatal, struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS, const char *szOperation) {
    char szLocation[LOCATION_MAXLEN];
    char szLHS[NUMBER_MAXLEN];
    char szRHS[NUMBER_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);
    deserialize_number(szLocation, szLHS, NUMBER_MAXLEN, pData->mType, ulLHS);
    deserialize_number(szLocation, szRHS, NUMBER_MAXLEN, pData->mType, ulRHS);

    report(isFatal, "UBSAN: Undefined Behavior in %s, %s integer overflow: %s %s %s cannot be represented in type %s\n",
           szLocation, ISSET(pData->mType->mTypeInfo, NUMBER_SIGNED_BIT) ? "signed" : "unsigned", szLHS, szOperation, szRHS, pData->mType->mTypeName);
}

static void
HandleNegateOverflow(bool isFatal, struct COverflowData *pData, unsigned long ulOldValue) {
    char szLocation[LOCATION_MAXLEN];
    char szOldValue[NUMBER_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);
    deserialize_number(szLocation, szOldValue, NUMBER_MAXLEN, pData->mType, ulOldValue);

    report(isFatal, "UBSAN: Undefined Behavior in %s, negation of %s cannot be represented in type %s\n",
           szLocation, szOldValue, pData->mType->mTypeName);
}

static void
HandleBuiltinUnreachable(bool isFatal, struct CUnreachableData *pData) {
    char szLocation[LOCATION_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);

    report(isFatal, "UBSAN: Undefined Behavior in %s, calling __builtin_unreachable()\n",
           szLocation);
}

static void
HandleTypeMismatch(bool isFatal, struct CSourceLocation *mLocation, struct CTypeDescriptor *mType, unsigned long mLogAlignment, uint8_t mTypeCheckKind, unsigned long ulPointer) {
    char szLocation[LOCATION_MAXLEN];

    ASSERT(mLocation);
    ASSERT(mType);

    if (is_already_reported(mLocation))
        return;

    deserialize_location(szLocation, LOCATION_MAXLEN, mLocation);

    if (ulPointer == 0) {
        report(isFatal, "UBSAN: Undefined Behavior in %s, %s null pointer of type %s\n",
               szLocation, deserialize_type_check_kind(mTypeCheckKind), mType->mTypeName);
    } else if ((mLogAlignment - 1) & ulPointer) {
        report(isFatal, "UBSAN: Undefined Behavior in %s, %s misaligned address %p for type %s which requires %ld byte alignment\n",
               szLocation, deserialize_type_check_kind(mTypeCheckKind), REINTERPRET_CAST(void *, ulPointer), mType->mTypeName, mLogAlignment);
    } else {
        report(isFatal, "UBSAN: Undefined Behavior in %s, %s address %p with insufficient space for an object of type %s\n",
               szLocation, deserialize_type_check_kind(mTypeCheckKind), REINTERPRET_CAST(void *, ulPointer), mType->mTypeName);
    }
}

static void
HandleVlaBoundNotPositive(bool isFatal, struct CVLABoundData *pData, unsigned long ulBound) {
    char szLocation[LOCATION_MAXLEN];
    char szBound[NUMBER_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);
    deserialize_number(szLocation, szBound, NUMBER_MAXLEN, pData->mType, ulBound);

    report(isFatal, "UBSAN: Undefined Behavior in %s, variable length array bound value %s <= 0\n",
           szLocation, szBound);
}

static void
HandleOutOfBounds(bool isFatal, struct COutOfBoundsData *pData, unsigned long ulIndex) {
    char szLocation[LOCATION_MAXLEN];
    char szIndex[NUMBER_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);
    deserialize_number(szLocation, szIndex, NUMBER_MAXLEN, pData->mIndexType, ulIndex);

    report(isFatal, "UBSAN: Undefined Behavior in %s, index %s is out of range for type %s\n",
           szLocation, szIndex, pData->mArrayType->mTypeName);
}

static void
HandleShiftOutOfBounds(bool isFatal, struct CShiftOutOfBoundsData *pData, unsigned long ulLHS, unsigned long ulRHS) {
    char szLocation[LOCATION_MAXLEN];
    char szLHS[NUMBER_MAXLEN];
    char szRHS[NUMBER_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);
    deserialize_number(szLocation, szLHS, NUMBER_MAXLEN, pData->mLHSType, ulLHS);
    deserialize_number(szLocation, szRHS, NUMBER_MAXLEN, pData->mRHSType, ulRHS);

    if (is_negative_number(szLocation, pData->mRHSType, ulRHS)) {
        report(isFatal, "UBSAN: Undefined Behavior in %s, shift exponent %s is negative\n",
               szLocation, szRHS);
    } else if (is_shift_exponent_too_large(szLocation, pData->mRHSType, ulRHS, z_deserialize_type_width(pData->mLHSType))) {
        report(isFatal, "UBSAN: Undefined Behavior in %s, shift exponent %s is too large for %zu-bit type %s\n",
               szLocation, szRHS, z_deserialize_type_width(pData->mLHSType), pData->mLHSType->mTypeName);
    } else if (is_negative_number(szLocation, pData->mLHSType, ulLHS)) {
        report(isFatal, "UBSAN: Undefined Behavior in %s, left shift of negative value %s\n",
               szLocation, szLHS);
    } else {
        report(isFatal, "UBSAN: Undefined Behavior in %s, left shift of %s by %s places cannot be represented in type %s\n",
               szLocation, szLHS, szRHS, pData->mLHSType->mTypeName);
    }
}

static void
HandleImplicitConversion(bool isFatal, struct CImplicitConversionData *pData, unsigned long ulFrom, unsigned long ulTo) {
    char szLocation[LOCATION_MAXLEN];
    char szFrom[NUMBER_MAXLEN];
    char szTo[NUMBER_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation))
        return;

    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);
    deserialize_number(szLocation, szFrom, NUMBER_MAXLEN, pData->mFromType, ulFrom);
    deserialize_number(szLocation, szTo, NUMBER_MAXLEN, pData->mToType, ulTo);

    report(isFatal, "UBSAN: Undefined Behavior in %s, %s from %s to %s\n",
           szLocation, deserialize_implicit_conversion_check_kind(pData->mKind), pData->mFromType->mTypeName, pData->mToType->mTypeName);
}

static void
HandleLoadInvalidValue(bool isFatal, struct CInvalidValueData *pData, unsigned long ulValue) {
    char szLocation[LOCATION_MAXLEN];
    char szValue[NUMBER_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);
    deserialize_number(szLocation, szValue, NUMBER_MAXLEN, pData->mType, ulValue);

    report(isFatal, "UBSAN: Undefined Behavior in %s, load of value %s is not a valid value for type %s\n",
           szLocation, szValue, pData->mType->mTypeName);
}

static void
HandleInvalidBuiltin(bool isFatal, struct CInvalidBuiltinData *pData) {
    char szLocation[LOCATION_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);

    report(isFatal, "UBSAN: Undefined Behavior in %s, passing zero to %s, which is not a valid argument\n",
           szLocation, deserialize_builtin_check_kind(pData->mKind));
}

static void
HandleFunctionTypeMismatch(bool isFatal, struct CFunctionTypeMismatchData *pData, unsigned long ulFunction) {
    char szLocation[LOCATION_MAXLEN];

    /*
     * There is no a portable C solution to translate an address of a
     * function to its name. On the cost of getting this routine simple
     * and portable without ifdefs between the userland and the kernel
     * just print the address of the function as-is.
     *
     * For better diagnostic messages in the userland, users shall use
     * the full upstream version shipped along with the compiler toolchain.
     */

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);

    report(isFatal, "UBSAN: Undefined Behavior in %s, call to function %#lx through pointer to incorrect function type %s\n",
           szLocation, ulFunction, pData->mType->mTypeName);
}

static void
HandleCFIBadType(bool isFatal, struct CCFICheckFailData *pData, unsigned long ulVtable, bool *bValidVtable, bool *FromUnrecoverableHandler, unsigned long *ProgramCounter, unsigned long *FramePointer) {
    char szLocation[LOCATION_MAXLEN];

    /*
     * This is a minimal implementation without diving into C++
     * specifics and (Itanium) ABI deserialization.
     */

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);

    if (pData->mCheckKind == CFI_ICALL || pData->mCheckKind == CFI_VMFCALL) {
        report(isFatal, "UBSAN: Undefined Behavior in %s, control flow integrity check for type %s failed during %s (vtable address %#lx)\n",
               szLocation, pData->mType->mTypeName, deserialize_cfi_check_kind(pData->mCheckKind), ulVtable);
    } else {
        report(isFatal || FromUnrecoverableHandler, "UBSAN: Undefined Behavior in %s, control flow integrity check for type %s failed during %s (vtable address %#lx; %s vtable; from %s handler; Program Counter %#lx; Frame Pointer %#lx)\n",
               szLocation, pData->mType->mTypeName, deserialize_cfi_check_kind(pData->mCheckKind), ulVtable, bValidVtable && *bValidVtable ? "valid" : "invalid", FromUnrecoverableHandler && *FromUnrecoverableHandler ? "unrecoverable" : "recoverable", ProgramCounter ? *ProgramCounter : 0, FramePointer ? *FramePointer : 0);
    }
}

static void
HandleDynamicTypeCacheMiss(bool isFatal, struct CDynamicTypeCacheMissData *pData, unsigned long ulPointer, unsigned long ulHash) {
#if 0
    char szLocation[LOCATION_MAXLEN];

    /*
     * Unimplemented.
     *
     * This UBSAN handler is special as the check has to be impelemented
     * in an implementation. In order to handle it there is need to
     * introspect into C++ ABI internals (RTTI) and use low-level
     * C++ runtime interfaces.
     */

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation))
        return;

    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);

    report(isFatal, "UBSAN: Undefined Behavior in %s, %s address %#lx which might not point to an object of type %s\n"
          szLocation, deserialize_type_check_kind(pData->mTypeCheckKind), ulPointer, pData->mType);
#endif
}

static void
HandleFloatCastOverflow(bool isFatal, struct CFloatCastOverflowData *pData, unsigned long ulFrom) {
    char szLocation[LOCATION_MAXLEN];
    char szFrom[NUMBER_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);
    deserialize_number(szLocation, szFrom, NUMBER_MAXLEN, pData->mFromType, ulFrom);

    report(isFatal, "UBSAN: Undefined Behavior in %s, %s (of type %s) is outside the range of representable values of type %s\n",
           szLocation, szFrom, pData->mFromType->mTypeName, pData->mToType->mTypeName);
}

static void
HandleMissingReturn(bool isFatal, struct CUnreachableData *pData) {
    char szLocation[LOCATION_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);

    report(isFatal, "UBSAN: Undefined Behavior in %s, execution reached the end of a value-returning function without returning a value\n",
           szLocation);
}

static void
HandleNonnullArg(bool isFatal, struct CNonNullArgData *pData) {
    char szLocation[LOCATION_MAXLEN];
    char szAttributeLocation[LOCATION_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);
    if (pData->mAttributeLocation.mFilename)
        deserialize_location(szAttributeLocation, LOCATION_MAXLEN, &pData->mAttributeLocation);
    else
        szAttributeLocation[0] = '\0';

    report(isFatal, "UBSAN: Undefined Behavior in %s, null pointer passed as argument %d, which is declared to never be null%s%s\n",
           szLocation, pData->mArgIndex, pData->mAttributeLocation.mFilename ? ", nonnull/_Nonnull specified in " : "", szAttributeLocation);
}

static void
HandleNonnullReturn(bool isFatal, struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer) {
    char szLocation[LOCATION_MAXLEN];
    char szAttributeLocation[LOCATION_MAXLEN];

    ASSERT(pData);
    ASSERT(pLocationPointer);

    if (is_already_reported(pLocationPointer)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, pLocationPointer);
    if (pData->mAttributeLocation.mFilename)
        deserialize_location(szAttributeLocation, LOCATION_MAXLEN, &pData->mAttributeLocation);
    else
        szAttributeLocation[0] = '\0';

    report(isFatal, "UBSAN: Undefined Behavior in %s, null pointer returned from function declared to never return null%s%s\n",
           szLocation, pData->mAttributeLocation.mFilename ? ", nonnull/_Nonnull specified in " : "", szAttributeLocation);
}

static void
HandlePointerOverflow(bool isFatal, struct CPointerOverflowData *pData, unsigned long ulBase, unsigned long ulResult) {
    char szLocation[LOCATION_MAXLEN];

    ASSERT(pData);

    if (is_already_reported(&pData->mLocation)) {
        return;
    }
    deserialize_location(szLocation, LOCATION_MAXLEN, &pData->mLocation);

    report(isFatal, "UBSAN: Undefined Behavior in %s, pointer expression with base %#lx overflowed to %#lx\n",
           szLocation, ulBase, ulResult);
}

/* Definions of public symbols emitted by the instrumentation code */
void
__ubsan_handle_add_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS) {

    ASSERT(pData);

    HandleOverflow(false, pData, ulLHS, ulRHS, PLUS_STRING);
}

void
__ubsan_handle_add_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS) {

    ASSERT(pData);

    HandleOverflow(true, pData, ulLHS, ulRHS, PLUS_STRING);
}

void
__ubsan_handle_builtin_unreachable(struct CUnreachableData *pData) {

    ASSERT(pData);

    HandleBuiltinUnreachable(true, pData);
}

void
__ubsan_handle_cfi_bad_type(struct CCFICheckFailData *pData, unsigned long ulVtable, bool bValidVtable, bool FromUnrecoverableHandler, unsigned long ProgramCounter, unsigned long FramePointer) {

    ASSERT(pData);

    HandleCFIBadType(false, pData, ulVtable, &bValidVtable, &FromUnrecoverableHandler, &ProgramCounter, &FramePointer);
}

void
__ubsan_handle_cfi_check_fail(struct CCFICheckFailData *pData, unsigned long ulValue, unsigned long ulValidVtable) {

    ASSERT(pData);

    HandleCFIBadType(false, pData, ulValue, 0, 0, 0, 0);
}

void
__ubsan_handle_cfi_check_fail_abort(struct CCFICheckFailData *pData, unsigned long ulValue, unsigned long ulValidVtable) {

    ASSERT(pData);

    HandleCFIBadType(true, pData, ulValue, 0, 0, 0, 0);
}

void
__ubsan_handle_divrem_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS) {

    ASSERT(pData);

    HandleOverflow(false, pData, ulLHS, ulRHS, DIVREM_STRING);
}

void
__ubsan_handle_divrem_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS) {

    ASSERT(pData);

    HandleOverflow(true, pData, ulLHS, ulRHS, DIVREM_STRING);
}

void
__ubsan_handle_dynamic_type_cache_miss(struct CDynamicTypeCacheMissData *pData, unsigned long ulPointer, unsigned long ulHash) {

    ASSERT(pData);

    HandleDynamicTypeCacheMiss(false, pData, ulPointer, ulHash);
}

void
__ubsan_handle_dynamic_type_cache_miss_abort(struct CDynamicTypeCacheMissData *pData, unsigned long ulPointer, unsigned long ulHash) {

    ASSERT(pData);

    HandleDynamicTypeCacheMiss(false, pData, ulPointer, ulHash);
}

void
__ubsan_handle_float_cast_overflow(struct CFloatCastOverflowData *pData, unsigned long ulFrom) {

    ASSERT(pData);

    HandleFloatCastOverflow(false, pData, ulFrom);
}

void
__ubsan_handle_float_cast_overflow_abort(struct CFloatCastOverflowData *pData, unsigned long ulFrom) {

    ASSERT(pData);

    HandleFloatCastOverflow(true, pData, ulFrom);
}

void
__ubsan_handle_function_type_mismatch(struct CFunctionTypeMismatchData *pData, unsigned long ulFunction) {

    ASSERT(pData);

    HandleFunctionTypeMismatch(false, pData, ulFunction);
}

void
__ubsan_handle_function_type_mismatch_abort(struct CFunctionTypeMismatchData *pData, unsigned long ulFunction) {

    ASSERT(pData);

    HandleFunctionTypeMismatch(false, pData, ulFunction);
}

void
__ubsan_handle_implicit_conversion(struct CImplicitConversionData *pData, unsigned long ulFrom, unsigned long ulTo) {
    ASSERT(pData);

    HandleImplicitConversion(false, pData, ulFrom, ulTo);
}

void
__ubsan_handle_implicit_conversion_abort(struct CImplicitConversionData *pData, unsigned long ulFrom, unsigned long ulTo) {

    ASSERT(pData);

    HandleImplicitConversion(false, pData, ulFrom, ulTo);
}

void
__ubsan_handle_invalid_builtin(struct CInvalidBuiltinData *pData) {

    ASSERT(pData);

    HandleInvalidBuiltin(true, pData);
}

void
__ubsan_handle_invalid_builtin_abort(struct CInvalidBuiltinData *pData) {

    ASSERT(pData);

    HandleInvalidBuiltin(true, pData);
}

void
__ubsan_handle_load_invalid_value(struct CInvalidValueData *pData, unsigned long ulValue) {

    ASSERT(pData);

    HandleLoadInvalidValue(false, pData, ulValue);
}

void
__ubsan_handle_load_invalid_value_abort(struct CInvalidValueData *pData, unsigned long ulValue) {

    ASSERT(pData);

    HandleLoadInvalidValue(true, pData, ulValue);
}

void
__ubsan_handle_missing_return(struct CUnreachableData *pData) {

    ASSERT(pData);

    HandleMissingReturn(true, pData);
}

void
__ubsan_handle_mul_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS) {

    ASSERT(pData);

    HandleOverflow(false, pData, ulLHS, ulRHS, MUL_STRING);
}

void
__ubsan_handle_mul_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS) {

    ASSERT(pData);

    HandleOverflow(true, pData, ulLHS, ulRHS, MUL_STRING);
}

void
__ubsan_handle_negate_overflow(struct COverflowData *pData, unsigned long ulOldValue) {

    ASSERT(pData);

    HandleNegateOverflow(false, pData, ulOldValue);
}

void
__ubsan_handle_negate_overflow_abort(struct COverflowData *pData, unsigned long ulOldValue) {

    ASSERT(pData);

    HandleNegateOverflow(true, pData, ulOldValue);
}

void
__ubsan_handle_nonnull_arg(struct CNonNullArgData *pData) {

    ASSERT(pData);

    HandleNonnullArg(false, pData);
}

void
__ubsan_handle_nonnull_arg_abort(struct CNonNullArgData *pData) {

    ASSERT(pData);

    HandleNonnullArg(true, pData);
}

void
__ubsan_handle_nonnull_return_v1(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer) {

    ASSERT(pData);
    ASSERT(pLocationPointer);

    HandleNonnullReturn(false, pData, pLocationPointer);
}

void
__ubsan_handle_nonnull_return_v1_abort(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer) {

    ASSERT(pData);
    ASSERT(pLocationPointer);

    HandleNonnullReturn(true, pData, pLocationPointer);
}

void
__ubsan_handle_nullability_arg(struct CNonNullArgData *pData) {

    ASSERT(pData);

    HandleNonnullArg(false, pData);
}

void
__ubsan_handle_nullability_arg_abort(struct CNonNullArgData *pData) {

    ASSERT(pData);

    HandleNonnullArg(true, pData);
}

void
__ubsan_handle_nullability_return_v1(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer) {

    ASSERT(pData);
    ASSERT(pLocationPointer);

    HandleNonnullReturn(false, pData, pLocationPointer);
}

void
__ubsan_handle_nullability_return_v1_abort(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer) {

    ASSERT(pData);
    ASSERT(pLocationPointer);

    HandleNonnullReturn(true, pData, pLocationPointer);
}

void
__ubsan_handle_out_of_bounds(struct COutOfBoundsData *pData, unsigned long ulIndex) {

    ASSERT(pData);

    HandleOutOfBounds(false, pData, ulIndex);
}

void
__ubsan_handle_out_of_bounds_abort(struct COutOfBoundsData *pData, unsigned long ulIndex) {

    ASSERT(pData);

    HandleOutOfBounds(true, pData, ulIndex);
}

void
__ubsan_handle_pointer_overflow(struct CPointerOverflowData *pData, unsigned long ulBase, unsigned long ulResult) {

    ASSERT(pData);

    HandlePointerOverflow(false, pData, ulBase, ulResult);
}

void
__ubsan_handle_pointer_overflow_abort(struct CPointerOverflowData *pData, unsigned long ulBase, unsigned long ulResult) {

    ASSERT(pData);

    HandlePointerOverflow(true, pData, ulBase, ulResult);
}

void
__ubsan_handle_shift_out_of_bounds(struct CShiftOutOfBoundsData *pData, unsigned long ulLHS, unsigned long ulRHS) {

    ASSERT(pData);

    HandleShiftOutOfBounds(false, pData, ulLHS, ulRHS);
}

void
__ubsan_handle_shift_out_of_bounds_abort(struct CShiftOutOfBoundsData *pData, unsigned long ulLHS, unsigned long ulRHS) {

    ASSERT(pData);

    HandleShiftOutOfBounds(true, pData, ulLHS, ulRHS);
}

void
__ubsan_handle_sub_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS) {

    ASSERT(pData);

    HandleOverflow(false, pData, ulLHS, ulRHS, MINUS_STRING);
}

void
__ubsan_handle_sub_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS) {

    ASSERT(pData);

    HandleOverflow(true, pData, ulLHS, ulRHS, MINUS_STRING);
}

void
__ubsan_handle_type_mismatch(struct CTypeMismatchData *pData, unsigned long ulPointer) {

    ASSERT(pData);

    HandleTypeMismatch(false, &pData->mLocation, pData->mType, pData->mLogAlignment, pData->mTypeCheckKind, ulPointer);
}

void
__ubsan_handle_type_mismatch_abort(struct CTypeMismatchData *pData, unsigned long ulPointer) {

    ASSERT(pData);

    HandleTypeMismatch(true, &pData->mLocation, pData->mType, pData->mLogAlignment, pData->mTypeCheckKind, ulPointer);
}

void
__ubsan_handle_type_mismatch_v1(struct CTypeMismatchData_v1 *pData, unsigned long ulPointer) {

    ASSERT(pData);

    HandleTypeMismatch(false, &pData->mLocation, pData->mType, __BIT(pData->mLogAlignment), pData->mTypeCheckKind, ulPointer);
}

void
__ubsan_handle_type_mismatch_v1_abort(struct CTypeMismatchData_v1 *pData, unsigned long ulPointer) {

    ASSERT(pData);

    HandleTypeMismatch(true, &pData->mLocation, pData->mType, __BIT(pData->mLogAlignment), pData->mTypeCheckKind, ulPointer);
}

void
__ubsan_handle_vla_bound_not_positive(struct CVLABoundData *pData, unsigned long ulBound) {

    ASSERT(pData);

    HandleVlaBoundNotPositive(false, pData, ulBound);
}

void
__ubsan_handle_vla_bound_not_positive_abort(struct CVLABoundData *pData, unsigned long ulBound) {

    ASSERT(pData);

    HandleVlaBoundNotPositive(true, pData, ulBound);
}

void
__ubsan_get_current_report_data(const char **ppOutIssueKind, const char **ppOutMessage, const char **ppOutFilename, uint32_t *pOutLine, uint32_t *pOutCol, char **ppOutMemoryAddr) {
    /*
     * Unimplemented.
     *
     * The __ubsan_on_report() feature is non trivial to implement in a
     * shared code between the kernel and userland. It's also opening
     * new sets of potential problems as we are not expected to slow down
     * execution of certain kernel subsystems (synchronization issues,
     * interrupt handling etc).
     *
     * A proper solution would need probably a lock-free bounded queue built
     * with atomic operations with the property of miltiple consumers and
     * multiple producers. Maintaining and validating such code is not
     * worth the effort.
     *
     * A legitimate user - besides testing framework - is a debugger plugin
     * intercepting reports from the UBSAN instrumentation. For such
     * scenarios it is better to run the Clang/GCC version.
     */
}

/* Local utility functions */

static void
report(bool isFatal, const char *pFormat, ...) {
    va_list ap;
    ASSERT(pFormat);
    va_start(ap, pFormat);
    /* The *v*print* functions can flush the va_list argument.
     * Create a local copy for each call to prevent invalid read. */
    va_list tmp;
    va_copy(tmp, ap);
    vcprintf(pFormat, tmp);
    va_end(tmp);
    if (isFatal) {
        panic("UBSAN: Fatal state");
        /* NOTREACHED */
    }
    va_end(ap);
}

static bool
is_already_reported(struct CSourceLocation *pLocation) {
    uint32_t siOldValue;
    volatile uint32_t *pLine;

    ASSERT(pLocation);

    pLine = &pLocation->mLine;

    do {
        siOldValue = *pLine;
    } while (__sync_val_compare_and_swap(pLine, siOldValue, siOldValue | ACK_REPORTED) != siOldValue);

    return ISSET(siOldValue, ACK_REPORTED);
}

static size_t
z_deserialize_type_width(struct CTypeDescriptor *pType) {
    size_t zWidth = 0;

    ASSERT(pType);

    switch (pType->mTypeKind) {
    case KIND_INTEGER:
        zWidth = __BIT(__SHIFTOUT(pType->mTypeInfo, ~NUMBER_SIGNED_BIT));
        break;
    case KIND_FLOAT:
        zWidth = pType->mTypeInfo;
        break;
    default:
        report(true, "UBSAN: Unknown variable type %#04" PRIx16 "\n", pType->mTypeKind);
        /* NOTREACHED */
    }

    /* Invalid width will be transformed to 0 */
    ASSERT(zWidth > 0);

    return zWidth;
}

static void
deserialize_location(char *pBuffer, size_t zBUfferLength, struct CSourceLocation *pLocation) {

    ASSERT(pLocation);
    ASSERT(pLocation->mFilename);

    snprintf(pBuffer, zBUfferLength, "%s:%" PRIu32 ":%" PRIu32, pLocation->mFilename, pLocation->mLine & (uint32_t)~ACK_REPORTED, pLocation->mColumn);
}

#ifdef __SIZEOF_INT128__
static void
deserialize_uint128_t(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, __uint128_t U128) {
    char szBuf[3]; /* 'XX\0' */
    char rgNumber[sizeof(ulongest)];
    ssize_t zI;

    memcpy(rgNumber, &U128, sizeof(U128));

    strlcpy(pBuffer, "Undecoded-128-bit-Integer-Type (0x", zBUfferLength);
#if BYTE_ORDER == LITTLE_ENDIAN
    for (zI = sizeof(ulongest) - 1; zI >= 0; zI--) {
#else
    for (zI = 0; zI < (ssize_t)sizeof(ulongest); zI++) {
#endif
        snprintf(szBuf, sizeof(szBuf), "%02" PRIx8, rgNumber[zI]);
        strlcat(pBuffer, szBuf, zBUfferLength);
    }
    strlcat(pBuffer, ")", zBUfferLength);
}
#endif

static void
deserialize_number_signed(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, longest L) {

    ASSERT(pBuffer);
    ASSERT(zBUfferLength > 0);
    ASSERT(pType);
    ASSERT(ISSET(pType->mTypeInfo, NUMBER_SIGNED_BIT));

    switch (z_deserialize_type_width(pType)) {
    default:
        ASSERT(0 && "Invalid codepath");
        /* NOTREACHED */
#ifdef __SIZEOF_INT128__
    case WIDTH_128:
        deserialize_uint128_t(pBuffer, zBUfferLength, pType, STATIC_CAST(__uint128_t, L));
        break;
#endif
    case WIDTH_64:
    case WIDTH_32:
    case WIDTH_16:
    case WIDTH_8:
        snprintf(pBuffer, zBUfferLength, "%" PRId64, STATIC_CAST(int64_t, L));
        break;
    }
}

static void
deserialize_number_unsigned(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, ulongest L) {

    ASSERT(pBuffer);
    ASSERT(zBUfferLength > 0);
    ASSERT(pType);
    ASSERT(!ISSET(pType->mTypeInfo, NUMBER_SIGNED_BIT));

    switch (z_deserialize_type_width(pType)) {
    default:
        ASSERT(0 && "Invalid codepath");
        /* NOTREACHED */
#ifdef __SIZEOF_INT128__
    case WIDTH_128:
        deserialize_uint128_t(pBuffer, zBUfferLength, pType, STATIC_CAST(__uint128_t, L));
        break;
#endif
    case WIDTH_64:
    case WIDTH_32:
    case WIDTH_16:
    case WIDTH_8:
        snprintf(pBuffer, zBUfferLength, "%" PRIu64, STATIC_CAST(uint64_t, L));
        break;
    }
}

static void
deserialize_float_over_pointer(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long *pNumber) {
    double D;
#ifdef __HAVE_LONG_DOUBLE
    long double LD;
#endif

    ASSERT(pBuffer);
    ASSERT(zBUfferLength > 0);
    ASSERT(pType);
    ASSERT(pNumber);
    /*
     * This function handles 64-bit number over a pointer on 32-bit CPUs.
     */
    ASSERT((sizeof(*pNumber) * CHAR_BIT < WIDTH_64) || (z_deserialize_type_width(pType) >= WIDTH_64));
    ASSERT(sizeof(D) == sizeof(uint64_t));
#ifdef __HAVE_LONG_DOUBLE
    ASSERT(sizeof(LD) > sizeof(uint64_t));
#endif

    switch (z_deserialize_type_width(pType)) {
#ifdef __HAVE_LONG_DOUBLE
    case WIDTH_128:
    case WIDTH_96:
    case WIDTH_80:
        memcpy(&LD, pNumber, sizeof(long double));
        snprintf(pBuffer, zBUfferLength, "%Lg", LD);
        break;
#endif
    case WIDTH_64:
        memcpy(&D, pNumber, sizeof(double));
        snprintf(pBuffer, zBUfferLength, "%g", D);
        break;
    }
}

static void
deserialize_float_inlined(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long ulNumber) {
    float F;
    double D;
    uint32_t U32;

    ASSERT(pBuffer);
    ASSERT(zBUfferLength > 0);
    ASSERT(pType);
    ASSERT(sizeof(F) == sizeof(uint32_t));
    ASSERT(sizeof(D) == sizeof(uint64_t));

    switch (z_deserialize_type_width(pType)) {
    case WIDTH_64:
        memcpy(&D, &ulNumber, sizeof(double));
        snprintf(pBuffer, zBUfferLength, "%g", D);
        break;
    case WIDTH_32:
        /*
         * On supported platforms sizeof(float)==sizeof(uint32_t)
         * unsigned long is either 32 or 64-bit, cast it to 32-bit
         * value in order to call memcpy(3) in an Endian-aware way.
         */
        U32 = STATIC_CAST(uint32_t, ulNumber);
        memcpy(&F, &U32, sizeof(float));
        snprintf(pBuffer, zBUfferLength, "%g", F);
        break;
    case WIDTH_16:
        snprintf(pBuffer, zBUfferLength, "Undecoded-16-bit-Floating-Type (%#04" PRIx16 ")", STATIC_CAST(uint16_t, ulNumber));
        break;
    }
}

static longest
llli_get_number(char *szLocation, struct CTypeDescriptor *pType, unsigned long ulNumber) {
    size_t zNumberWidth;
    longest L = 0;

    ASSERT(szLocation);
    ASSERT(pType);

    zNumberWidth = z_deserialize_type_width(pType);
    switch (zNumberWidth) {
    default:
        report(true, "UBSAN: Unexpected %zu-Bit Type in %s\n", zNumberWidth, szLocation);
        /* NOTREACHED */
    case WIDTH_128:
#ifdef __SIZEOF_INT128__
        memcpy(&L, REINTERPRET_CAST(longest *, ulNumber), sizeof(longest));
#else
        report(true, "UBSAN: Unexpected 128-Bit Type in %s\n", szLocation);
        /* NOTREACHED */
#endif
        break;
    case WIDTH_64:
        if (sizeof(ulNumber) * CHAR_BIT < WIDTH_64) {
            L = *REINTERPRET_CAST(int64_t *, ulNumber);
        } else {
            L = STATIC_CAST(int64_t, STATIC_CAST(uint64_t, ulNumber));
        }
        break;
    case WIDTH_32:
        L = STATIC_CAST(int32_t, STATIC_CAST(uint32_t, ulNumber));
        break;
    case WIDTH_16:
        L = STATIC_CAST(int16_t, STATIC_CAST(uint16_t, ulNumber));
        break;
    case WIDTH_8:
        L = STATIC_CAST(int8_t, STATIC_CAST(uint8_t, ulNumber));
        break;
    }

    return L;
}

static ulongest
lllu_get_number(char *szLocation, struct CTypeDescriptor *pType, unsigned long ulNumber) {
    size_t zNumberWidth;
    ulongest UL = 0;

    ASSERT(pType);

    zNumberWidth = z_deserialize_type_width(pType);
    switch (zNumberWidth) {
    default:
        report(true, "UBSAN: Unexpected %zu-Bit Type in %s\n", zNumberWidth, szLocation);
        /* NOTREACHED */
    case WIDTH_128:
#ifdef __SIZEOF_INT128__
        memcpy(&UL, REINTERPRET_CAST(ulongest *, ulNumber), sizeof(ulongest));
        break;
#else
        report(true, "UBSAN: Unexpected 128-Bit Type in %s\n", szLocation);
        /* NOTREACHED */
#endif
    case WIDTH_64:
        if (sizeof(ulNumber) * CHAR_BIT < WIDTH_64) {
            UL = *REINTERPRET_CAST(uint64_t *, ulNumber);
            break;
        }
        /* FALLTHROUGH */
    case WIDTH_32:
        /* FALLTHROUGH */
    case WIDTH_16:
        /* FALLTHROUGH */
    case WIDTH_8:
        UL = ulNumber;
        break;
    }

    return UL;
}

static void
deserialize_number_float(char *szLocation, char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long ulNumber) {
    size_t zNumberWidth;

    ASSERT(szLocation);
    ASSERT(pBuffer);
    ASSERT(zBUfferLength > 0);
    ASSERT(pType);
    ASSERT(pType->mTypeKind == KIND_FLOAT);

    zNumberWidth = z_deserialize_type_width(pType);
    switch (zNumberWidth) {
    default:
        report(true, "UBSAN: Unexpected %zu-Bit Type in %s\n", zNumberWidth, szLocation);
        /* NOTREACHED */
#ifdef __HAVE_LONG_DOUBLE
    case WIDTH_128:
    case WIDTH_96:
    case WIDTH_80:
        deserialize_float_over_pointer(pBuffer, zBUfferLength, pType, REINTERPRET_CAST(unsigned long *, ulNumber));
        break;
#endif
    case WIDTH_64:
        if (sizeof(ulNumber) * CHAR_BIT < WIDTH_64) {
            deserialize_float_over_pointer(pBuffer, zBUfferLength, pType, REINTERPRET_CAST(unsigned long *, ulNumber));
            break;
        }
    case WIDTH_32:
    case WIDTH_16:
        deserialize_float_inlined(pBuffer, zBUfferLength, pType, ulNumber);
        break;
    }
}

static void
deserialize_number(char *szLocation, char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long ulNumber) {

    ASSERT(szLocation);
    ASSERT(pBuffer);
    ASSERT(zBUfferLength > 0);
    ASSERT(pType);

    switch (pType->mTypeKind) {
    case KIND_INTEGER:
        if (ISSET(pType->mTypeInfo, NUMBER_SIGNED_BIT)) {
            longest L = llli_get_number(szLocation, pType, ulNumber);
            deserialize_number_signed(pBuffer, zBUfferLength, pType, L);
        } else {
            ulongest UL = lllu_get_number(szLocation, pType, ulNumber);
            deserialize_number_unsigned(pBuffer, zBUfferLength, pType, UL);
        }
        break;
    case KIND_FLOAT:
        deserialize_number_float(szLocation, pBuffer, zBUfferLength, pType, ulNumber);
        break;
    case KIND_UNKNOWN:
        report(true, "UBSAN: Unknown Type in %s\n", szLocation);
        /* NOTREACHED */
        break;
    }
}

static const char *
deserialize_type_check_kind(uint8_t hhuTypeCheckKind) {
    const char *rgczTypeCheckKinds[] = {
            "load of",
            "store to",
            "reference binding to",
            "member access within",
            "member call on",
            "constructor call on",
            "downcast of",
            "downcast of",
            "upcast of",
            "cast to virtual base of",
            "_Nonnull binding to",
            "dynamic operation on"};

    ASSERT(__arraycount(rgczTypeCheckKinds) > hhuTypeCheckKind);

    return rgczTypeCheckKinds[hhuTypeCheckKind];
}

static const char *
deserialize_builtin_check_kind(uint8_t hhuBuiltinCheckKind) {
    const char *rgczBuiltinCheckKinds[] = {
            "ctz()",
            "clz()"};

    ASSERT(__arraycount(rgczBuiltinCheckKinds) > hhuBuiltinCheckKind);

    return rgczBuiltinCheckKinds[hhuBuiltinCheckKind];
}

static const char *
deserialize_cfi_check_kind(uint8_t hhuCFICheckKind) {
    const char *rgczCFICheckKinds[] = {
            "virtual call",                                /* CFI_VCALL */
            "non-virtual call",                            /* CFI_NVCALL */
            "base-to-derived cast",                        /* CFI_DERIVEDCAST */
            "cast to unrelated type",                      /* CFI_UNRELATEDCAST */
            "indirect function call",                      /* CFI_ICALL */
            "non-virtual pointer to member function call", /* CFI_NVMFCALL */
            "virtual pointer to member function call",     /* CFI_VMFCALL */
    };

    ASSERT(__arraycount(rgczCFICheckKinds) > hhuCFICheckKind);

    return rgczCFICheckKinds[hhuCFICheckKind];
}

static const char *
deserialize_implicit_conversion_check_kind(uint8_t hhuImplicitConversionCheckKind) {
    const char *rgczImplicitConversionCheckKind[] = {
            "integer truncation",
            "unsigned integer truncation",
            "signed integer truncation",
            "integer sign change",
            "signed integer trunctation or sign change",
    };

    ASSERT(__arraycount(rgczImplicitConversionCheckKind) > hhuImplicitConversionCheckKind);

    return rgczImplicitConversionCheckKind[hhuImplicitConversionCheckKind];
}

static bool
is_negative_number(char *szLocation, struct CTypeDescriptor *pType, unsigned long ulNumber) {

    ASSERT(szLocation);
    ASSERT(pType);
    ASSERT(pType->mTypeKind == KIND_INTEGER);

    if (!ISSET(pType->mTypeInfo, NUMBER_SIGNED_BIT))
        return false;

    return llli_get_number(szLocation, pType, ulNumber) < 0;
}

static bool
is_shift_exponent_too_large(char *szLocation, struct CTypeDescriptor *pType, unsigned long ulNumber, size_t zWidth) {

    ASSERT(szLocation);
    ASSERT(pType);
    ASSERT(pType->mTypeKind == KIND_INTEGER);

    return lllu_get_number(szLocation, pType, ulNumber) >= zWidth;
}
