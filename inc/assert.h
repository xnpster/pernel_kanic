/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_ASSERT_H
#define JOS_INC_ASSERT_H

#include <inc/stdio.h>

void _warn(const char*, int, const char*, ...) __attribute__((format(printf, 3, 4)));
_Noreturn void _panic(const char*, int, const char*, ...) __attribute__((format(printf, 3, 4)));

#define warn(...)  _warn(__FILE__, __LINE__, __VA_ARGS__)
#define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)

#define assert(x)                              \
    do {                                       \
        if (!(x))                              \
            panic("assertion failed: %s", #x); \
    } while (0)

#define static_assert _Static_assert

#endif /* !JOS_INC_ASSERT_H */
