#ifndef JOS_INC_TYPES_H
#define JOS_INC_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uintptr_t physaddr_t;
typedef ptrdiff_t ssize_t;
typedef int32_t off_t;

/* Efficient min and max operations */
#ifndef MIN
#define MIN(_a, _b) ({      \
    typeof(_a) __a = (_a);  \
    typeof(_b) __b = (_b);  \
    __a <= __b ? __a : __b; })
#endif

#ifndef MAX
#define MAX(_a, _b) ({      \
    typeof(_a) __a = (_a);  \
    typeof(_b) __b = (_b);  \
    __a >= __b ? __a : __b; })
#endif

/* Rounding operations (efficient when n is a power of 2) */

/* Round down to the nearest multiple of n */
#define ROUNDDOWN(a, n) ({        \
    uint64_t __a = (uint64_t)(a); \
    (typeof(a))(__a - __a % (n)); })

/* Round up to the nearest multiple of n */
#define ROUNDUP(a, n) ({                                  \
    uint64_t __n = (uint64_t)(n);                         \
    (typeof(a))(ROUNDDOWN((uint64_t)(a) + __n - 1, __n)); })

/* Round up to the nearest multiple of n */
#define CEILDIV(a, n) ({                        \
    uint64_t __n = (uint64_t)(n);               \
    (typeof(a))(((uint64_t)(a) + __n - 1)/__n); })

#endif /* !JOS_INC_TYPES_H */
