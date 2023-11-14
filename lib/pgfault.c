/* User-level page fault handler support.
 * Rather than register the C page fault handler directly with the
 * kernel as the page fault handler, we register the assembly language
 * wrapper in pfentry.S, which in turns calls the registered C
 * function. */

#include <inc/lib.h>

/* Assembly language pgfault entrypoint defined in lib/pfentry.S. */
extern void _pgfault_upcall(void);

#define MAX_PFHANDLER 8

/* Vector of currently set handlers. */
pf_handler_t *_pfhandler_vec[MAX_PFHANDLER];
/* Vector size */
static size_t _pfhandler_off = 0;
static bool _pfhandler_inititiallized = 0;

void
_handle_vectored_pagefault(struct UTrapframe *utf) {
    /* This trying to handle pagefault until
     * some handler returns 1, that indicates
     * successfully handled exception */
    for (size_t i = 0; i < _pfhandler_off; i++)
        if (_pfhandler_vec[i](utf)) return;

    /* Unhandled exceptions just cause panic */
    panic("Userspace page fault rip=%08lX va=%08lX err=%x\n",
          utf->utf_rip, utf->utf_fault_va, (int)utf->utf_err);
}

/* Set the page fault handler function.
 * If there isn't one yet, _pgfault_handler will be 0.
 * The first time we register a handler, we need to
 * allocate an exception stack (one page of memory with its top
 * at USER_EXCEPTION_STACK_TOP), and tell the kernel to call the assembly-language
 * _pgfault_upcall routine when a page fault occurs. */
int
add_pgfault_handler(pf_handler_t handler) {
    int res = 0;
    if (!_pfhandler_inititiallized) {
        /* First time through! */
        // LAB 9: Your code here:
        goto end;
        _pfhandler_inititiallized = 1;
    }

    for (size_t i = 0; i < _pfhandler_off; i++)
        if (handler == _pfhandler_vec[i]) return 0;

    if (_pfhandler_off == MAX_PFHANDLER)
        res = -E_INVAL;
    else
        _pfhandler_vec[_pfhandler_off++] = handler;

end:
    if (res < 0) panic("set_pgfault_handler: %i", res);
    return res;
}

void
remove_pgfault_handler(pf_handler_t handler) {
    assert(_pfhandler_inititiallized);
    for (size_t i = 0; i < _pfhandler_off; i++) {
        if (_pfhandler_vec[i] == handler) {
            memmove(_pfhandler_vec + i, _pfhandler_vec + i + 1, (_pfhandler_off - i - 1) * sizeof(*handler));
            _pfhandler_off--;
            return;
        }
    }
}
