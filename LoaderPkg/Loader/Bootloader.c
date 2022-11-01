/** @file
  Copyright (c) 2020, ISP RAS. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <IndustryStandard/Acpi62.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/DebugLib.h>
#include "Bootloader.h"
#include "VirtualMemory.h"

VOID *
AllocateLowRuntimePool (
  IN UINTN  Size
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Area;

  Area = BASE_1GB;

  Status = gBS->AllocatePages (
    AllocateMaxAddress,
    EfiRuntimeServicesData,
    EFI_SIZE_TO_PAGES (Size),
    &Area
    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return (VOID *)(UINTN) Area;
}

/**
  Handle protocol on handle and fallback to any protocol when missing.

  @param[in]  Handle        Handle to search for protocol.
  @param[in]  Protocol      Protocol to search for.
  @param[out] Interface     Protocol interface if found.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
HandleProtocolFallback (
  IN  EFI_HANDLE  Handle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface
  )
{
  EFI_STATUS Status;

  ASSERT (Protocol != NULL);
  ASSERT (Interface != NULL);

  Status = gBS->HandleProtocol (
    Handle,
    Protocol,
    Interface
    );
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateProtocol (
      Protocol,
      NULL,
      Interface
      );
  }

  return Status;
}

/**
  Initialise graphics rendering and set loader params.

  @param[out] LoaderParams  Loader parameters for graphics configuration.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
InitGraphics (
  OUT LOADER_PARAMS  *LoaderParams
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;

  ASSERT (LoaderParams != NULL);

  STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL mBlackColour = {0x00, 0x00, 0x00, 0x00};

  //
  // Obtain graphics output protocol.
  //
  Status = HandleProtocolFallback (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID **) &GraphicsOutput
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Cannot find GOP protocol - %r\n", Status));
    return Status;
  }

  //
  // LAB 1: Your code here.
  //
  // Switch to the maximum or any other resolution of your preference.
  // Refer to Graphics Output Protocol description in UEFI spec for
  // more details.
  //
  // Hint: Use GetMode/SetMode functions.
  //


  //
  // Fill screen with black.
  //
  GraphicsOutput->Blt (
    GraphicsOutput,
    &mBlackColour,
    EfiBltVideoFill,
    0,
    0,
    0,
    0,
    GraphicsOutput->Mode->Info->HorizontalResolution,
    GraphicsOutput->Mode->Info->VerticalResolution,
    0
    );

  //
  // Fill GPU properties.
  //
  LoaderParams->FrameBufferBase      = GraphicsOutput->Mode->FrameBufferBase;
  LoaderParams->FrameBufferSize      = GraphicsOutput->Mode->FrameBufferSize;
  LoaderParams->PixelsPerScanLine    = GraphicsOutput->Mode->Info->PixelsPerScanLine;
  LoaderParams->HorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
  LoaderParams->VerticalResolution   = GraphicsOutput->Mode->Info->VerticalResolution;

  return EFI_SUCCESS;
}

/**
  Find RSD_PTR Table In Legacy Area

  @retval EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER
**/
STATIC
EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *
AcpiFindLegacyRsdp (
  VOID
  )
{
  EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp;

  UINTN                                        Address;
  UINTN                                        Index;

  //
  // First Search 0x0E0000 - 0x0FFFFF for RSD_PTR
  //

  Rsdp = NULL;

  for (Address = 0x0E0000; Address < 0x0FFFFF; Address += 16) {
    if (*(UINT64 *) Address == EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE) {
      Rsdp = (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *) Address;
    }
  }

  //
  // Then Search EBDA 0x40E - 0x800
  //

  if (Rsdp == NULL) {
    Address = ((UINTN)(*(UINT16 *) 0x040E) << 4U);

    for (Index = 0; Index < 0x0400; Index += 16) {
      if (*(UINT64 *) (Address + Index) == EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE) {
        Rsdp = (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *) Address;
      }
    }
  }

  return Rsdp;
}

/**
  Find RSD_PTR Table From System Configuration Tables

  @retval EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER
**/
STATIC
EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *
AcpiFindRsdp (
  VOID
  )
{
  EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  UINTN                                         Index;

  Rsdp    = NULL;

  //
  // Find ACPI table RSD_PTR from system table
  //

  for (Index = 0; Index < gST->NumberOfTableEntries; ++Index) {
    //
    // Prefer ACPI 2.0
    //
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiAcpi20TableGuid)) {
      Rsdp = (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *) gST->ConfigurationTable[Index].VendorTable;
      DEBUG ((DEBUG_INFO, "JOS: Found ACPI 2.0 RSDP table %p\n", Rsdp));
      break;
    }

    //
    // Otherwise use ACPI 1.0, but do search for ACPI 2.0.
    //
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiAcpi10TableGuid)) {
      Rsdp = (EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER *) gST->ConfigurationTable[Index].VendorTable;
      DEBUG ((DEBUG_INFO, "JOS: Found ACPI 1.0 RSDP table %p\n", Rsdp));
    }
  }

  //
  // Try to use legacy search as a last resort.
  //
  if (Rsdp == NULL) {
    Rsdp = AcpiFindLegacyRsdp ();
    if (Rsdp != NULL) {
      DEBUG ((DEBUG_INFO, "JOS: Found ACPI legacy RSDP table %p\n", Rsdp));
    }
  }

  if (Rsdp == NULL) {
    DEBUG ((DEBUG_WARN, "OCA: Failed to find ACPI RSDP table\n"));
  }

  return Rsdp;
}

/**
  Obtain kernel file protocol.

  @param[out] FileProtocol  Instance of file protocol holding the kernel.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
GetKernelFile (
  OUT  EFI_FILE_PROTOCOL  **FileProtocol
  )
{
  EFI_STATUS                       Status;
  EFI_LOADED_IMAGE_PROTOCOL        *LoadedImage;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  EFI_FILE_PROTOCOL                *CurrentDriveRoot;
  EFI_FILE_PROTOCOL                *KernelFile;

  ASSERT (FileProtocol != NULL);

  Status = gBS->HandleProtocol (
    gImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **) &LoadedImage
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Cannot find LoadedImage protocol - %r\n", Status));
    return Status;
  }

  if (LoadedImage->DeviceHandle == NULL) {
    DEBUG ((DEBUG_ERROR, "JOS: LoadedImage protocol has no DeviceHandle\n"));
    return EFI_UNSUPPORTED;
  }

  Status = gBS->HandleProtocol (
    LoadedImage->DeviceHandle,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &FileSystem
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Cannot find own FileSystem protocol - %r\n", Status));
    return Status;
  }

  Status = FileSystem->OpenVolume (
    FileSystem,
    &CurrentDriveRoot
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Cannot access own file system - %r\n", Status));
    return Status;
  }

  Status = CurrentDriveRoot->Open (
    CurrentDriveRoot,
    &KernelFile,
    KERNEL_PATH,
    EFI_FILE_MODE_READ,
    0
    );

  CurrentDriveRoot->Close (CurrentDriveRoot);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Cannot access own file system - %r\n", Status));
    return Status;
  }

  *FileProtocol = KernelFile;
  return EFI_SUCCESS;
}

/**
  Read file data at offset of specified size.

  @param[in]  File    File protocol instance.
  @param[in]  Offset  Data reading offset.
  @param[in]  Size    Amount of data to read.
  @param[out] Data    Preallocated buffer for data reading.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
CheckedReadData (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINTN              Offset,
  IN  UINTN              Size,
  OUT VOID               *Data
  )
{
  EFI_STATUS  Status;
  UINTN       ReadSize;

  ASSERT (File != NULL);
  ASSERT (Data != NULL);

  Status = File->SetPosition (File, Offset);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Failed to set poisitition to 0x%X to read %u bytes - %r\n",
      (UINT32) Offset,
      (UINT32) Size,
      Status
      ));
    return Status;
  }

  ReadSize = Size;
  Status = File->Read (File, &ReadSize, Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Failed to read %u bytes at 0x%X - %r\n",
      (UINT32) Size,
      (UINT32) Offset,
      Status
      ));
    return Status;
  }

  if (Size != ReadSize) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Failed to read %u bytes at %u, read only %u bytes\n",
      (UINT32) Size,
      (UINT32) Offset,
      (UINT32) ReadSize
      ));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Read C string at offset of specified maximum size.

  @param[in]  File    File protocol instance.
  @param[in]  Offset  String reading offset.
  @param[in]  Size    Maximum string size to read.
  @param[out] String  Preallocated buffer for string reading.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
CheckedReadString (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINTN              Offset,
  IN  UINTN              Size,
  OUT CHAR8              *String
  )
{
  EFI_STATUS  Status;
  UINTN       ReadSize;

  ASSERT (File != NULL);
  ASSERT (Size > 1);
  ASSERT (String != NULL);

  Status = File->SetPosition (File, Offset);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Failed to set poisitition to 0x%X to read %u-byte string - %r\n",
      (UINT32) Offset,
      (UINT32) Size,
      Status
      ));
    return Status;
  }

  ReadSize = Size - 1;
  Status = File->Read (File, &ReadSize, String);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Failed to read %u-byte string at 0x%X - %r\n",
      (UINT32) Size,
      (UINT32) Offset,
      Status
      ));
    return Status;
  }

  String[ReadSize] = '\0';
  return EFI_SUCCESS;
}

/**
  Load kernel into memory.

  @param[out] LoaderParams  Loader parameters to fill debug information.
  @param[out] EntryPoint    Kernel entry point.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
LoadKernel (
  OUT  LOADER_PARAMS  *LoaderParams,
  OUT  UINTN          *EntryPoint
  )
{
  EFI_STATUS            Status;
  EFI_FILE_PROTOCOL     *KernelFile;
  UINTN                 Index;
  UINTN                 Index2;
  struct Elf            ElfHeader;
  struct Secthdr        *Sections;
  struct Proghdr        *ProgramHeaders;
  UINTN                 StringOffset;
  CHAR8                 NameTemp[16];
  VOID                  *SectionData;
  EFI_PHYSICAL_ADDRESS  MinAddress;
  EFI_PHYSICAL_ADDRESS  MaxAddress;
  EFI_PHYSICAL_ADDRESS  KernelSize;

  ASSERT (LoaderParams != NULL);
  ASSERT (EntryPoint != NULL);

  //
  // Obtain kernel file handle.
  // Assume that our loader booted from a device using the EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.
  //
  Status = GetKernelFile (&KernelFile);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed to obtain kernel file - %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "JOS: Loading kernel image...\n"));

  //
  // Read and verify ELF header.
  // TODO: Here we blindly trust that ELF file has valid data. Anybody implementing
  // Secure Boot functionality will have to ensure this by checking the signature
  // or at least verifying the bounds.
  //
  Status = CheckedReadData (KernelFile, 0, sizeof (ElfHeader), &ElfHeader);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Cannot read kernel header - %r\n", Status));
    KernelFile->Close (KernelFile);
    return Status;
  }

  if (ElfHeader.e_magic != ELF_MAGIC) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Kernel has magic %08X instead of %08X\n",
      ElfHeader.e_magic,
      ELF_MAGIC
      ));
    KernelFile->Close (KernelFile);
    return EFI_UNSUPPORTED;
  }

  //
  // TODO: Check other fields.
  //

  //
  // Go through sections and get the needed debug information.
  //
  DEBUG ((DEBUG_INFO, "JOS: Loading debug tables...\n"));

  if (ElfHeader.e_shentsize != sizeof (struct Secthdr)) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Kernel has sections of %u bytes instead of %u\n",
      ElfHeader.e_shentsize,
      (UINT32) sizeof (struct Secthdr)
      ));
    KernelFile->Close (KernelFile);
    return EFI_UNSUPPORTED;
  }

  if (ElfHeader.e_shstrndx >= ElfHeader.e_shnum) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Kernel string section has invalid index %u out of %u entries\n",
      ElfHeader.e_shstrndx,
      ElfHeader.e_shnum
      ));
    KernelFile->Close (KernelFile);
    return EFI_UNSUPPORTED;
  }

  if (ElfHeader.e_phentsize != sizeof (struct Proghdr)) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Kernel has program headers of %u bytes instead of %u\n",
      ElfHeader.e_phentsize,
      (UINT32) sizeof (struct Proghdr)
      ));
    KernelFile->Close (KernelFile);
    return EFI_UNSUPPORTED;
  }

  Sections = AllocatePool (ElfHeader.e_shnum * sizeof (struct Secthdr));
  if (Sections == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Failed to allocate memory for %u kernel section headers\n",
      ElfHeader.e_shnum
      ));
    KernelFile->Close (KernelFile);
    return EFI_UNSUPPORTED;
  }

  ProgramHeaders = AllocatePool (ElfHeader.e_phnum * sizeof (struct Proghdr));
  if (ProgramHeaders == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Failed to allocate memory for %u kernel program headers\n",
      ElfHeader.e_phnum
      ));
    KernelFile->Close (KernelFile);
    FreePool (Sections);
    return EFI_UNSUPPORTED;
  }

  Status = CheckedReadData (
    KernelFile,
    ElfHeader.e_shoff,
    ElfHeader.e_shnum * sizeof (struct Secthdr),
    Sections
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Failed to read %u kernel section headers - %r\n",
      ElfHeader.e_shnum,
      Status
      ));
    FreePool (Sections);
    FreePool (ProgramHeaders);
    KernelFile->Close (KernelFile);
    return EFI_UNSUPPORTED;
  }

  StringOffset = Sections[ElfHeader.e_shstrndx].sh_offset;

  Status = CheckedReadData (
    KernelFile,
    ElfHeader.e_phoff,
    ElfHeader.e_phnum * sizeof (struct Proghdr),
    ProgramHeaders
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "JOS: Failed to read %u kernel program headers - %r\n",
      ElfHeader.e_shnum,
      Status
      ));
    FreePool (Sections);
    FreePool (ProgramHeaders);
    KernelFile->Close (KernelFile);
    return EFI_UNSUPPORTED;
  }

  STATIC struct {
    CONST CHAR8  *Name;
    UINTN        StartOffset;
    UINTN        EndOffset;
  } mDebugMapping[] = {
    {".debug_aranges",  OFFSET_OF (LOADER_PARAMS, DebugArangesStart),  OFFSET_OF (LOADER_PARAMS, DebugArangesEnd)},
    {".debug_abbrev",   OFFSET_OF (LOADER_PARAMS, DebugAbbrevStart),   OFFSET_OF (LOADER_PARAMS, DebugAbbrevEnd)},
    {".debug_info",     OFFSET_OF (LOADER_PARAMS, DebugInfoStart),     OFFSET_OF (LOADER_PARAMS, DebugInfoEnd)},
    {".debug_line",     OFFSET_OF (LOADER_PARAMS, DebugLineStart),     OFFSET_OF (LOADER_PARAMS, DebugLineEnd)},
    {".debug_str",      OFFSET_OF (LOADER_PARAMS, DebugStrStart),      OFFSET_OF (LOADER_PARAMS, DebugStrEnd)},
    {".debug_pubnames", OFFSET_OF (LOADER_PARAMS, DebugPubnamesStart), OFFSET_OF (LOADER_PARAMS, DebugPubnamesEnd)},
    {".debug_pubtypes", OFFSET_OF (LOADER_PARAMS, DebugPubtypesStart), OFFSET_OF (LOADER_PARAMS, DebugPubtypesEnd)},
    {".symtab",         OFFSET_OF (LOADER_PARAMS, SymbolTableStart),   OFFSET_OF (LOADER_PARAMS, SymbolTableEnd)},
    {".strtab",         OFFSET_OF (LOADER_PARAMS, StringTableStart),   OFFSET_OF (LOADER_PARAMS, StringTableEnd)},
  };

  Status = EFI_SUCCESS;

  for (Index = 0; Index < ElfHeader.e_shnum; ++Index) {
    Status = CheckedReadString (
      KernelFile,
      StringOffset + Sections[Index].sh_name,
      sizeof (NameTemp),
      NameTemp
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "JOS: Cannot read section %u (base 0x%X off 0x%X) name - %r\n",
        (UINT32) Index,
        (UINT32) StringOffset,
        (UINT32) Sections[Index].sh_name,
        Status
        ));
      break;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "JOS: Got section %u/%u %a (at 0x%X + 0x%X)\n",
      (UINT32) (Index + 1),
      (UINT32) ElfHeader.e_shnum,
      NameTemp,
      (UINT32) StringOffset,
      (UINT32) Sections[Index].sh_name
      ));

    for (Index2 = 0; Index2 < ARRAY_SIZE (mDebugMapping); ++Index2) {
      ASSERT (AsciiStrSize (mDebugMapping[Index2].Name) <= sizeof (NameTemp));

      if (AsciiStrCmp (mDebugMapping[Index2].Name, NameTemp) == 0) {
        SectionData = AllocateLowRuntimePool (Sections[Index].sh_size);
        if (SectionData == NULL) {
          DEBUG ((
            DEBUG_ERROR,
            "JOS: Failed to allocate %u bytes for section %a\n",
            Sections[Index].sh_size,
            NameTemp
            ));
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        DEBUG ((DEBUG_VERBOSE, "JOS: Allocated section %a to %p\n", NameTemp, SectionData));

        Status = CheckedReadData (
          KernelFile,
          Sections[Index].sh_offset,
          Sections[Index].sh_size,
          SectionData
          );
        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_ERROR,
            "JOS: Failed to read section %a\n",
            NameTemp
            ));
          gBS->FreePages ((UINTN) SectionData, EFI_SIZE_TO_PAGES (Sections[Index].sh_size));
          break;
        }

        *(EFI_PHYSICAL_ADDRESS *)((UINT8 *) LoaderParams + mDebugMapping[Index2].StartOffset) =
          (UINTN) SectionData;
        *(EFI_PHYSICAL_ADDRESS *)((UINT8 *) LoaderParams + mDebugMapping[Index2].EndOffset) =
          (UINTN) SectionData + Sections[Index].sh_size;

        break;
      }

      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // Allocate kernel memory.
    //
    MinAddress = ~0ULL;   ///< Physical address min (-1 wraps around to max 64-bit number).
    MaxAddress = 0;       ///< Physical address max.

    for (Index = 0; Index < ElfHeader.e_phnum; ++Index) {
      if (ProgramHeaders[Index].p_type == PT_LOAD) {
        //
        // TODO: Add overflow bounds checking.
        //
        MinAddress = MinAddress < ProgramHeaders[Index].p_pa
          ? MinAddress : ProgramHeaders[Index].p_pa;
        MaxAddress = (MaxAddress > ProgramHeaders[Index].p_pa + ProgramHeaders[Index].p_memsz)
          ? MaxAddress : ProgramHeaders[Index].p_pa + ProgramHeaders[Index].p_memsz;
      }
    }

    KernelSize = MaxAddress - MinAddress;

    DEBUG ((
      DEBUG_INFO,
      "JOS: MinAddress: 0x%Lx, MaxAddress: 0x%Lx, Kernel size: 0x%Lx\n",
      MinAddress,
      MaxAddress,
      KernelSize
      ));

    if (MaxAddress > MinAddress) {
      Status = gBS->AllocatePages (
        AllocateAddress,
        EfiRuntimeServicesData,
        EFI_SIZE_TO_PAGES (KernelSize),
        &MinAddress
        );
      if (!EFI_ERROR(Status)) {
        ZeroMem((VOID *)(UINTN) MinAddress, KernelSize);

        for (Index = 0; Index < ElfHeader.e_phnum; ++Index) {
          if (ProgramHeaders[Index].p_type == PT_LOAD &&
              ProgramHeaders[Index].p_filesz > 0) {
            Status = CheckedReadData (
              KernelFile,
              ProgramHeaders[Index].p_offset,
              ProgramHeaders[Index].p_filesz,
              (VOID *)(UINTN) ProgramHeaders[Index].p_pa
              );
            if (EFI_ERROR (Status)) {
              DEBUG ((DEBUG_ERROR, "JOS: Could not read program header %u - %r\n", (UINT32) Index, Status));
              gBS->FreePages (MinAddress, EFI_SIZE_TO_PAGES (KernelSize));
              break;
            }
          }
        }
      } else {
        DEBUG ((DEBUG_ERROR, "JOS: Could not allocate kernel memory - %r\n", Status));
      }
    } else {
      DEBUG ((DEBUG_ERROR, "JOS: Kernel has invalid size\n"));
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Free sections and headers.
  //
  FreePool (Sections);
  FreePool (ProgramHeaders);

  //
  // Free allocated memory on error.
  //
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "JOS: Loaded kernel with %Lx entry point\n", ElfHeader.e_entry));
    *EntryPoint = (UINTN) ElfHeader.e_entry;
  } else {
    for (Index = 0; Index < ARRAY_SIZE (mDebugMapping); ++Index) {
      SectionData = (VOID *)(UINTN) *(EFI_PHYSICAL_ADDRESS *)(
        (UINT8 *) LoaderParams + mDebugMapping[Index2].StartOffset);

      if (SectionData != NULL) {
        gBS->FreePages (
          (UINTN) SectionData,
          EFI_SIZE_TO_PAGES (
            *(EFI_PHYSICAL_ADDRESS *)((UINT8 *) LoaderParams + mDebugMapping[Index2].EndOffset)
            - *(EFI_PHYSICAL_ADDRESS *)((UINT8 *) LoaderParams + mDebugMapping[Index2].StartOffset)
            )
          );
      }
    }
  }

  return Status;
}

/**
  Exit BootServices services and setup virtual addressing
  to prepare for kernel loading.

  @param[out]  LoaderParams  Loader parameters.
  @param[out]  GateData      Arch-specific kernel call gate data.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
TransitionToKernelMode (
  OUT  LOADER_PARAMS  *LoaderParams,
  OUT  VOID           **GateData
  )
{
  EFI_STATUS             Status;
  UINTN                  MemMapSize;
  UINTN                  MemMapKey;
  UINTN                  MemMapDescriptorSize;
  EFI_MEMORY_DESCRIPTOR  *MemMap;
  UINT32                 MemMapDescriptorVersion;

  ASSERT (LoaderParams != NULL);
  ASSERT (GateData != NULL);

  Status = GenerateGateData (
    GateData
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed to generate gate data - %r\n", Status));
    return Status;
  }

  Status = GetMemoryMap (
    &MemMapSize,
    &MemMap,
    &MemMapKey,
    &MemMapDescriptorSize,
    &MemMapDescriptorVersion
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed to obtain memory map - %r\n", Status));
    return Status;
  }

  //
  // For an operating system to use any UEFI runtime services, it must
  // preserve all memory in the memory map marked as runtime code and runtime data
  //
  LoaderParams->MemoryMapDescriptorVersion = MemMapDescriptorVersion;
  LoaderParams->MemoryMapDescriptorSize    = MemMapDescriptorSize;
  LoaderParams->MemoryMap                  = (UINTN) MemMap;
  LoaderParams->MemoryMapVirt              = (UINTN) MemMap;
  LoaderParams->MemoryMapSize              = MemMapSize;

  //
  // Until EFI_BOOT_SERVICES.ExitBootServices() is called, the memory map is owned by the firmware
  // and the currently executing UEFI Image should only use memory pages it has explicitly allocated.
  //
  Status = gBS->ExitBootServices (gImageHandle, MemMapKey);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed to exit Boot Services - %r\n", Status));
    FreePool (MemMap);
    return Status;
  }

  SetupVirtualAddresses (
    MemMapSize,
    MemMap,
    MemMapDescriptorSize
    );

  Status = gRT->SetVirtualAddressMap (
    MemMapSize,
    MemMapDescriptorSize,
    MemMapDescriptorVersion,
    MemMap
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed SetVirtualAddressMap - %r\n", Status));
    CpuDeadLoop ();
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Virtual address map transition event to update
  loader params physical addresses to virtual.

  @param[in] Event     Event handle.
  @param[in] Context   Loader params.
**/
STATIC
VOID
EFIAPI
TranslateVirtualAddresses (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS     Status;
  LOADER_PARAMS  *LoaderParams;

  LoaderParams = Context;

  Status = gRT->ConvertPointer (0, (VOID **)&LoaderParams->SelfVirtual);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (0, (VOID **)&LoaderParams->RTServices);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (0, (VOID **)&LoaderParams->MemoryMapVirt);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugArangesStart);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugArangesEnd);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugAbbrevStart);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugAbbrevEnd);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugInfoStart);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugInfoEnd);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugLineStart);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugLineEnd);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugStrStart);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugStrEnd);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugPubnamesStart);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugPubnamesEnd);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugPubtypesStart);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->DebugPubtypesEnd);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->SymbolTableStart);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->SymbolTableEnd);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->StringTableStart);
  ASSERT_EFI_ERROR (Status);
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&LoaderParams->StringTableEnd);
  ASSERT_EFI_ERROR (Status);
}

/**
  Loader's "main" function. This bootloader's program entry
  point is defined as UefiMain per UEFI application convention.

  @param[in] ImageHandle  Bootloader image handle.
  @param[in] SystemTable  UEFI System Table pointer.

  @returns Does not return on success.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  EFI_TIME           Now;
  LOADER_PARAMS      *LoaderParams;
  EFI_EVENT          VirtualNotifyEvent;
  UINTN              EntryPoint;
  VOID               *GateData;

#if 1 ///< Uncomment to await debugging
  volatile BOOLEAN   Connected;
  DEBUG ((DEBUG_INFO, "JOS: Awaiting debugger connection\n"));

  Connected = FALSE;
  while (!Connected) {
    ;
  }
#endif

  Status = gRT->GetTime (&Now, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Error when getting time - %r\n", Status));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "JOS: Loading at %u-%u-%u - %u:%u:%u...\n",
    Now.Year,
    Now.Month,
    Now.Day,
    Now.Hour,
    Now.Minute,
    Now.Second
    ));

  LoaderParams = AllocateRuntimePool (sizeof (*LoaderParams));
  if (LoaderParams == NULL) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed to allocate loader parameters\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (LoaderParams, sizeof (*LoaderParams));

  LoaderParams->RTServices   = (UINTN) gRT;
  LoaderParams->ACPIRoot     = (UINTN) AcpiFindRsdp ();
  LoaderParams->SelfVirtual  = (UINTN) LoaderParams;

  Status = InitGraphics (LoaderParams);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed to init graphics - %r\n", Status));
    FreePool (LoaderParams);
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "JOS: CR0 - %Lx CR3 - %Lx\n",
    (UINT64) AsmReadCr0 (),
    (UINT64) AsmReadCr3 ()
    ));

  Status = gBS->CreateEvent (
    EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
    TPL_NOTIFY,
    TranslateVirtualAddresses,
    LoaderParams,
    &VirtualNotifyEvent
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed to create vmap event - %r\n", Status));
    FreePool (LoaderParams);
    return Status;
  }

  Status = LoadKernel (LoaderParams, &EntryPoint);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed to load kernel - %r\n", Status));
    FreePool (LoaderParams);
    gBS->CloseEvent (VirtualNotifyEvent);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "JOS: LoaderParams pointer: %p\n", LoaderParams));
  DEBUG ((DEBUG_INFO, "JOS: KernelCallGate: %p\n", CallKernelThroughGate));

#if 0 ///< Enable to see current memory map.
  PrintMemoryMap ();
#endif

  Status = TransitionToKernelMode (LoaderParams, &GateData);
  if (EFI_ERROR (Status)) {
    CpuDeadLoop ();
  }

  CallKernelThroughGate (EntryPoint, LoaderParams, GateData);

  DEBUG ((DEBUG_INFO, "JOS: KernelCallGate returned\n"));
  CpuDeadLoop ();
  return EFI_UNSUPPORTED;
}
