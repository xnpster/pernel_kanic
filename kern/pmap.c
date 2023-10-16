/* See COPYRIGHT for copyright information. */

#include <inc/assert.h>
#include <inc/error.h>
#include <inc/mmu.h>
#include <inc/string.h>
#include <inc/uefi.h>
#include <inc/x86.h>

#include <kern/env.h>
#include <kern/kclock.h>
#include <kern/pmap.h>
#include <kern/traceopt.h>
#include <kern/trap.h>

/*
 * Term "page" used here does not
 * refer to real x86 page but rather
 * some memory region of size 2^N that
 * is aligned on 2^N that is described
 * by struct Page
 */

/* for O(1) page allocation */
static struct List free_classes[MAX_CLASS];
/* List of descriptor pools */
static struct PagePool *first_pool;
/* List of free descriptors */
static struct List free_descriptors;
static size_t free_desc_count;
/* Physical memory size */
size_t max_memory_map_addr;
/* Kernel address space */
struct AddressSpace kspace;
/* Currently active address spcae */
struct AddressSpace *current_space;
/* Root node of physical memory tree */
struct Page root;
/* Top address for page pools mappings */
static uintptr_t metaheaptop;

/* Kernel executable end virtual address */
extern char end[];
extern char pfstacktop[], pfstack[];

/* Those are internal flags for map_page function */
#define ALLOC_POOL 0x10000
/* Allocate but don't remove from free lists */
#define ALLOC_WEAK 0x20000
/* Allocate page within [0; BOOT_MEM_SIZE) */
#define ALLOC_BOOTMEM 0x40000

/* Descriptor pool page size */
#define POOL_CLASS 1

#define LOOKUP_SPLIT    2
#define LOOKUP_ALLOC    1
#define LOOKUP_PRESERVE 0

#define PAGE_IS_FREE(p) (!(p)->refc && !(p)->left && !(p)->right)
#define PAGE_IS_UNIQ(p) ((p)->refc == 1 && !(p)->left && !(p)->right)

#define INIT_DESCR 256

#define ABSDIFF(x, y) ((x) > (y) ? (x) - (y) : (y) - (x))

#define assert_physical(n) ({ if (trace_memory_more) _assert_root(__FILE__, __LINE__, n, 1); assert(((n)->state & NODE_TYPE_MASK) >= PARTIAL_NODE); })
#define assert_virtual(n)  ({if (trace_memory_more) _assert_root(__FILE__, __LINE__, n, 0); assert(((n)->state & NODE_TYPE_MASK) < PARTIAL_NODE); })

inline static bool __attribute__((always_inline))
list_empty(struct List *list) {
    return list->next == list;
}

inline static void __attribute__((always_inline))
list_init(struct List *list) {
    list->next = list->prev = list;
}

/*
 * Appends list element 'new' after list element 'list'
 */
inline static void __attribute__((always_inline))
list_append(struct List *list, struct List *new) {
    // LAB 6: Your code here
}

/*
 * Deletes list element from list.
 * NOTE: Use list_init() on deleted List element
 */
inline static struct List *__attribute__((always_inline))
list_del(struct List *list) {
    // LAB 6: Your code here

    return list;
}

static struct Page *alloc_page(int class, int flags);

void
ensure_free_desc(size_t count) {
    if (free_desc_count < count) {
        struct Page *res = alloc_page(POOL_CLASS, ALLOC_POOL);
        (void)res;
        if (!res) panic("Out of memory\n");
    }

    assert(free_desc_count >= count);
    assert(!list_empty(&free_descriptors));
}

static struct Page *
alloc_descriptor(enum PageState state) {
    ensure_free_desc(1);

    struct Page *new = (struct Page *)list_del(free_descriptors.next);

    memset(new, 0, sizeof *new);
    list_init((struct List *)new);
    new->state = state;
    free_desc_count--;

    return new;
}

static void
free_descriptor(struct Page *page) {
    list_del((struct List *)page);
    list_append(&free_descriptors, (struct List *)page);
    free_desc_count++;
}

static void
_assert_root(const char *file, int line, struct Page *p, bool phy) {
    while (p->parent) p = p->parent;
    if ((p == &root) != phy)
        _panic(file, line, "Page %p (phy %p) should%s be physical\n", p, (void *)PADDR(p), phy ? "" : "n't");
}

static void
free_desc_rec(struct Page *p) {
    while (p) {
        assert(!p->refc);
        free_desc_rec(p->right);
        struct Page *tmp = p->left;
        free_descriptor(p);
        p = tmp;
    }
}

/*
 * This function allocates child
 * node for given parent in physical memory tree
 * right indicated whether left or right child should
 * be allocated.
 *
 * New child describes lower (for the left one)
 * or higher (for the right one) half of memory
 * described by parent node.
 * The memory have the same type as for parent.
 *
 * NOTE: Be careful with overflows
 * NOTE: Child node should have it's
 * reference counter to be equal either 0 or 1
 * depending on whether parent's refc is 0 or non-zero,
 * correspondingly.
 * HINT: Use alloc_descriptor() here
 */
static struct Page *
alloc_child(struct Page *parent, bool right) {
    assert_physical(parent);
    assert(parent);

    // LAB 6: Your code here

    struct Page *new = NULL;

    return new;
}


/* Lookup physical memory node with given address and class */
static struct Page *
page_lookup(struct Page *hint, uintptr_t addr, int class, enum PageState type, bool alloc) {
    assert(class >= 0);
    if (hint) assert_physical(hint);

    struct Page *node = hint ? hint : &root;
    assert(!(addr & CLASS_MASK(class)));
    assert(node);

    while (node && node->class > class) {
        assert(class >= 0);
        bool right = addr & CLASS_SIZE(node->class - 1);

        if (alloc) {
            ensure_free_desc((node->class - class + 1) * 2);
            bool was_free = node->state == ALLOCATABLE_NODE && PAGE_IS_FREE(node);
            if (!node->left) alloc_child(node, 0);
            if (!node->right) alloc_child(node, 1);

            if (was_free) {
                /* Recalculate free lists for allocatable page */
                struct Page *other = !right ? node->right : node->left;
                assert(other->state == ALLOCATABLE_NODE);
                list_del((struct List *)node);
                list_append(&free_classes[node->class - 1], (struct List *)other);
            }

            if (type != PARTIAL_NODE && node->state != type)
                node->state = PARTIAL_NODE;
        }

        assert((node->left && node->right) || !alloc);

        node = right ? node->right : node->left;
    }

    if (alloc) assert(node);
    if (alloc && type != PARTIAL_NODE) /* Attach new memory */ {
        /* Cannot attach mapped memory */
        assert(!node->refc);

        /* Need to free old subtree when retyping memory */
        free_desc_rec(node->left);
        free_desc_rec(node->right);
        node->left = node->right = NULL;
        list_del((struct List *)node);

        /* We cannot change RESERVED_NODE memory to ALLOCATABLE_NODE */
        if (type != PARTIAL_NODE && node->state != RESERVED_NODE) node->state = type;
        if (node->state == ALLOCATABLE_NODE) list_append(&free_classes[node->class], (struct List *)node);

        if (trace_memory) cprintf("Attaching page (%x) at %p class=%d\n", node->state, (void *)page2pa(node), (int)node->class);
    }

    if (node) assert(!(page2pa(node) & CLASS_MASK(node->class)));

    return node;
}

static void
page_ref(struct Page *node) {
    if (!node) return;

    /* If parent is allocated
     * all of its children are allocated too,
     * so need to reference them recursively
     * when refc transitions from 0 to 1 */
    if (!node->refc++) {
        list_del((struct List *)node);
        list_init((struct List *)node);
        page_ref(node->left);
        page_ref(node->right);
    }
}

static void
page_unref(struct Page *page) {
    if (!page) return;
    assert_physical(page);
    assert(page->refc);

    /* NOTE Decrementing refc after
     * this if statement is important
     * to prevent double frees */

    if (page->refc == 1) {
        page_unref(page->left);
        page_unref(page->right);
    }

    page->refc--;

    /* Try to merge free page with adjacent */
    if (PAGE_IS_FREE(page)) {
        while (page != &root) {
            struct Page *par = page->parent;
            assert_physical(par);
            if (par->state == page->state &&
                PAGE_IS_FREE(par->left) &&
                PAGE_IS_FREE(par->right)) {
                free_descriptor(par->left);
                par->left = NULL;

                free_descriptor(par->right);
                par->right = NULL;

                if (par->state == ALLOCATABLE_NODE) {
                    assert(list_empty((struct List *)par));
                    list_append(&free_classes[par->class], (struct List *)par);
                }
                page = par;
            } else
                break;
        }
        list_del((struct List *)page);
        if (page->state == ALLOCATABLE_NODE)
            list_append(&free_classes[page->class], (struct List *)page);
    }
}

static void
attach_region(uintptr_t start, uintptr_t end, enum PageState type) {
    if (trace_memory_more)
        cprintf("Attaching memory region [%08lX, %08lX] with type %d\n", start, end - 1, type);
    int class = 0, res = 0;

    (void)class;
    (void)res;

    start = ROUNDDOWN(start, CLASS_SIZE(0));
    end = ROUNDUP(end, CLASS_SIZE(0));

    // LAB 6: Your code here
}

/*
 * Helper function for dumping single page table
 * entry (base) describing element with given size (step)
 * isz indicates if given entry is a leaf node
 */
inline static void
dump_entry(pte_t base, size_t step, bool isz) {
    cprintf("%s[%08llX, %08llX] %c%c%c%c%c -- step=%zx\n",
            step == 4 * KB ? " >    >    >    >" :
            step == 2 * MB ? " >    >    >" :
            step == 1 * GB ? " >    >" :
                             " >",
            PTE_ADDR(base),
            PTE_ADDR(base) + (isz ? (step * isz - 1) : 0xFFF),
            base & PTE_P ? 'P' : '-',
            base & PTE_U ? 'U' : '-',
            base & PTE_W ? 'W' : '-',
            base & PTE_NX ? '-' : 'X',
            base & PTE_PS ? 'S' : '-',
            step);
}

static void
check_physical_tree(struct Page *page) {
    assert_physical(page);
    assert(page->class >= 0);
    assert(!(page2pa(page) & CLASS_MASK(page->class)));
    if (page->state == ALLOCATABLE_NODE || page->state == RESERVED_NODE) {
        if (page->left) assert(page->left->state == page->state);
        if (page->right) assert(page->right->state == page->state);
    }
    if (page->left) {
        assert(page->left->class + 1 == page->class);
        assert(page2pa(page) == page2pa(page->left));
    }
    if (page->right) {
        assert(page->right->class + 1 == page->class);
        assert(page->addr + (1ULL << (page->class - 1)) == page->right->addr);
    }
    if (page->parent) {
        assert(page->parent->class - 1 == page->class);
        assert((page->parent->left == page) ^ (page->parent->right == page));
    } else {
        assert(page->class == MAX_CLASS);
        assert(page == &root);
    }
    if (!page->refc) {
        assert(page->head.next && page->head.prev);
        if (!list_empty((struct List *)page)) {
            for (struct List *n = page->head.next;
                 n != &free_classes[page->class]; n = n->next) {
                assert(n != &page->head);
            }
        }
    } else {
        for (struct List *n = (struct List *)page->head.next;
             (struct List *)page != n; n = n->next) {
            struct Page *v = (struct Page *)n;
            assert_virtual(v);
            assert(v->phy == page);
        }
    }
    if (page->left) {
        assert(page->left->parent == page);
        check_physical_tree(page->left);
    }
    if (page->right) {
        assert(page->right->parent == page);
        check_physical_tree(page->right);
    }
}

static void
check_virtual_tree(struct Page *page, int class) {
    assert(class >= 0);
    assert_virtual(page);
    if ((page->state & NODE_TYPE_MASK) == MAPPING_NODE) {
        assert(page->phy);
        assert(!(page->state & PROT_LAZY) || !(page->state & PROT_SHARE));
        assert(!page->left && !page->right);
        assert(page->phy);
        if (!(page->phy->class == class)) cprintf("%d %d\n", page->phy->class, class);
        assert(page->phy->class == class);
    } else {
        assert(!page->phy);
        assert(page->state == INTERMEDIATE_NODE);
    }
    if (page->left) {
        assert(page->left->parent == page);
        check_virtual_tree(page->left, class - 1);
    }
    if (page->right) {
        assert(page->right->parent == page);
        check_virtual_tree(page->right, class - 1);
    }
}

/*
 * Pretty-print virtual memory tree
 */
void
dump_virtual_tree(struct Page *node, int class) {
    // LAB 7: Your code here
}

void
dump_memory_lists(void) {
    // LAB 6: Your code here
}


/*
 * Pretty-print page table
 * You can read about the page table
 * structure in the LAB 7 description
 * NOTE: Use dump_entry().
 * NOTE: Don't forget about PTE_PS
 */
void
dump_page_table(pte_t *pml4) {
    uintptr_t addr = 0;
    cprintf("Page table:\n");
    // LAB 7: Your code here
    (void)addr;
}

/* Just allocate page, without mapping it */
static struct Page *
alloc_page(int class, int flags) {
    struct List *li = NULL;
    struct Page *peer = NULL;

    if (flags & ALLOC_POOL) flags |= ALLOC_BOOTMEM;

    /* Find page that is not smaller than requested
     * (Pool memory should also be within BOOT_MEM_SIZE) */
    for (int pclass = class; pclass < MAX_CLASS; pclass++, li = NULL) {
        for (li = free_classes[pclass].next; li != &free_classes[pclass]; li = li->next) {
            peer = (struct Page *)li;
            assert(peer->state == ALLOCATABLE_NODE);
            assert_physical(peer);
            if (!(flags & ALLOC_BOOTMEM) || page2pa(peer) + CLASS_SIZE(class) < BOOT_MEM_SIZE) goto found;
        }
    }
    return NULL;

found:
    list_del(li);

    size_t ndesc = 0;
    static bool allocating_pool;
    if (flags & ALLOC_POOL) {
        assert(!allocating_pool);
        allocating_pool = 1;

        struct PagePool *newpool = KADDR(page2pa(peer));
        ndesc = POOL_ENTRIES_FOR_SIZE(CLASS_SIZE(class));
        for (size_t i = 0; i < ndesc; i++)
            list_append(&free_descriptors, (struct List *)&newpool->data[i]);
        newpool->next = first_pool;
        first_pool = newpool;
        free_desc_count += ndesc;
        if (trace_memory_more) cprintf("Allocated pool of size %zu at [%08lX, %08lX]\n",
                                       ndesc, page2pa(peer), page2pa(peer) + (long)CLASS_MASK(class));
    }

    struct Page *new = page_lookup(peer, page2pa(peer), class, PARTIAL_NODE, 1);
    assert(!new->refc);

    if (flags & ALLOC_POOL) {
        assert(KADDR(page2pa(new)) == first_pool);
        page_ref(new);
        first_pool->peer = new;
        allocating_pool = 0;
    } else {
        if (trace_memory_more) cprintf("Allocated page at [%08lX, %08lX] class=%d\n",
                                       page2pa(new), page2pa(new) + (long)CLASS_MASK(new->class), new->class);
    }

    assert(page2pa(new) >= PADDR(end) || page2pa(new) + CLASS_MASK(new->class) < IOPHYSMEM);

    return new;
}

static struct Page *zero_page, *one_page;

/* Buffers for filler pages are statically allocated for simplicity
 * (this is also required for early KASAN) */
__attribute__((aligned(HUGE_PAGE_SIZE))) uint8_t zero_page_raw[HUGE_PAGE_SIZE];
__attribute__((aligned(HUGE_PAGE_SIZE))) uint8_t one_page_raw[HUGE_PAGE_SIZE];


/*
 * This function initialized physical memory tree
 * with either UEFI memory map or CMOS contents.
 * Every region is inserted into the tree using
 * attach_region() function.
 */
static void
detect_memory(void) {
    size_t basemem, extmem;

    /* Attach reserved regions */

    /* Attach first page as reserved memory */
    // LAB 6: Your code here

    /* Attach kernel and old IO memory
     * (from IOPHYSMEM to the physical address of end label. end points the the
     *  end of kernel executable image.)*/
    // LAB 6: Your code here

    /* Detect memory via ether UEFI or CMOS */
    if (uefi_lp && uefi_lp->MemoryMap) {
        EFI_MEMORY_DESCRIPTOR *start = (void *)uefi_lp->MemoryMap;
        EFI_MEMORY_DESCRIPTOR *end = (void *)(uefi_lp->MemoryMap + uefi_lp->MemoryMapSize);
        size_t total_mem = 0;

        while (start < end) {
            enum PageState type;
            switch (start->Type) {
            case EFI_LOADER_CODE:
            case EFI_LOADER_DATA:
            case EFI_BOOT_SERVICES_CODE:
            case EFI_BOOT_SERVICES_DATA:
            case EFI_CONVENTIONAL_MEMORY:
                type = start->Attribute & EFI_MEMORY_WB ? ALLOCATABLE_NODE : RESERVED_NODE;
                total_mem += start->NumberOfPages * EFI_PAGE_SIZE;
                break;
            default:
                type = RESERVED_NODE;
            }

            max_memory_map_addr = MAX(start->NumberOfPages * EFI_PAGE_SIZE + start->PhysicalStart, max_memory_map_addr);

            /* Attach memory described by memory map entry described by start
             * of type type*/
            // LAB 6: Your code here
            (void)type;

            start = (void *)((uint8_t *)start + uefi_lp->MemoryMapDescriptorSize);
        }

        basemem = MIN(total_mem, IOPHYSMEM);
        extmem = total_mem - basemem;
    } else {
        basemem = cmos_read16(CMOS_BASELO) * KB;
        extmem = cmos_read16(CMOS_EXTLO) * KB;

        size_t pextmem = (size_t)cmos_read16(CMOS_PEXTLO) * KB * 64;
        if (pextmem) extmem = (16 * MB + pextmem - MB);

        max_memory_map_addr = extmem ? EXTPHYSMEM + extmem : basemem;

        attach_region(0, max_memory_map_addr, ALLOCATABLE_NODE);
    }

    if (trace_init) {
        cprintf("Physical memory: %zuM available, base = %zuK, extended = %zuK\n",
                (size_t)((basemem + extmem) / MB), (size_t)(basemem / KB), (size_t)(extmem / KB));
    }

    check_physical_tree(&root);

    /* Setup constant one/zero pages */
    one_page = page_lookup(NULL, PADDR(one_page_raw), MAX_ALLOCATION_CLASS, PARTIAL_NODE, 1);
    page_ref(one_page);

    zero_page = page_lookup(NULL, PADDR(zero_page_raw), MAX_ALLOCATION_CLASS, PARTIAL_NODE, 1);
    page_ref(zero_page);

    assert(zero_page && one_page);
    assert(!((uintptr_t)one_page_raw & (HUGE_PAGE_SIZE - 1)));
    assert(!((uintptr_t)zero_page_raw & (HUGE_PAGE_SIZE - 1)));
}

static void
init_allocator(void) {
    static struct Page initial_buffer[INIT_DESCR];

    metaheaptop = KERN_HEAP_START + ROUNDUP(uefi_lp->FrameBufferSize, PAGE_SIZE);

    /* Initialize lists */
    for (size_t i = 0; i < MAX_CLASS; i++)
        list_init(&free_classes[i]);

    /* Initialize first pool */

    if (trace_memory_more) cprintf("First pool at [%08lX, %08lX]\n", PADDR(initial_buffer),
                                   PADDR(initial_buffer) + INIT_DESCR * sizeof(struct Page));

    list_init(&free_descriptors);
    free_desc_count = INIT_DESCR;
    for (size_t i = 0; i < INIT_DESCR; i++)
        list_append(&free_descriptors, (struct List *)&initial_buffer[i]);

    list_init(&root.head);
    root.class = MAX_CLASS;
    root.state = PARTIAL_NODE;
}

void
init_memory(void) {

    init_allocator();
    if (trace_init) cprintf("Memory allocator is initialized\n");

    detect_memory();
    check_physical_tree(&root);
    if (trace_init) cprintf("Physical memory tree is correct\n");
}
