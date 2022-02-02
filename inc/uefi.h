#ifndef JOS_INC_UEFI_H
#define JOS_INC_UEFI_H

#include <inc/types.h>

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;

typedef uintptr_t UINTN;
typedef uint16_t CHAR16;
typedef void VOID;

typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;

typedef struct {
    /* Type of the memory region.
    * Type EFI_MEMORY_TYPE is defined in the
    * AllocatePages() function description. */
    UINT32 Type;

    /* Physical address of the first byte in the memory region. PhysicalStart must be
    * aligned on a 4 KiB boundary, and must not be above 0xFFFFFFFFFFFFF000. Type
    * EFI_PHYSICAL_ADDRESS is defined in the AllocatePages() function description */
    EFI_PHYSICAL_ADDRESS PhysicalStart;

    /* Virtual address of the first byte in the memory region.
    * VirtualStart must be aligned on a 4 KiB boundary,
    * and must not be above 0xFFFFFFFFFFFFF000 */

    EFI_VIRTUAL_ADDRESS VirtualStart;
    /* NumberOfPagesNumber of 4 KiB pages in the memory region.
    * NumberOfPages must not be 0, and must not be any value
    * that would represent a memory page with a start address,
    * either physical or virtual, above 0xFFFFFFFFFFFFF000 */

    UINT64 NumberOfPages;
    /* Attributes of the memory region that describe the bit mask of capabilities
    * for that memory region, and not necessarily the current settings for that
    * memory region. */

    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
    UINT16 Year;
    UINT8 Month;
    UINT8 Day;
    UINT8 Hour;
    UINT8 Minute;
    UINT8 Second;
    UINT8 Pad1;
    UINT32 Nanosecond;
    INT16 TimeZone;
    UINT8 Daylight;
    UINT8 Pad2;
} EFI_TIME;

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8 Data4[8];
} GUID;

typedef GUID EFI_GUID;

#include "../LoaderPkg/Include/LoaderParams.h"

typedef struct {
    uint64_t rcx;
    uint64_t rdx;
    uint64_t r8;
    uint64_t r9;
    uint64_t rax;
} efi_registers;

extern LOADER_PARAMS *uefi_lp;

int efi_call_in_32bit_mode(uint32_t func,
                           efi_registers *efi_reg,
                           void *stack_contents,
                           size_t stack_contents_size, /* 16-byte multiple */
                           uint32_t *efi_status);

/* Attribute values */

/* uncached */
#define EFI_MEMORY_UC ((UINT64)0x0000000000000001ULL)
/* write-coalescing */
#define EFI_MEMORY_WC ((UINT64)0x0000000000000002ULL)
/* write-through */
#define EFI_MEMORY_WT ((UINT64)0x0000000000000004ULL)
/* write-back */
#define EFI_MEMORY_WB ((UINT64)0x0000000000000008ULL)
/* uncached, exported */
#define EFI_MEMORY_UCE ((UINT64)0x0000000000000010ULL)
/* write-protect */
#define EFI_MEMORY_WP ((UINT64)0x0000000000001000ULL)
/* read-protect */
#define EFI_MEMORY_RP ((UINT64)0x0000000000002000ULL)
/* execute-protect */
#define EFI_MEMORY_XP ((UINT64)0x0000000000004000ULL)
/* non-volatile */
#define EFI_MEMORY_NV ((UINT64)0x0000000000008000ULL)
/* higher reliability */
#define EFI_MEMORY_MORE_RELIABLE ((UINT64)0x0000000000010000ULL)
/* read-only */
#define EFI_MEMORY_RO ((UINT64)0x0000000000020000ULL)
/* range requires runtime mapping */
#define EFI_MEMORY_RUNTIME ((UINT64)0x8000000000000000ULL)

#define EFI_MEM_DESC_VERSION 1

#define EFI_PAGE_SHIFT 12
#define EFI_PAGE_SIZE  (1ULL << EFI_PAGE_SHIFT)
#define EFI_PAGE_MASK  (EFI_PAGE_SIZE - 1)

/* Memory types: */
#define EFI_RESERVED_TYPE               0
#define EFI_LOADER_CODE                 1
#define EFI_LOADER_DATA                 2
#define EFI_BOOT_SERVICES_CODE          3
#define EFI_BOOT_SERVICES_DATA          4
#define EFI_RUNTIME_SERVICES_CODE       5
#define EFI_RUNTIME_SERVICES_DATA       6
#define EFI_CONVENTIONAL_MEMORY         7
#define EFI_UNUSABLE_MEMORY             8
#define EFI_ACPI_RECLAIM_MEMORY         9
#define EFI_ACPI_MEMORY_NVS             10
#define EFI_MEMORY_MAPPED_IO            11
#define EFI_MEMORY_MAPPED_IO_PORT_SPACE 12
#define EFI_PAL_CODE                    13
#define EFI_PERSISTENT_MEMORY           14
#define EFI_MAX_MEMORY_TYPE             15

#endif /* JOS_INC_UEFI_H */
