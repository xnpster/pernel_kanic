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
 * is alligned on 2^N that is described
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

// TODO Test these properly via cpuid

/* Not-executable bit supported by page tables */
static bool nx_supported = 1;
/* 1GB pages are supported */
static bool has_1gb_pages = 1;

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
    // LAB 6: Your code here.

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

#if SANITIZE_SHADOW_BASE
        if (current_space) {
            platform_asan_poison(KADDR(page2pa(page)), CLASS_SIZE(page->class));
        }
#endif
    }
}

void
alloc_virtual_child(struct Page *parent, struct Page **dst) {
    assert_virtual(parent);
    assert(parent->phy && parent->phy->left && parent->phy->right);

    if ((*dst = alloc_descriptor(parent->state))) {
        (*dst)->parent = parent;
        (*dst)->phy = dst == &parent->left ? parent->phy->left : parent->phy->right;
        page_ref((*dst)->phy);
        list_append((struct List *)(*dst)->phy, (struct List *)(*dst));
    }
}

/*
 * This function attaches a new memory region
 * to the physical memory tree during the
 * initiallization of the memory manager.
 *
 * HINT: Use page_lookup() with alloc == 1 for
 * each page of this memory region. Try
 * using as huge memory pages as possible.
 *
 * Roughly speaking, this this function
 * should first iterate from smallest
 * page size class (0) to MAX_CLASS trying to get
 * aligned address. And then iterate from maximal
 * used class to minimal, allocating page of that
 * class it it does not extend beyond given region
 *
 * HINT: CLASS_MASK() and CLASS_SIZE() macros might be
 * useful here
 */
static void
check_virtual_class(struct Page *node, int class) {
    while (node->parent) class ++, node = node->parent;
    assert(class == MAX_CLASS);
}

/* Lookup virtual address space mapping node with given address and class */
static struct Page *
page_lookup_virtual(struct Page *node, uintptr_t addr, int class, int alloc) {
    assert(class >= 0);
    assert_virtual(node);


    int nclass = MAX_CLASS;
    while (nclass > class) {
        assert(nclass > 0);
        bool right = addr & CLASS_SIZE(nclass - 1);


        struct Page **next = right ? &node->right : &node->left;

        if (!*next) {
            if (!alloc) break;
            if (!node->phy && alloc == LOOKUP_SPLIT) break;
            ensure_free_desc((nclass - class + 1) * 2);

            assert(nclass);
            if (node->phy) {
                assert(nclass == node->phy->class);
                assert((node->state & NODE_TYPE_MASK) == MAPPING_NODE);

                struct Page *pleft = page_lookup(node->phy, page2pa(node->phy), node->phy->class - 1, PARTIAL_NODE, 1);
                if (!pleft) return NULL;

                assert(node->phy->left && node->phy->right);

                alloc_virtual_child(node, &node->left);
                if (!node->left) return NULL;
                alloc_virtual_child(node, &node->right);
                if (!node->right) return NULL;

                list_del((struct List *)node);
                page_unref(node->phy);
                node->phy = NULL;
                node->state = INTERMEDIATE_NODE;
            } else {
                assert(node->state == INTERMEDIATE_NODE);
                *next = alloc_descriptor(INTERMEDIATE_NODE);
                (*next)->parent = node;
            }
            assert(*next);
        }
        node = *next;
        nclass--;
    }

    if (node && (alloc == LOOKUP_ALLOC || (alloc == LOOKUP_SPLIT && node->phy)) && trace_memory_more) {
        check_virtual_class(node, class);
    }

    return node;
}

static void
attach_region(uintptr_t start, uintptr_t end, enum PageState type) {
    if (trace_memory_more) cprintf("Attaching memory region [%08lX, %08lX] with type %d\n", start, end - 1, type);
    int class = 0, res = 0;

    (void)class; (void)res;

    start = ROUNDDOWN(start, CLASS_SIZE(0));
    end = ROUNDUP(end, CLASS_SIZE(0));

    // LAB 6: Your code here
}

static void
unmap_page_remove(struct Page *node) {
    if (!node) return;
    assert_virtual(node);

    if (node->phy) {
        assert(!node->left && !node->right);
        assert((node->state & NODE_TYPE_MASK) == MAPPING_NODE);
        page_unref(node->phy);
    } else {
        assert((node->state & NODE_TYPE_MASK) == INTERMEDIATE_NODE);
        unmap_page_remove(node->left);
        unmap_page_remove(node->right);
    }

    if (node->parent) {
        *(node->parent->left == node ?
                  &node->parent->left :
                  &node->parent->right) = NULL;
    }

    free_descriptor(node);
}

static void
remove_pt(pte_t *pt, pte_t base, size_t step, uintptr_t i0, uintptr_t i1) {
    assert(step == 1 * GB || step == 2 * MB || step == 4 * KB || step == 512 * GB);
    for (size_t i = i0; i < i1; i++) {
        if (!(pt[i] & PTE_P)) continue;
        assert(!(pt[i] & PTE_PS) || (step == 1 * GB || step == 2 * MB));

        if (!(pt[i] & PTE_PS) && step > 4 * KB) {
            pte_t *pt2 = KADDR(PTE_ADDR(pt[i]));
            remove_pt(pt2, base, step / PT_ENTRY_COUNT, 0, PT_ENTRY_COUNT);
            page_unref(page_lookup(NULL, (uintptr_t)PADDR(pt2), 0, PARTIAL_NODE, 0));
        }

        pt[i] = 0;
    }
}

inline static pte_t
prot2pte(int flags) {
    assert(!(flags & PROT_LAZY) | !(flags & PROT_SHARE));
    pte_t res = PTE_P | (flags & (PTE_AVAIL | PTE_PCD | PTE_PWT));
    if (flags & PROT_W && !(flags & PROT_LAZY)) res |= PTE_W;
    if (!(flags & PROT_X) && nx_supported) res |= PTE_NX;
    if (flags & PROT_SHARE) res |= PTE_SHARE;
    if (flags & PROT_USER_) res |= PTE_U;
    return res;
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
 * You can read about page the table
 * structure in lab 7 description
 * (Use dump_entry())
 * NOTE: Don't forget about PTE_PS
 */
void
dump_page_table(pte_t *pml4) {
    uintptr_t addr = 0;
    cprintf("Page table:\n");
    (void)addr;
    // LAB 7: Your code here
}

inline static int
alloc_pt(pte_t *dst) {
    if (!(*dst & PTE_P) || (*dst & PTE_PS)) {
        struct Page *page = alloc_page(0, ALLOC_BOOTMEM);
        if (!page) return -E_NO_MEM;
#ifdef SANITIZE_SHADOW_BASE
        assert(page2pa(page) + CLASS_SIZE(page->class) <= BOOT_MEM_SIZE);
#endif
        assert(!page->refc);
        page_ref(page);
        *dst = page2pa(page) | PTE_U | PTE_W | PTE_P;

#ifdef SANITIZE_SHADOW_BASE
        if (current_space) platform_asan_unpoison(KADDR(page2pa(page)), CLASS_SIZE(0));
#endif
        memset(KADDR(page2pa(page)), 0, CLASS_SIZE(0));
    }
    return 0;
}

static void
propagate_one_pml4(struct AddressSpace *dst, struct AddressSpace *src) {
    /* Reference level 3 page tables */
    for (size_t i = NUSERPML4; i < PML4_ENTRY_COUNT; i++) {
        if (src->pml4[i] & PTE_P && i != PML4_INDEX(UVPT))
            page_ref(page_lookup(NULL, PTE_ADDR(src->pml4[i]), 0, PARTIAL_NODE, 0));
    }

    /* Dereference old level 3 page tables */
    for (size_t i = NUSERPML4; i < PML4_ENTRY_COUNT; i++) {
        if (dst->pml4[i] & PTE_P && i != PML4_INDEX(UVPT))
            page_ref(page_lookup(NULL, PTE_ADDR(dst->pml4[i]), 0, PARTIAL_NODE, 0));
    }

    pte_t uvpt = dst->pml4[PML4_INDEX(UVPT)];
    memcpy(dst->pml4 + NUSERPML4, src->pml4 + NUSERPML4,
           PAGE_SIZE - NUSERPML4 * sizeof(pml4e_t));
    dst->pml4[PML4_INDEX(UVPT)] = uvpt;
}

static void
propagate_pml4(struct AddressSpace *spc) {
    if (!current_space) return;

    if (spc != &kspace) propagate_one_pml4(&kspace, spc);
    for (size_t i = 0; i < NENV; i++) {
        if (envs[i].env_status != ENV_FREE && &envs[i].address_space != spc)
            propagate_one_pml4(&envs[i].address_space, spc);
    }
}

inline static int
alloc_fill_pt(pte_t *dst, pte_t base, size_t step, size_t i0, size_t i1) {
    assert(i0 != i1);
    bool need_recur = step > 1 * GB || (step == 1 * GB && !has_1gb_pages);
    if (!need_recur && step != 4 * KB) base |= PTE_PS;

    if (trace_memory_more) dump_entry(base, step, i1 - i0);

    for (size_t i = i0; i < i1; i++, base += step) {
        if (need_recur) {
            int res = alloc_pt(dst + i);
            if (res < 0) return res;
            res = alloc_fill_pt(dst + i, base, step / PT_ENTRY_COUNT, 0, PT_ENTRY_COUNT);
            if (res < 0) return res;
        } else {
            if ((PTE_ADDR(base) & (step - 1))) cprintf("%08lX %08lX\n", (long)PTE_ADDR(base), step);
            assert(!(PTE_ADDR(base) & (step - 1)));
            dst[i] = base;
        }
    }

    return 0;
}

/* Copy physical page contents to some virtual address
 *
 * To copy physical address you can use linear
 * physical memeory mapping to KERN_BASE_ADDR
 * (via KADDR).
 *
 * You need temporaly switch to dst address space and
 * restore previous address space afterwards.
 *
 * Don't forget to disable write protection, since
 * destination page might be read-only
 *
 * TIP: switch_address_space, nosan_memcpy, and set_wp are used here
 */
static void
memcpy_page(struct AddressSpace *dst, uintptr_t va, struct Page *page) {
    assert(current_space);
    assert(dst);

    // LAB 7: Your code here
}

static void
tlb_invalidate_range(struct AddressSpace *spc, uintptr_t start, uintptr_t end) {
    if (current_space == spc || !current_space) {
        /* If we need to invalidate a lot of memory, just flush whole cache */
        if (start - end > 512 * GB)
            lcr3(rcr3());
        else {
            while (start < end) {
                invlpg((void *)start);
                start += PAGE_SIZE;
            }
        }
    }
}

static void
unmap_page(struct AddressSpace *spc, uintptr_t addr, int class) {
    if (trace_memory) cprintf("<%p> Unmapping [%08lX, %08lX]\n",
                              spc, addr, addr + (long)CLASS_MASK(class));
    int res;
    assert(!(addr & CLASS_MASK(class)));

    struct Page *node = page_lookup_virtual(spc->root, addr, class, LOOKUP_ALLOC);
    if (node) unmap_page_remove(node);
    /* Disallow root node deallocation */
    if (node == spc->root)
        spc->root = alloc_descriptor(INTERMEDIATE_NODE);

    uintptr_t end = addr + CLASS_SIZE(class);
    uintptr_t inval_start = addr, inval_end = end;

    size_t pml4i0 = PML4_INDEX(addr), pml4i1 = PML4_INDEX(end);
    if (class >= 27) {
        remove_pt(spc->pml4, addr, 512 * GB, pml4i0, pml4i1);
        if (pml4i1 - 1 >= NUSERPML4) propagate_pml4(spc);
        goto finish;
    }

    if (!(spc->pml4[pml4i0] & PTE_P)) return;
    pdpe_t *pdp = KADDR(PTE_ADDR(spc->pml4[pml4i0]));

    size_t pdpi0 = PDP_INDEX(addr), pdpi1 = PDP_INDEX(end);

    /* Fixup index if pdpi0 == 511 and pdpi1 == 0 (and should be 512) */
    if (pdpi0 > pdpi1) pdpi1 = PDP_ENTRY_COUNT;

    /* Unmap 1*GB hardware page if size of virtual page
     * is >= than 1*GB */

    if (class >= 18) {
        remove_pt(pdp, addr, 1 * GB, pdpi0, pdpi1);
        goto finish;
    }

    /* If page is not present don't need to do anything */

    if (!(pdp[pdpi0] & PTE_P))
        return;
    /* otherwise we need to split 1*GB page hw page
     * into smaller 2*MB pages, allocting new page table level */
    else if (pdp[pdpi0] & PTE_PS) {
        pdpe_t old = pdp[pdpi0];
        res = alloc_pt(pdp + pdpi0);
        assert(!res);
        pde_t *pd = KADDR(PTE_ADDR(pdp[pdpi0]));
        res = alloc_fill_pt(pd, old & ~PTE_PS, 2 * MB, 0, PT_ENTRY_COUNT);
        inval_start = ROUNDDOWN(inval_start, 1 * GB);
        inval_end = ROUNDUP(inval_end, 1 * GB);
        assert(!res);
    }
    pde_t *pd = KADDR(PTE_ADDR(pdp[pdpi0]));
    (void)pd;


    /* Unmap 2 MB hw pages if requested virtual page size is larger than
     * or equal 2 MB.
     * Use remove_pt() here. remove_pt() can handle recusive removal.
     * TIP: this resembles closely unmapping code */

    // LAB 7: Your code here

    size_t pdi0 = 0, pdi1 = 0;

    /* Return if page is not present or
     * split 2*MB page into 4KB pages if required.
     * Then load apporopriate page table kernel virutal address
     * to the pt pointer.
     * Use alloc_pt(), alloc_fill_pt(), KADDR here
     * (just like is above for 1BG pages)
     */

    // LAB 7: Your code here

    (void)pdi1, (void)pdi0;
    pte_t *pt = NULL;

    /* Unmap 4KB hw pages */
    size_t pti0 = PT_INDEX(addr), pti1 = PT_INDEX(end);
    if (pti0 > pti1) pti1 = PT_ENTRY_COUNT;
    if (class >= 0) {
        remove_pt(pt, addr, 4 * KB, pti0, pti1);
        goto finish;
    }

    /* We cannot allocate less than a page */
    assert(0);

finish:
    tlb_invalidate_range(spc, inval_start, inval_end);
}

static int
map_page(struct AddressSpace *spc, uintptr_t addr, struct Page *page, int flags) {
    assert(!(flags & PROT_LAZY) | !(flags & PROT_SHARE));
    assert(page && spc);
    assert_physical(page);
    assert(!(addr & CLASS_MASK(page->class)));

    /* NOTE ALLOC_WEAK cannot be map()'ed/unmap()'ed
     * since it does not store any
     * metadata and only exits as a part of page table */

    if (!(flags & ALLOC_WEAK)) {
        page_ref(page);
        unmap_page(spc, addr, page->class);
        struct Page *mapping = page_lookup_virtual(spc->root, addr, page->class, LOOKUP_ALLOC);
        if (!mapping) return -E_NO_MEM;

        mapping->phy = page;
        mapping->state = (PAGE_PROT(flags) & ~PROT_COMBINE) | MAPPING_NODE;
        list_append((struct List *)page, (struct List *)mapping);
    }

    if (trace_memory) cprintf("<%p> Mapping [%08lX, %08lX] to [%08lX, %08lX] (class=%d flags=%x)\n", spc,
                              page2pa(page), page2pa(page) + (long)CLASS_SIZE(page->class) - 1,
                              addr, addr + (long)CLASS_SIZE(page->class) - 1, page->class, flags);

    /* Insert page into page table */

    uintptr_t end = addr + CLASS_SIZE(page->class);
    uintptr_t base = page2pa(page) | prot2pte(flags);
    assert(!(page2pa(page) & CLASS_MASK(page->class)));

    size_t pml4i0 = PML4_INDEX(addr), pml4i1 = PML4_INDEX(end);
    /* Fill PML4 range if page size is larger than 512GB */
    if (page->class >= 27) {
        int res = alloc_fill_pt(spc->pml4, base, 512 * GB, pml4i0, pml4i1);
        if (pml4i1 - 1 >= NUSERPML4) propagate_pml4(spc);
        return res;
    }

    /* Allocate empty pdp if required */
    if (!(spc->pml4[pml4i0] & PTE_P)) {
        if (alloc_pt(spc->pml4 + pml4i0) < 0) return -E_NO_MEM;
        if (pml4i0 >= NUSERPML4) propagate_pml4(spc);
    }
    assert(!(spc->pml4[pml4i0] & PTE_PS)); /* There's (yet) no support for 512GB pages in x86 arch */
    pdpe_t *pdp = KADDR(PTE_ADDR(spc->pml4[pml4i0]));

    /* If requested region is larger than or equal to 1GB (at least one whole PD) */

    size_t pdpi0 = PDP_INDEX(addr), pdpi1 = PDP_INDEX(end);
    /* Fixup index if pdpi0 == 511 and pdpi1 == 0 (and should be 512) */
    if (pdpi0 > pdpi1) pdpi1 = PDP_ENTRY_COUNT;
    /* Fill PDP range if page size is larger than 1GB */
    if (page->class >= 18) return alloc_fill_pt(pdp, base, 1 * GB, pdpi0, pdpi1);

    /* Allocate empty pd... */
    if (!(pdp[pdpi0] & PTE_P) && alloc_pt(pdp + pdpi0) < 0) return -E_NO_MEM;
    /* ...or split 1GB page into 2MB pages if required */
    else if (pdp[pdpi0] & PTE_PS) {
        pdpe_t old = pdp[pdpi0];
        if (alloc_pt(pdp + pdpi0) < 0) return -E_NO_MEM;
        pde_t *pd = KADDR(PTE_ADDR(pdp[pdpi0]));
        if (alloc_fill_pt(pd, old & ~PTE_PS, 2 * MB, 0, PT_ENTRY_COUNT) < 0) return -E_NO_MEM;
    }
    /* Calculate kernel virtual address of page directory */
    pde_t *pd = KADDR(PTE_ADDR(pdp[pdpi0]));


    /* If requested region is larger than or equal to 2MB (at least one whole PT) */

    /* Calculate indexes and fill PD range if page size is larger than 2MB */

    // LAB 7: Your code here

    (void)pd;
    size_t pdi0 = 0, pdi1 = 0;

    /* Allocate empty pt or split 2MB page into 4KB pages if required and
     * calculate virtual address into pt.
     * alloc_pt(), alloc_fill_pt() are used here.
     * TIP: Look at the code above doing the same thing for 1GB pages */

    // LAB 7: Your code here

    (void)pdi0, (void)pdi1;
    pte_t *pt = NULL;

    /* If requested region is larger than or equal to 4KB (at least one whole page) */

    size_t pti0 = PT_INDEX(addr), pti1 = PT_INDEX(end);
    if (pti0 > pti1) pti1 = PT_ENTRY_COUNT;
    /* Fill PT range if page size is larger than 4KB */
    if (page->class >= 0) return alloc_fill_pt(pt, base, 4 * KB, pti0, pti1);

    /* We cannot allocate less than a page */
    assert(0);
}

void
unmap_region(struct AddressSpace *dspace, uintptr_t dst, uintptr_t size) {
    int class = 0;

    uintptr_t start = ROUNDDOWN(dst, 1ULL << CLASS_BASE);
    uintptr_t end = ROUNDUP(dst + size, 1ULL << CLASS_BASE);

    for (; class < MAX_CLASS && start + CLASS_SIZE(class) <= end; class ++) {
        if (start & CLASS_SIZE(class)) {
            unmap_page(dspace, start, class);
            start += CLASS_SIZE(class);
        }
    }

    for (; class >= 0 && start < end; class --) {
        if (start + CLASS_SIZE(class) <= end) {
            unmap_page(dspace, start, class);
            start += CLASS_SIZE(class);
        }
    }
}

/* Just allocate page, without mapping it */
static struct Page *
alloc_page(int class, int flags) {
    struct List *li = NULL;
    struct Page *peer = NULL;

    if (flags & ALLOC_POOL) flags |= ALLOC_BOOTMEM;
#ifndef SANITIZE_SHADOW_BASE
    if (current_space) flags &= ~ALLOC_BOOTMEM;
#endif

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
#ifdef SANITIZE_SHADOW_BASE
        /* Need to unpoison early to initiallize lists inplace */
        if (current_space) platform_asan_unpoison(newpool, CLASS_SIZE(class));
#endif
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
#ifdef SANITIZE_SHADOW_BASE
        assert(page2pa(new) + CLASS_SIZE(new->class) <= BOOT_MEM_SIZE);
#endif
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

int
region_maxref(struct AddressSpace *spc, uintptr_t addr, size_t size) {
    uintptr_t start = ROUNDDOWN(addr, PAGE_SIZE);
    uintptr_t end = ROUNDUP(addr + size, PAGE_SIZE);
    int res = 0;
    while (start < end) {
        struct Page *page = page_lookup_virtual(spc->root, start, 0, LOOKUP_PRESERVE);
        if (page && page->phy) {
            res = MAX(res, page->phy->refc + (page->phy->left || page->phy->right));
            start += CLASS_SIZE(page->phy->class);
        } else
            start += CLASS_SIZE(0);
    }
    return res;
}

inline static int
addr_common_class(uintptr_t addr1, uintptr_t addr2) {
    assert(!((addr1 | addr2) & CLASS_MASK(0)));
    int res = 0;
    while (!((addr1 ^ addr2) & CLASS_SIZE(res)) && res < MAX_CLASS) res++;
    return res;
}


static int
map_physical_region(struct AddressSpace *dst, uintptr_t dstart, uintptr_t pstart, size_t size, int flags) {
    if (trace_memory) cprintf("Mapping physical region [%08lX, %08lX] to [%08lX, %08lX] (flags=%x)\n",
                              pstart, pstart + (long)size - 1, dstart, dstart + (long)size - 1, flags);
    assert(dstart > MAX_USER_ADDRESS || dst == &kspace);

    int class = 0, res;

    uintptr_t start = ROUNDDOWN(dstart, CLASS_SIZE(0));
    uintptr_t end = ROUNDUP(dstart + size, CLASS_SIZE(0));
    pstart = ROUNDDOWN(pstart, CLASS_SIZE(0));

#if SANITIZE_SHADOW_BASE
    if (!(flags & ALLOC_WEAK) && dst == &kspace &&
        current_space && dstart >= 512 * GB) {
        platform_asan_unpoison((void *)start, size);
    }
#endif

    int max_class = addr_common_class(dstart, pstart);
    for (; class < max_class && start + CLASS_SIZE(class) <= end; class ++) {
        if (start & CLASS_SIZE(class)) {
            struct Page *page = page_lookup(NULL, pstart, class, PARTIAL_NODE, 1);
            assert(page);
            if ((res = map_page(dst, start, page, flags)) < 0) return res;
            start += CLASS_SIZE(class);
            pstart += CLASS_SIZE(class);
        }
    }

    for (; class >= 0 && start < end; class --) {
        while (start + CLASS_SIZE(class) <= end) {
            struct Page *page = page_lookup(NULL, pstart, class, PARTIAL_NODE, 1);
            assert(page);
            if ((res = map_page(dst, start, page, flags)) < 0) return res;
            assert_virtual(dst->root);
            start += CLASS_SIZE(class);
            pstart += CLASS_SIZE(class);
        }
    }


    return 0;
}

/* Allocate page (possibly physically discontiguous) and map it to address space */
int
alloc_composite_page(struct AddressSpace *spc, uintptr_t addr, int class, int flags) {
    int res = -E_NO_MEM;

    assert(!(addr & CLASS_MASK(class)));

    struct Page *page = alloc_page(class, flags);
    if (page) {
        res = map_page(spc, addr, page, flags);
    } else if (class) {
        /* If bigger page is not found try
         * to compose page from smaller pages recursively */
        if ((res = alloc_composite_page(spc, addr, class - 1, flags)) < 0) return res;
        if ((res = alloc_composite_page(spc, addr + CLASS_SIZE(class - 1), class - 1, flags)) < 0)
            unmap_page(spc, addr, class - 1);
    }

    return res;
}

int
force_alloc_page(struct AddressSpace *spc, uintptr_t va, int maxclass) {
    int res = -E_FAULT;
    /* FIXME We need to propagate kernel PML4E
     * changes to every AddressSpace or just use KPTI
     * (now it's ok since kernel does not map huge chunks of memory (>= 512GB)
     * to higher part of address space after initiallization) */

    static_assert(!(MAX_USER_ADDRESS & (HUGE_PAGE_SIZE * 512 * 512 - 1)), "MAX_USER_ADDRESS should be alligned on 512GiB");

    /* If we are working with kernel addresses
     * kspace should be current */
    struct AddressSpace *old = NULL;
    assert(current_space);
    old = switch_address_space(spc = (va > MAX_USER_ADDRESS ? &kspace : spc));


    /* Lookup page mapping such that it's class it not larger than MAX_ALLOCATION_CLASS */
    struct Page *page;
    if (!(page = page_lookup_virtual(spc->root, va, maxclass, LOOKUP_SPLIT))) goto fault;
    if (!(page = page_lookup_virtual(spc->root, va, 0, LOOKUP_PRESERVE))) goto fault;
    if (!(page->state & PROT_LAZY)) goto fault;

    va &= ~CLASS_MASK(page->phy->class);

    if (PAGE_IS_UNIQ(page->phy)) {
        /* If we have the only reference to the page and
         * and its mapping to itself we can actually just
         * disable lazy flag and not bother copying */
        res = map_page(spc, va, page->phy, page->state & ~PROT_LAZY);
    } else {
        if (trace_memory) {
            cprintf("<%p> Allocating new page [%08lX, %08lX] flags=%x\n", spc,
                    va, va + (long)CLASS_MASK(page->phy->class), page->state & PROT_ALL & ~PROT_LAZY);
        }

        struct Page *phy = page->phy;
        page_ref(phy);
        res = alloc_composite_page(spc, va, phy->class, page->state & PROT_ALL & ~PROT_LAZY);
        if (!res) memcpy_page(spc, va, phy);
        page_unref(phy);
    }

fault:
    switch_address_space(old);

    if (res == -E_NO_MEM) {
        if (spc != &kspace) {
            struct Env *env = (void *)((uint8_t *)spc - offsetof(struct Env, address_space));
            env_destroy(env);
        } else
            panic("Out of memory\n");
    } else
        assert(!res || res == -E_FAULT);

    return res;
}

static int
do_map_page(struct AddressSpace *dspace, uintptr_t dst, struct AddressSpace *sspace, uintptr_t src, struct Page *phy, int oldflags, int flags) {
    int res;

    /* PROT_COMBINE simplifies fork implementation */
    if (flags & PROT_COMBINE) {
        if (oldflags & PROT_SHARE)
            flags &= oldflags;
        else
            flags &= oldflags | PROT_LAZY;
    }

    assert(!(oldflags & PROT_LAZY) | !(oldflags & PROT_SHARE));
    assert(!(flags & PROT_LAZY) | !(flags & PROT_SHARE));

    /* Cannot enable RWX if not copying and they were disabled */
    if (!(flags & PROT_LAZY) && ~oldflags &
                                        (PROT_R | PROT_W | PROT_X) & flags) return -E_INVAL;

    /*
     * There are several differently handled configurations of
     * oldflags and flags:
     *     PROT_LAZY and PROT_LAZY -- map with PROT_LAZY set
     *     PROT_LAZY and PROT_SHARE -- map copy with PROT_SHARE set
     *     PROT_LAZY and none -- map copy
     *     PROT_SHARE and PROT_LAZY -- map with PROT_SHARE set
     *     PROT_SHARE and PROT_SHARE -- map with PROT_SHARE set
     *     PROT_SHARE and none -- map
     *     none and PROT_LAZY -- map with PROT_LAZY set
     *     none and PROT_SHARE -- map with PROT_SHARE set
     *     none and none -- map
     * NOTE PROT_SHARE becomes active *after* this mapping finishes
     *      and not before
     */

    /* Lock page so it cannot be deallocated during copying/mapping */
    if (!(flags & PROT_LAZY) && (oldflags & PROT_LAZY)) {
        int class = phy->class;
        res = force_alloc_page(sspace, src, MAX_CLASS);
        if (res < 0 || (sspace == dspace && src == dst)) return res;

        struct Page *newv = page_lookup_virtual(sspace->root, src, class, LOOKUP_PRESERVE);
        check_virtual_class(newv, class);
        assert(newv && newv->phy);
        phy = newv->phy;
    }

    page_ref(phy);

    if (oldflags & PROT_SHARE && flags & PROT_LAZY)
        flags = (flags & ~PROT_LAZY) | PROT_SHARE;

    bool need_remap = (flags & PROT_LAZY) && (sspace != dspace || src != dst);

    res = map_page(dspace, dst, phy, flags);
    if (!res && need_remap) {
        /* If PROT_LAZY is enabled in destination,
         * it also needs to be enabled in source */
        res = map_page(sspace, src, phy, oldflags | PROT_LAZY);
    }

    page_unref(phy);
    return res;
}

/* Subtree consiting of one or more physical pages */
static int
do_map_subtree(struct AddressSpace *dspace, uintptr_t dst, struct AddressSpace *sspace, uintptr_t src, struct Page *vpage, int class, int flags) {
    check_virtual_class(vpage, class);
    int res = 0;
    while (!res && vpage) {
        assert(class >= 0);
        if (vpage->phy) {
            assert((vpage->state & NODE_TYPE_MASK) == MAPPING_NODE);
            return do_map_page(dspace, dst, sspace, src,
                               vpage->phy, vpage->state & PROT_ALL, flags);
        }
        assert(vpage->state == INTERMEDIATE_NODE);

        if (vpage->left && (res = do_map_subtree(dspace, dst,
                                                 sspace, src, vpage->left, class - 1, flags)) < 0) break;

        dst += CLASS_SIZE(class - 1);
        src += CLASS_SIZE(class - 1);
        vpage = vpage->right;
        class --;
    }
    return res;
}

static struct Page *zero_page, *one_page;

static int
do_map_region_one_page(struct AddressSpace *dspace, uintptr_t dst, struct AddressSpace *sspace, uintptr_t src, int class, int flags) {
    if (dspace == sspace && src != dst) assert(ABSDIFF(dst, src) >= CLASS_SIZE(class));

    int res = 0;
    if (flags & (ALLOC_ONE | ALLOC_ZERO)) {
        if (flags & PROT_SHARE) {
            /* Shared pages cannot be lazily allocated
             * So just allocate them and filled with 0's/FF's */
            res = alloc_composite_page(dspace, dst, class, flags & PROT_ALL & ~(PROT_LAZY | PROT_COMBINE));
            if (!res) {
                assert(current_space);
                assert(dspace);
                struct AddressSpace *old = switch_address_space(dspace);
                set_wp(0);
                nosan_memset((void *)dst, flags & ALLOC_ONE ? 0xFF : 0x00, CLASS_SIZE(class));
                set_wp(1);
                switch_address_space(old);
            }
        } else {
            /* MAP_ZERO and MAP_ONE ignore sspace and source and
             * use special 0x00/0xFF-filled pages */

            /* Get filler page of appropriate size */
            struct Page *cpage = flags & ALLOC_ONE ? one_page : zero_page;
            cpage = page_lookup(cpage, page2pa(cpage), MIN(class, MAX_ALLOCATION_CLASS), PARTIAL_NODE, 1);
            if (!cpage) return -E_NO_MEM;

            /* And then just map them */
            size_t size_inc = CLASS_SIZE(MIN(class, MAX_ALLOCATION_CLASS));
            uintptr_t end = dst + CLASS_SIZE(class);
            assert(CLASS_SIZE(cpage->class) == size_inc);
            assert(!(flags & PROT_SHARE));
            while (dst < end && !res) {
                res = map_page(dspace, dst, cpage, (flags & PROT_ALL & ~PROT_COMBINE) | PROT_LAZY);
                dst += size_inc;
            }
        }
    } else {
        struct Page *page1 = page_lookup_virtual(sspace->root, src, class, LOOKUP_ALLOC);
        assert(page1);
        if (page1->phy && page1->phy->class > class) {
            /* We need to split physical page if part of it is remapped */
            struct Page *page = page_lookup(page1->phy, src, class, PARTIAL_NODE, 1);
            return do_map_page(dspace, dst, sspace, src, page, page1->state & PROT_ALL, flags);
        } else {
            check_virtual_class(page1, class);
            /* If more than one physical page need to be mapped,
             * we should do it recursively */
            return do_map_subtree(dspace, dst, sspace, src, page1, class, flags);
        }
    }

    return res;
}

int
map_region(struct AddressSpace *dspace, uintptr_t dst, struct AddressSpace *sspace, uintptr_t src, uintptr_t size, int flags) {
    if (src & CLASS_MASK(0) || (!sspace && !(flags & (ALLOC_ZERO | ALLOC_ONE)))) return -E_INVAL;
    if (dst & CLASS_MASK(0) || !dspace) return -E_INVAL;
    if (size & CLASS_MASK(0) || !size) return -E_INVAL;

    /* FIXME This thing does not properly handle
     * remapping overlapping regions to higher addresses */
    assert(sspace != dspace || dst <= src || ABSDIFF(src, dst) >= size);

    uintptr_t end = dst + size;
    int max_class = addr_common_class(src, dst), class = 0, res;
    for (; class < max_class && dst + CLASS_SIZE(class) <= end; class ++) {
        if (dst & CLASS_SIZE(class)) {
            res = do_map_region_one_page(dspace, dst, sspace, src, class, flags);
            if (res < 0) return res;
            dst += CLASS_SIZE(class);
            src += CLASS_SIZE(class);
        }
    }

    for (; class >= 0 && dst < end; class --) {
        while (dst + CLASS_SIZE(class) <= end) {
            res = do_map_region_one_page(dspace, dst, sspace, src, class, flags);
            if (res < 0) return res;
            dst += CLASS_SIZE(class);
            src += CLASS_SIZE(class);
        }
    }

    return 0;
}

void
release_address_space(struct AddressSpace *space) {
    /* NOTE: This function should not be called for kspace */

    /* Manually unref level 3 kernel page tables */
    for (size_t i = NUSERPML4; i < PML4_ENTRY_COUNT; i++) {
        if (kspace.pml4[i] & PTE_P && i != UVPT_INDEX)
            page_unref(page_lookup(NULL, PTE_ADDR(kspace.pml4[i]), 0, PARTIAL_NODE, 0));
    }

    /* Unmap all memory from the space
     * (kernel is cheating and does not store
     *  metadata for upper part of address space (privileged)
     *  in tree and only in page tables for user address spaces,
     *  so unmapping is safe) */
    unmap_page(space, 0, MAX_CLASS);

    /* Also unmap PML4 itself since it is never deallocated by page_uname*/
    page_unref(page_lookup(NULL, space->cr3, 0, PARTIAL_NODE, 0));

    /* Zero-out metadata */
    memset(space, 0, sizeof *space);
}


/*
 * This function is used for switch address spaces
 *
 * Sets current_space to space
 * Loads new CR3 value with lcr3()
 *
 * TIP: Don't switch address space
 * if space == current_space for perfomance reasons
 * (Why it might me impactful?)
 *
 * Returns old address space
 */
struct AddressSpace *
switch_address_space(struct AddressSpace *space) {
    assert(space);
    ///LAB 7: Your code here
    return NULL;
}

/* Buffers for filler pages are statically allocated for simplicity
 * (this is also required for early KASAN) */
__attribute__((aligned(HUGE_PAGE_SIZE))) uint8_t zero_page_raw[HUGE_PAGE_SIZE];
__attribute__((aligned(HUGE_PAGE_SIZE))) uint8_t one_page_raw[HUGE_PAGE_SIZE];


/*
 * This function initialized phyisical memory tree
 * with either UEFI memroy map or CMOS contents.
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

    /* Detech memory via ether UEFI or CMOS */
    if (uefi_lp && uefi_lp->MemoryMap) {
        EFI_MEMORY_DESCRIPTOR *start = (void *)uefi_lp->MemoryMap;
        EFI_MEMORY_DESCRIPTOR *end = (void *)(uefi_lp->MemoryMap + uefi_lp->MemoryMapSize);
        while (start < end) {
            enum PageState type;
            switch (start->Type) {
            case EFI_LOADER_CODE:
            case EFI_LOADER_DATA:
            case EFI_BOOT_SERVICES_CODE:
            case EFI_BOOT_SERVICES_DATA:
            case EFI_CONVENTIONAL_MEMORY:
                type = start->Attribute & EFI_MEMORY_WB ? ALLOCATABLE_NODE : RESERVED_NODE;
                break;
            default:
                type = RESERVED_NODE;
            }

            max_memory_map_addr = MAX(start->NumberOfPages * EFI_PAGE_SIZE + start->PhysicalStart, max_memory_map_addr);

            /* Attach memory described by memory map entry described by start
             * of type type*/
            // LAB 6: Your code here



            start = (void *)((uint8_t *)start + uefi_lp->MemoryMapDescriptorSize);
        }

        basemem = MIN(max_memory_map_addr, IOPHYSMEM);
        extmem = max_memory_map_addr - basemem;
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

    /* Initiallize lists */
    for (size_t i = 0; i < MAX_CLASS; i++)
        list_init(&free_classes[i]);

    /* Initiallize first pool */

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

void *
kzalloc_region(size_t size) {
    assert(current_space);

    size = ROUNDUP(size, PAGE_SIZE);

    if (metaheaptop + size > KERN_HEAP_END) panic("Kernel heap overflow\n");

    uintptr_t res = metaheaptop;
    metaheaptop += size;

    int r = map_region(&kspace, res, NULL, 0, size, PROT_R | PROT_W | ALLOC_ZERO);
    if (r < 0) panic("kzalloc_region: %i\n", r);

#ifdef SANITIZE_SHADOW_BASE
    if (res + size >= SANITIZE_SHADOW_BASE) {
        cprintf("kzalloc_region: returning shadow memory page! Increase base address?\n");
        return NULL;
    }
    platform_asan_unpoison((void *)res, size);
#endif

    return (void *)res;
}

static uintptr_t prev_mmio;
void *
mmio_map_region(physaddr_t addr, size_t size) {
    assert(current_space == &kspace);
    uintptr_t start = ROUNDDOWN(addr, PAGE_SIZE);
    uintptr_t end = ROUNDUP(addr + size, PAGE_SIZE);

    prev_mmio = metaheaptop;
    metaheaptop += end - start;

    if (map_physical_region(&kspace, prev_mmio, start, end - start, PROT_R | PROT_W | PROT_CD) < 0)
        panic("Cannot map physical region at %p of size %zd", (void *)addr, size);

    return (void *)(prev_mmio + addr - start);
}

void *
mmio_remap_last_region(physaddr_t addr, void *oldva, size_t oldsz, size_t size) {
    uintptr_t start = ROUNDDOWN(addr, PAGE_SIZE);
    uintptr_t end = ROUNDUP(addr + size, PAGE_SIZE);

    if (prev_mmio + addr - start != (uintptr_t)oldva &&
        (prev_mmio + end - start != metaheaptop))
        panic("Trying to remap non-last MMIO region!\n");

    metaheaptop = prev_mmio;
    return mmio_map_region(addr, size);
}

static void
init_kspace(void) {
    struct Page *page = alloc_page(0, ALLOC_BOOTMEM);
    page_ref(page);
    kspace.pml4 = KADDR(page2pa(page));
    kspace.cr3 = page2pa(page);
    memset(kspace.pml4, 0, CLASS_SIZE(0));
    kspace.pml4[PML4_INDEX(UVPT)] = kspace.cr3 | PTE_P | PTE_U;
    kspace.root = alloc_descriptor(INTERMEDIATE_NODE);
}

#ifdef SANITIZE_SHADOW_BASE
static void
unpoison_meta(struct Page *node) {
    while (node) {
        /* Only unpoison physical pages within BOOT_MEM_SIZE */
        if (node->refc && page2pa(node) + CLASS_SIZE(node->class) < MIN(BOOT_MEM_SIZE, max_memory_map_addr)) {
            platform_asan_unpoison(KADDR(page2pa(node)), CLASS_SIZE(node->class));
            return;
        }

        if (node->left) unpoison_meta(node->left);
        node = node->right;
    }
}

static void
init_shadow_pre(void) {
    int res;

    /* Map shadow memory as filled with 0xFFs */
    res = map_region(&kspace, SANITIZE_SHADOW_BASE - 1 * GB, NULL, 0, MIN(SANITIZE_SHADOW_SIZE, max_memory_map_addr >> 3) + 1 * GB, PROT_R | PROT_W | ALLOC_ONE);
    assert(!res);

    /* Force allocation of first (at most) 128MB of shadow memory
     * which descriptors and page tables are allocated from */
    int prealloc_class = 0;
    size_t minpre = MIN(max_memory_map_addr, BOOT_MEM_SIZE);
    if (minpre & (minpre - 1)) minpre += minpre;
    while (CLASS_SIZE(prealloc_class) < minpre) prealloc_class++;
    res = alloc_composite_page(&kspace, SANITIZE_SHADOW_BASE, MAX(prealloc_class - 3, 0), PROT_R | PROT_W);
    assert(!res);

    res = alloc_composite_page(&kspace, SANITIZE_SHADOW_BASE - CLASS_SIZE(3), 3, PROT_R | PROT_W);
    assert(!res);

    EFI_MEMORY_DESCRIPTOR *mstart = (void *)uefi_lp->MemoryMap;
    EFI_MEMORY_DESCRIPTOR *mend = (void *)((uint8_t *)mstart + uefi_lp->MemoryMapSize);
    for (; mstart < mend; mstart = (void *)((uint8_t *)mstart + uefi_lp->MemoryMapDescriptorSize)) {
        if (mstart->Attribute & EFI_MEMORY_RUNTIME) {
            /* Map shadow memory for each runtime UEFI memory region */
            uintptr_t s_addr = ROUNDDOWN(SHADOW_ADDR(mstart->VirtualStart), PAGE_SIZE);
            uintptr_t s_end = ROUNDUP(SHADOW_ADDR(mstart->VirtualStart + mstart->NumberOfPages * PAGE_SIZE), PAGE_SIZE);
            res = map_region(&kspace, s_addr, NULL, 0, s_end - s_addr, PROT_R | PROT_W | ALLOC_ZERO);
            assert(!res);
        }
    }
}
#endif

void
init_memory(void) {
    int res;
    (void)res;

    init_allocator();
    if (trace_init) cprintf("Memory allocator is initiallized\n");

    detect_memory();
    check_physical_tree(&root);
    if (trace_init) cprintf("Physical memory tree is correct\n");

    init_kspace();

    /* First, only map kernel itself, kernel stacks, UEFI memory
     * and KASAN shadow memory regions to new kernel address space.
     * Allocated memory should not be touches until address space switch */

    /* Map physical memroy onto kernel address space weakly... */
    /* NOTE We cannot use map_region to map memory allocated with ALLOC_WEAK */

    // LAB 7: Your code here
    // NOTE: You need to check if map_physical_region returned 0 everywhere! (and panic otherwise)
    // Map [0, max_memory_map_addr] to [KERN_BASE_ADDR, KERN_BASE_ADDR + max_memory_map_addr] as RW- + ALLOC_WEAK


    extern char __text_end[], __text_start[];
    assert(!((uintptr_t)__text_start & CLASS_MASK(0)));
    assert(__text_end - __text_start < MAX_LOW_ADDR_KERN_SIZE);
    assert((uintptr_t)(end - KERN_BASE_ADDR) < MIN(BOOT_MEM_SIZE, max_memory_map_addr));

    /* ...and make kernel .text section executable: */

    // LAB 7: Your code here
    // Map [PADDR(__text_start);PADDR(__text_end)] to [__text_start, __text_end] as RW-


    /* Allocate kernel stacks */

    // LAB 7: Your code here
    // Map [PADDR(bootstack), PADDR(bootstack) + KERN_STACK_SIZE] to [KERN_STACK_TOP - KERN_STACK_SIZE, KERN_STACK_TOP] as RW-
    // Map [PADDR(pfstack), PADDR(pfstack) + KERN_PF_STACK_SIZE] to [KERN_PF_STACK_TOP - KERN_PF_STACK_SIZE, KERN_PF_STACK_TOP] as RW-

#ifdef SANITIZE_SHADOW_BASE
    init_shadow_pre();
#endif

    EFI_MEMORY_DESCRIPTOR *mstart = (void *)uefi_lp->MemoryMap;
    EFI_MEMORY_DESCRIPTOR *mend = (void *)((uint8_t *)mstart + uefi_lp->MemoryMapSize);
    for (; mstart < mend; mstart = (void *)((uint8_t *)mstart + uefi_lp->MemoryMapDescriptorSize)) {
        if (mstart->Attribute & EFI_MEMORY_RUNTIME) {
            // LAB 7: Your code here
            // Map [mstart->PhysicalStart, mstart->PhysicalStart+mstart->NumberOfPages*PAGE_SIZE] to
            //     [mstart->VirtualStart, mstart->VirtualStart+mstart->NumberOfPages*PAGE_SIZE] as RW-
        }
    }

    if (trace_memory_more) {
        cprintf("uefi_lp= %p %p\n", (void *)uefi_lp, (void *)uefi_lp->SelfVirtual);
    }

    /* Fixup loader params address after mapping it */
    uefi_lp = (LOADER_PARAMS *)uefi_lp->SelfVirtual;
    /* Set appropriate cr0 and cr4 bits
     * (In assembly code only minimal set of modes was set)*/
    lcr0(CR0_PE | CR0_PG | CR0_AM | CR0_WP | CR0_NE | CR0_MP);
    lcr4(CR4_PSE | CR4_PAE | CR4_PCE);

    /* Enable NX bit (execution protection) */
    uint64_t efer = rdmsr(EFER_MSR);
    efer |= EFER_NXE;
    wrmsr(EFER_MSR, efer);

    for (size_t i = 0; i < CLASS_SIZE(MAX_ALLOCATION_CLASS); i++) assert(!zero_page_raw[i]);

    switch_address_space(&kspace);

    /* One page is a page filled with 0xFF values -- ASAN poison */
    nosan_memset(one_page_raw, 0xFF, CLASS_SIZE(MAX_ALLOCATION_CLASS));

    /* Perform global constructor initialisation (e.g. asan)
    * This must be done as early as possible */
    extern void (*__ctors_start)();
    extern void (*__ctors_end)();
    void (**ctor)() = &__ctors_start;
    while (ctor < &__ctors_end) (*ctor++)();

#ifdef SANITIZE_SHADOW_BASE
    unpoison_meta(&root);
#endif

    /* Traps needs to be initiallized here
     * to alloc #PF to be handled during lazy allocation */
    trap_init();

    /* Check uefi memory mappings */
    mstart = (void *)uefi_lp->MemoryMapVirt;
    mend = (void *)((uint8_t *)mstart + uefi_lp->MemoryMapSize);
    for (; mstart < mend; mstart = (void *)((uint8_t *)mstart + uefi_lp->MemoryMapDescriptorSize)) {
        if (mstart->Attribute & EFI_MEMORY_RUNTIME) {
            int expected;
            nosan_memcpy(&expected, KADDR(mstart->PhysicalStart), sizeof(int));
            assert(*(volatile int *)mstart->VirtualStart == expected);
        }
    }
    /* Map the rest of memory regions after initiallizing shadow memory */

    // LAB 7: Your code here
    // Map [FRAMEBUFFER, FRAMEBUFFER + uefi_lp->FrameBufferSize] to
    //     [uefi_lp->FrameBufferBase, uefi_lp->FrameBufferBase + uefi_lp->FrameBufferSize] RW- + PROT_WC
    // Map [X86ADDR(KERN_BASE_ADDR),MIN(MAX_LOW_ADDR_KERN_SIZE, max_memory_map_addr)] to
    //     [0, MIN(MAX_LOW_ADDR_KERN_SIZE, max_memory_map_addr)] as RW + ALLOC_WEAK
    // Map [X86ADDR((uintptr_t)__text_start),ROUNDUP(X86ADDR((uintptr_t)__text_end), CLASS_SIZE(0))] to
    //     [PADDR(__text_start), ROUNDUP(__text_end, CLASS_SIZE(0))] as R-X
    // Map [X86ADDR(KERN_STACK_TOP - KERN_STACK_SIZE), KERN_STACK_TOP] to
    //     [PADDR(bootstack), PADDR(boottop)] as RW-
    // Map [X86ADDR(KERN_PF_STACK_TOP - KERN_PF_STACK_SIZE), KERN_PF_STACK_TOP] to
    //     [PADDR(pfstack), PADDR(pfstacktop)] as RW-

    if (trace_memory_more) dump_page_table(kspace.pml4);

    check_physical_tree(&root);
    if (trace_init) cprintf("Physical memory tree is stil correct\n");

    check_virtual_tree(kspace.root, MAX_CLASS);
    if (trace_init) cprintf("Kernel virutal memory tree is correct\n");
}
