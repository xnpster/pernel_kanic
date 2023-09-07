/** @file
  Copyright (c) 2020, ISP RAS. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef LOADER_PARAMS_H
#define LOADER_PARAMS_H

typedef struct {
  ///
  /// Virtual pointer to self.
  ///
  EFI_VIRTUAL_ADDRESS      SelfVirtual;

  ///
  /// UEFI services and configuration.
  ///
  EFI_VIRTUAL_ADDRESS      RTServices;                  // UEFI Runtime Services
  EFI_PHYSICAL_ADDRESS     ACPIRoot;                    // ACPI RSDP Table.

  ///
  /// Memory mapping
  ///
  UINT32                   MemoryMapDescriptorVersion;  // The memory descriptor version
  UINT64                   MemoryMapDescriptorSize;     // The size of an individual memory descriptor
  EFI_PHYSICAL_ADDRESS     MemoryMap;                   // The system memory map as an array of EFI_MEMORY_DESCRIPTOR structs
  EFI_VIRTUAL_ADDRESS      MemoryMapVirt;               // Virtual address of memmap
  UINT64                   MemoryMapSize;               // The total size of the system memory map

  ///
  /// Graphics information.
  ///
  EFI_PHYSICAL_ADDRESS     FrameBufferBase;
  UINT32                   FrameBufferSize;
  UINT32                   PixelsPerScanLine;
  UINT32                   VerticalResolution;
  UINT32                   HorizontalResolution;

  ///
  /// Debug information.
  ///
  EFI_PHYSICAL_ADDRESS     DebugArangesStart;
  EFI_PHYSICAL_ADDRESS     DebugArangesEnd;
  EFI_PHYSICAL_ADDRESS     DebugAbbrevStart;
  EFI_PHYSICAL_ADDRESS     DebugAbbrevEnd;
  EFI_PHYSICAL_ADDRESS     DebugInfoStart;
  EFI_PHYSICAL_ADDRESS     DebugInfoEnd;
  EFI_PHYSICAL_ADDRESS     DebugLineStart;
  EFI_PHYSICAL_ADDRESS     DebugLineEnd;
  EFI_PHYSICAL_ADDRESS     DebugStrStart;
  EFI_PHYSICAL_ADDRESS     DebugStrEnd;
  EFI_PHYSICAL_ADDRESS     DebugPubnamesStart;
  EFI_PHYSICAL_ADDRESS     DebugPubnamesEnd;
  EFI_PHYSICAL_ADDRESS     DebugPubtypesStart;
  EFI_PHYSICAL_ADDRESS     DebugPubtypesEnd;

  ///
  /// Kernel symbols
  ///
  EFI_PHYSICAL_ADDRESS     SymbolTableStart;
  EFI_PHYSICAL_ADDRESS     SymbolTableEnd;
  EFI_PHYSICAL_ADDRESS     StringTableStart;
  EFI_PHYSICAL_ADDRESS     StringTableEnd;

} LOADER_PARAMS;

#endif // LOADER_PARAMS_H
