/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_PMAP_H
#define JOS_KERN_PMAP_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/env.h>
#include <inc/x86.h>

#define CLASS_BASE    12
#define CLASS_SIZE(c) (1ULL << ((c) + CLASS_BASE))
#define CLASS_MASK(c) (CLASS_SIZE(c) - 1)

#ifdef SANITIZE_SHADOW_BASE
#define SHADOW_ADDR(v) (((uintptr_t)(v) >> 3) + SANITIZE_SHADOW_OFF)
/* asan unpoison routine used for whitelisting regions. */
void platform_asan_unpoison(void *, size_t);
void platform_asan_poison(void *, size_t);
/* not sanitized memset allows us to access "invalid" areas for extra poisoning. */
void __nosan_memset(void *, int, size_t);
void __nosan_memcpy(void *, void *, size_t);
void __nosan_memmove(void *, void *, size_t);
#define nosan_memmove __nosan_memmove
#define nosan_memcpy  __nosan_memcpy
#define nosan_memset  __nosan_memset
#else
#define nosan_memcpy  memcpy
#define nosan_memset  memset
#define nosan_memmove memmove
#endif

#define MAX_CLASS 48

#define POOL_ENTRIES_FOR_SIZE(sz) (((sz)-offsetof(struct PagePool, data)) / sizeof(struct Page))

#define KB 1024LL
#define MB (KB * 1024)
#define GB (MB * 1024)
#define TB (GB * 1024)

#define PAGE_PROT(p) ((p) & ~NODE_TYPE_MASK)

/* map_region() source override flags */
#define ALLOC_ZERO 0x100000 /* Allocate memory filled with 0x00 */
#define ALLOC_ONE  0x200000 /* Allocate memory filled with 0xFF */
/* map_physical_region() behaviour flags */
#define MAP_USER_MMIO 0x400000 /* Disallow multiple use and be stricter */

/* Memory protection flags & attributes */
#define PROT_X       0x1 /* Executable */
#define PROT_W       0x2 /* Writable */
#define PROT_R       0x4 /* Readable (mostly ignored) */
#define PROT_RWX     0x7
#define PROT_WC      0x8   /* Write-combining */
#define PROT_CD      0x18  /* Cache disable */
#define PROT_USER_   0x20  /* User-accessible */
#define PROT_SHARE   0x40  /* Shared copy flag */
#define PROT_LAZY    0x80  /* Copy-on-Write flag */
#define PROT_COMBINE 0x100 /* Combine old and new priviliges */
#define PROT_AVAIL   0xA00 /* Free-to-use flags, available for applications */
/* (mapped directly to page table unused flags) */
#define PROT_ALL 0xFFF

/* Maximal size of page allocated on pagefault */
#define MAX_ALLOCATION_CLASS 9

enum PageState {
    MAPPING_NODE = 0x100000,      /* Memory mapping (part of virtual tree) */
    INTERMEDIATE_NODE = 0x200000, /* Intermediate node of virtual memory tree */
    PARTIAL_NODE = 0x300000,      /* Intermediate node of physical memory tree */
    ALLOCATABLE_NODE = 0x400000,  /* Generic allocatable memory (part of physical tree) */
    RESERVED_NODE = 0x500000,     /* Reserved memory (part of physical tree) */
    NODE_TYPE_MASK = 0xF00000,
};

extern __attribute__((aligned(HUGE_PAGE_SIZE))) uint8_t zero_page_raw[HUGE_PAGE_SIZE];
extern __attribute__((aligned(HUGE_PAGE_SIZE))) uint8_t one_page_raw[HUGE_PAGE_SIZE];

struct Page {
    struct List head; /* This should be first member */
    struct Page *left, *right, *parent;
    enum PageState state;
    union {
        struct /* physical page */ {
            /* Number of references
             * Child nodes always have class
             * smaller by 1 than their parents */
            uint32_t refc;
            uintptr_t class : CLASS_BASE;                        /* = log2(size)-CLASS_BASE */
            uintptr_t addr : sizeof(uintptr_t) * 8 - CLASS_BASE; /* = address >> CLASS_BASE */
        };
        /* mapping */
        struct Page *phy; /* If phy == NULL this is intemediate page */
    };
};

struct PagePool {
    struct Page *peer;     /* Page from which memory is taken */
    struct PagePool *next; /* Next pool link */
    struct Page data[];    /* Page descriptors storage */
};

int map_region(struct AddressSpace *dspace, uintptr_t dst, struct AddressSpace *sspace, uintptr_t src, uintptr_t size, int flags);
int map_physical_region(struct AddressSpace *dst, uintptr_t dstart, uintptr_t pstart, size_t size, int flags);
void unmap_region(struct AddressSpace *dspace, uintptr_t dst, uintptr_t size);
void init_memory(void);
void release_address_space(struct AddressSpace *space);
struct AddressSpace *switch_address_space(struct AddressSpace *space);
int init_address_space(struct AddressSpace *space);
void user_mem_assert(struct Env *env, const void *va, size_t len, int perm);
int region_maxref(struct AddressSpace *spc, uintptr_t addr, size_t size);
int force_alloc_page(struct AddressSpace *spc, uintptr_t va, int maxclass);
void dump_page_table(pte_t *pml4);
void dump_memory_lists(void);
void dump_virtual_tree(struct Page *node, int class);

void *kzalloc_region(size_t size);

void *mmio_map_region(physaddr_t addr, size_t size);
void *mmio_remap_last_region(physaddr_t addr, void *oldva, size_t oldsz, size_t size);

extern struct AddressSpace kspace;
extern struct AddressSpace *current_space;
extern struct Page root;
extern char bootstacktop[], bootstack[];
extern size_t max_memory_map_addr;

/* This macro takes a kernel virtual address -- an address that points above
 * KERN_BASE_ADDR, where the machine's maximum 512MB of physical memory is mapped --
 * and returns the corresponding physical address.  It panics if you pass it a
 * non-kernel virtual address. */
#define PADDR(kva) _paddr(__FILE__, __LINE__, kva)

static inline physaddr_t
_paddr(const char *file, int line, void *kva) {
    if ((uint64_t)kva < KERN_BASE_ADDR)
        _panic(file, line, "PADDR called with invalid kva %p", kva);
    return (physaddr_t)kva - KERN_BASE_ADDR;
}

/* This macro takes a physical address and returns the corresponding kernel
 * virtual address.  It panics if you pass an invalid physical address. */
#define KADDR(pa) _kaddr(__FILE__, __LINE__, pa)
/* CAUTION: use only before page detection! */
#define _KADDR_NOCHECK(pa) (void *)((physaddr_t)pa + KERN_BASE_ADDR)

static inline void *
_kaddr(const char *file, int line, physaddr_t pa) {
    if (pa > max_memory_map_addr)
        _panic(file, line, "KADDR called with invalid pa %p with max_memory_map_addr=%p", (void *)pa, (void *)max_memory_map_addr);
    return (void *)(pa + KERN_BASE_ADDR);
}

/* This macro takes a kernel virtual address and applies 32-bit mask to it.
 * This is used for mapping required regions in kernel PML table so that
 * required addresses are accessible in 32-bit uefi. */
#define X86MASK      0xFFFFFFFF
#define X86ADDR(kva) ((kva)&X86MASK)

/* Number of PML4 entries taken by userspace */
#define NUSERPML4 1

inline static physaddr_t __attribute__((always_inline))
page2pa(struct Page *page) {
    return page->addr << CLASS_BASE;
}

inline static void
set_wp(bool wp) {
    uintptr_t old = rcr0();
    lcr0(wp ? old | CR0_WP : old & ~CR0_WP);
}


#endif /* !JOS_KERN_PMAP_H */
