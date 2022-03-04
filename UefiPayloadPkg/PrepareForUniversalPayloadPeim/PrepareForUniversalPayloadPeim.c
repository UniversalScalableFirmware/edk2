/** @file
  Do necessary preparation for Universal Payload

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/PrepareForUniversalPayload.h>
#include <Library/MemoryMapLib.h>

VOID *
EFIAPI
PrepareForUniversalPayload (
  VOID  *HobStart
  )
{
  RETURN_STATUS  Status;

  DEBUG ((DEBUG_INFO, "Begin to do necessary preparation for Universal Payload\n"));
  Status = BuildMemoryMapHob ();
  if (RETURN_ERROR (Status)) {
    return NULL;
  }

  //
  // Today the UPL interface is still HOB based.
  // The Memory Map information is created in HOB
  //
  return HobStart;
}

EDKII_PREPARE_FOR_UNIVERSAL_PAYLOAD_PPI  gPrepareForUniveralPayloadPpi = {
  PrepareForUniversalPayload
};

EFI_PEI_PPI_DESCRIPTOR  gPrepareForUniversalPayloadPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiPrepareForUniversalPayloadPpiGuid,
  &gPrepareForUniveralPayloadPpi
};

/**

  Install Prepare For Universal Payload PPI.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point executes successfully.
  @retval Others      Some error occurs during the execution of this function.

**/
EFI_STATUS
EFIAPI
InitializePrepareForUniversalPayload (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesInstallPpi (&gPrepareForUniversalPayloadPpiList);

  return Status;
}
