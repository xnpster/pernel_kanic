/** @file
  Copyright (c) 2006, Intel Corporation. All rights reserved.
  Copyright (c) 2020, ISP RAS. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtualMemory.h"

//
// Create 2M-page table
// PML4 (47:39)
// PDPTE (38:30)
// PDE (29:21)
//

#define EFI_MAX_ENTRY_NUM     512

#define EFI_PML4_ENTRY_NUM    1
#define EFI_PDPTE_ENTRY_NUM   4
#define EFI_PDE_ENTRY_NUM     EFI_MAX_ENTRY_NUM

#define EFI_PML4_PAGE_NUM     1
#define EFI_PDPTE_PAGE_NUM    EFI_PML4_ENTRY_NUM
#define EFI_PDE_PAGE_NUM      (EFI_PML4_ENTRY_NUM * EFI_PDPTE_ENTRY_NUM)

#define EFI_PAGE_NUMBER       (EFI_PML4_PAGE_NUM + EFI_PDPTE_PAGE_NUM + EFI_PDE_PAGE_NUM)
#define EFI_PAGE_SIZE_2M      0x200000

VOID *
PreparePageTable (
  VOID
  )
{
  EFI_STATUS                                    Status;
  UINT64                                        PageAddress;
  EFI_PHYSICAL_ADDRESS                          PageTableMemory;
  UINT8                                         *PageTable;
  UINT8                                         *PageTablePtr;
  INTN                                          PML4Index;
  INTN                                          PDPTEIndex;
  INTN                                          PDEIndex;
  X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageMapLevel4Entry;
  X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageDirectoryPointerEntry;
  X64_PAGE_TABLE_ENTRY_2M                       *PageDirectoryEntry2MB;


  PageTableMemory = BASE_4GB;
  Status = gBS->AllocatePages (
    AllocateMaxAddress,
    EfiRuntimeServicesData,
    EFI_PAGE_NUMBER,
    &PageTableMemory
    );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  PageTable = (VOID *)(UINTN) PageTableMemory;

  ZeroMem (PageTable, (EFI_PAGE_NUMBER * EFI_PAGE_SIZE));
  PageTablePtr = PageTable;

  PageAddress = 0;

  //
  //  Page Table structure 3 level 2MB.
  //
  //                   Page-Map-Level-4-Table        : bits 47-39
  //                   Page-Directory-Pointer-Table  : bits 38-30
  //
  //  Page Table 2MB : Page-Directory(2M)            : bits 29-21
  //
  //

  PageMapLevel4Entry = (X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K *)PageTablePtr;

  for (PML4Index = 0; PML4Index < EFI_PML4_ENTRY_NUM; PML4Index++, PageMapLevel4Entry++) {
    //
    // Each Page-Map-Level-4-Table Entry points to the base address of a Page-Directory-Pointer-Table Entry
    //
    PageTablePtr += EFI_PAGE_SIZE;
    PageDirectoryPointerEntry = (X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K *)PageTablePtr;

    //
    // Make a Page-Map-Level-4-Table Entry
    //
    PageMapLevel4Entry->Uint64 = (UINT64)(UINTN) PageDirectoryPointerEntry;
    PageMapLevel4Entry->Bits.ReadWrite = 1;
    PageMapLevel4Entry->Bits.Present = 1;

    for (PDPTEIndex = 0; PDPTEIndex < EFI_PDPTE_ENTRY_NUM; PDPTEIndex++, PageDirectoryPointerEntry++) {
      //
      // Each Page-Directory-Pointer-Table Entry points to the base address of a Page-Directory Entry
      //
      PageTablePtr += EFI_PAGE_SIZE;
      PageDirectoryEntry2MB = (X64_PAGE_TABLE_ENTRY_2M *)PageTablePtr;

      //
      // Make a Page-Directory-Pointer-Table Entry
      //
      PageDirectoryPointerEntry->Uint64 = (UINT64)(UINTN) PageDirectoryEntry2MB;
      PageDirectoryPointerEntry->Bits.ReadWrite = 1;
      PageDirectoryPointerEntry->Bits.Present = 1;

      for (PDEIndex = 0; PDEIndex < EFI_PDE_ENTRY_NUM; PDEIndex++, PageDirectoryEntry2MB++) {
        //
        // Make a Page-Directory Entry
        //
        PageDirectoryEntry2MB->Uint64 = (UINT64)PageAddress;
        PageDirectoryEntry2MB->Bits.ReadWrite = 1;
        PageDirectoryEntry2MB->Bits.Present = 1;
        PageDirectoryEntry2MB->Bits.MustBe1 = 1;

        PageAddress += EFI_PAGE_SIZE_2M;
      }
    }
  }

  return PageTable;
}

//
// List of currently defined memory types by the specification.
//
STATIC CONST CHAR8 *mMemoryTypes[] = {
  "EfiReservedMemoryType     ",
  "EfiLoaderCode             ",
  "EfiLoaderData             ",
  "EfiBootServicesCode       ",
  "EfiBootServicesData       ",
  "EfiRuntimeServicesCode    ",
  "EfiRuntimeServicesData    ",
  "EfiConventionalMemory     ",
  "EfiUnusableMemory         ",
  "EfiACPIReclaimMemory      ",
  "EfiACPIMemoryNVS          ",
  "EfiMemoryMappedIO         ",
  "EfiMemoryMappedIOPortSpace",
  "EfiPalCode                ",
  "EfiPersistentMemory       "
};

EFI_STATUS
GetMemoryMap (
  IN OUT UINTN                       *MemoryMapSize,
  OUT    EFI_MEMORY_DESCRIPTOR       **MemoryMap,
  OUT    UINTN                       *MapKey,
  OUT    UINTN                       *DescriptorSize,
  OUT    UINT32                      *DescriptorVersion
  )
{
  EFI_STATUS  Status;

  //
  // If the MemoryMap buffer is too small, the EFI_BUFFER_TOO_SMALL error code is returned and the
  // MemoryMapSize value contains the size of the buffer needed to contain the current memory map.
  // The actual size of the buffer allocated for the consequent call to GetMemoryMap() should be bigger then the
  // value returned in MemoryMapSize, since allocation of the new buffer may potentially increase memory map size.
  // On success a MapKey is returned that identifies the current memory map. The firmware’s key is changed
  // every time something in the memory map changes. In order to successfully invoke
  // EFI_BOOT_SERVICES.ExitBootServices() the caller must provide the current memory map key.
  //
  *MemoryMap = NULL;
  *MemoryMapSize = 0;
  Status = gBS->GetMemoryMap (
    MemoryMapSize,
    *MemoryMap,
    MapKey,
    DescriptorSize,
    DescriptorVersion
    );

  while (Status == EFI_BUFFER_TOO_SMALL) {
    if (*MemoryMap != NULL) {
      FreePool (*MemoryMap);
    }

    *MemoryMapSize += 1024;
    *MemoryMap = AllocateRuntimePool (*MemoryMapSize);
    if (*MemoryMap == NULL) {
      DEBUG ((DEBUG_ERROR, "JOS: Failed to allocate memory map\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->GetMemoryMap (
      MemoryMapSize,
      *MemoryMap,
      MapKey,
      DescriptorSize,
      DescriptorVersion
      );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed to obtain memory map\n"));
    if (*MemoryMap != NULL) {
      FreePool (*MemoryMap);
    }
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PrintMemoryMap (
  VOID
  )
{
  EFI_STATUS              Status;
  UINTN                   MemMapSize;
  UINTN                   MemMapKey;
  UINTN                   MemMapDescriptorSize;
  UINT32                  Index;
  UINT32                  MemMapDescriptorVersion;
  EFI_MEMORY_DESCRIPTOR   *MemMap;
  EFI_MEMORY_DESCRIPTOR   *MemMapEnd;
  EFI_MEMORY_DESCRIPTOR   *Piece;
  CONST CHAR8             *TypeName;

  Status = GetMemoryMap (
    &MemMapSize,
    &MemMap,
    &MemMapKey,
    &MemMapDescriptorSize,
    &MemMapDescriptorVersion
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Error getting memory map for printing - %r\n", Status));
    return Status;
  }

  MemMapEnd = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *) MemMap + MemMapSize);

  DEBUG ((
    DEBUG_INFO,
    "MemMapSize: %u, MemMapDescriptorSize: %u, MemMapDescriptorVersion: 0x%x\n",
    (UINT32) MemMapSize,
    (UINT32) MemMapDescriptorSize,
    MemMapDescriptorVersion
    ));

  DEBUG ((
    DEBUG_INFO,
    "   #  Memory Type                Phys Addr Start    Phys Addr End      Attr\n"
    ));

  //
  // There is no virtual addressing yet, so there is no need to print Piece->VirtualStart.
  //
  Index = 0;
  for (Piece = MemMap; Piece < MemMapEnd; Piece = NEXT_MEMORY_DESCRIPTOR (Piece, MemMapDescriptorSize)) {
    if (Piece->Type >= ARRAY_SIZE (mMemoryTypes)) {
      TypeName = "Unknown";
    } else {
      TypeName = mMemoryTypes[Piece->Type];
    }

    DEBUG ((
      DEBUG_INFO,
      "% 4u: %a 0x%016Lx-0x%016Lx 0x%Lx\r\n",
      Index,
      TypeName,
      Piece->PhysicalStart,
      Piece->PhysicalStart + (EFI_PAGES_TO_SIZE (Piece->NumberOfPages) - 1),
      Piece->Attribute
      ));
    
    ++Index;
  }

  FreePool (MemMap);
  return EFI_SUCCESS;
}

EFI_VIRTUAL_ADDRESS
SetupVirtualAddresses (
  IN     UINTN                  MemMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemMap,
  IN     UINTN                  DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR   *Piece;
  EFI_MEMORY_DESCRIPTOR   *MemMapEnd;
  EFI_VIRTUAL_ADDRESS     CurrentAddress;

  CurrentAddress = VIRT_ADDR_START;
  MemMapEnd = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *) MemMap + MemMapSize);

  //
  // Put all untouchable memory to virtual address space starting with VIRT_ADDR_START.
  //
  for (Piece = MemMap; Piece < MemMapEnd; Piece = NEXT_MEMORY_DESCRIPTOR (Piece, DescriptorSize)) {
    //
    // Skip reserved and non-runtime memory.
    //
    if (Piece->Type == EfiReservedMemoryType || (Piece->Attribute & EFI_MEMORY_RUNTIME) == 0) {
      continue;
    }

    Piece->VirtualStart = CurrentAddress;
    CurrentAddress     += EFI_PAGES_TO_SIZE (Piece->NumberOfPages);
  }

  return CurrentAddress;
}
