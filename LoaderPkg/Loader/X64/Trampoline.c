/** @file
  Copyright (c) 2020, ISP RAS. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "Bootloader.h"

EFI_STATUS
GenerateGateData (
  OUT VOID **GateData
  )
{
  ASSERT (GateData != NULL);
  //
  // No need for any extra data in 64-bit mode.
  //
  *GateData = NULL;
  return EFI_SUCCESS;
}

VOID
EFIAPI
CallKernelThroughGate (
  IN UINTN          EntryPoint,
  IN LOADER_PARAMS  *LoaderParams,
  IN VOID           *GateData
  )
{
  KERNEL_ENTRY  KernelEntry;

  ASSERT (LoaderParams != NULL);
  ASSERT (GateData == NULL);

  KernelEntry = (KERNEL_ENTRY) EntryPoint;
  KernelEntry (LoaderParams, GateData);
}
