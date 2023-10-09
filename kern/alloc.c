#include <inc/types.h>
#include <kern/alloc.h>
#include <inc/assert.h>
#include <kern/spinlock.h>

#define SPACE_SIZE 5 * 0x1000

static uint8_t space[SPACE_SIZE];

/* empty list to get started */
static Header base = {.next = (Header *)space, .prev = (Header *)space};
/* start of free list */
static Header *freep = NULL;

static void
check_list(void) {
    asm volatile("cli");
    Header *prevp = freep, *p = prevp->next;
    for (; p != freep; p = p->next) {
        if (prevp != p->prev) panic("Corrupted list.\n");
        prevp = p;
    }
    asm volatile("sti");
}

/* malloc: general-purpose storage allocator */
void *
test_alloc(uint8_t nbytes) {

    /* Make allocator thread-safe with the help of spin_lock/spin_unlock. */
    // LAB 5: Your code here:

    size_t nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;

    /* no free list yet */
    if (!freep) {
        Header *hd = (Header *)&space;

        hd->next = (Header *)&base;
        hd->prev = (Header *)&base;
        hd->size = (SPACE_SIZE - sizeof(Header)) / sizeof(Header);

        freep = &base;
    }

    check_list();

    for (Header *p = freep->next;; p = p->next) {
        /* big enough */
        if (p->size >= nunits) {
            freep = p->prev;
            /* exactly */
            if (p->size == nunits) {
                p->prev->next = p->next;
                p->next->prev = p->prev;
            } else { /* allocate tail end */
                p->size -= nunits;
                p += p->size;
                p->size = nunits;
            }
            return (void *)(p + 1);
        }

        /* wrapped around free list */
        if (p == freep) {
            return NULL;
        }
    }
}

/* free: put block ap in free list */
void
test_free(void *ap) {

    /* point to block header */
    Header *bp = (Header *)ap - 1;

    /* Make allocator thread-safe with the help of spin_lock/spin_unlock. */
    // LAB 5: Your code here

    /* freed block at start or end of arena */
    Header *p = freep;
    for (; !(bp > p && bp < p->next); p = p->next)
        if (p >= p->next && (bp > p || bp < p->next)) break;

    if (bp + bp->size == p->next && p + p->size == bp) /* join to both */ {
        p->size += bp->size + p->next->size;
        p->next->next->prev = p;
        p->next = p->next->next;
    } else if (bp + bp->size == p->next) /* join to upper nbr */ {
        bp->size += p->next->size;
        bp->next = p->next->next;
        bp->prev = p->next->prev;
        p->next->next->prev = bp;
        p->next = bp;
    } else if (p + p->size == bp) /* join to lower nbr */ {
        p->size += bp->size;
    } else {
        bp->next = p->next;
        bp->prev = p;
        p->next->prev = bp;
        p->next = bp;
    }
    freep = p;

    check_list();
}
