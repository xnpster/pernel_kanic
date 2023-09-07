/** @file
  Copyright (c) 2020, ISP RAS. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "Bootloader.h"
#include "VirtualMemory.h"

#include <Register/Intel/Cpuid.h>

VOID
EFIAPI
CallKernelThroughGateAsm (
  IN UINTN          EntryPoint,
  IN LOADER_PARAMS  *LoaderParams,
  IN VOID           *GateData
  );

BOOLEAN
EFIAPI
IsCpuidSupportedAsm (
  VOID
  );

VOID
EFIAPI
CallKernelThroughGate (
  IN UINTN          EntryPoint,
  IN LOADER_PARAMS  *LoaderParams,
  IN VOID           *GateData
  )
{
  BOOLEAN                      Supported;
  UINT32                       Level;
  CPUID_EXTENDED_CPU_SIG_EDX   ExFlags;

  ASSERT (LoaderParams != NULL);
  ASSERT (GateData != NULL);

  //
  // Check CPU compatibility with 64-bit mode.
  //
  Supported = IsCpuidSupportedAsm ();
  if (!Supported) {
    DEBUG ((DEBUG_ERROR, "JOS: No CPUID support\n"));
    return;
  }

  AsmCpuid (CPUID_SIGNATURE, &Level, NULL, NULL, NULL);
  if (Level < CPUID_VERSION_INFO) {
    DEBUG ((DEBUG_ERROR, "JOS: No CPUID level support - %u\n", Level));
    return;
  }

  AsmCpuid (CPUID_EXTENDED_FUNCTION, &Level, NULL, NULL, NULL);
  if (Level < CPUID_EXTENDED_CPU_SIG) {
    DEBUG ((DEBUG_ERROR, "JOS: No CPUID extended level support - %u\n", Level));
    return;
  }

  AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &ExFlags.Uint32);
  if (ExFlags.Bits.LM == 0) {
    DEBUG ((DEBUG_ERROR, "JOS: No LM-bit support on the target CPU\n"));
    return;
  }

  CallKernelThroughGateAsm (
    EntryPoint,
    LoaderParams,
    GateData
    );
}

EFI_STATUS
GenerateGateData (
  OUT VOID **GateData
  )
{
  ASSERT (GateData != NULL);

  *GateData = PreparePageTable ();
  if (*GateData == NULL) {
    DEBUG ((DEBUG_ERROR, "JOS: Failed to allocate page table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}
