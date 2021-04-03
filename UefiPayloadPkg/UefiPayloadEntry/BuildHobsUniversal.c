/** @file

  Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

/**
  Add HOB into HOB list

  @param[in]  Hob    The HOB to be added into the HOB list.
**/
VOID
AddNewHob (
  IN EFI_PEI_HOB_POINTERS    *Hob
  )
{
  EFI_PEI_HOB_POINTERS    NewHob;

  if (Hob->Raw == NULL) {
    return ;
  }

  NewHob.Header = CreateHob (Hob->Header->HobType, Hob->Header->HobLength);
  DEBUG ((EFI_D_ERROR, "   Add hob, type = 0x%x, 0x%x\n", Hob->Header->HobType, Hob->Header->HobLength));

  if (NewHob.Header != NULL) {
    CopyMem (NewHob.Header + 1, Hob->Header + 1, Hob->Header->HobLength - sizeof (EFI_HOB_GENERIC_HEADER));
  }

}

/**
  Find the serial port information

  @param  SERIAL_PORT_INFO   Pointer to serial port info structure

  @retval RETURN_SUCCESS     Successfully find the serial port information.
  @retval RETURN_NOT_FOUND   Failed to find the serial port information .

**/
RETURN_STATUS
ParseSerialInfo (
  OUT SERIAL_PORT_INFO     *SerialPortInfo
  )
{
  SERIAL_PORT_INFO         *BlSerialPortInfo;
  UINT8                    *GuidHob;

  GuidHob = GetNextGuidHob (&gUefiSerialPortInfoGuid, (VOID*)(UINTN)GET_BOOTLOADER_PARAMETER (1));
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }
  BlSerialPortInfo  = (SERIAL_PORT_INFO *) GET_GUID_HOB_DATA (GuidHob);
  CopyMem (SerialPortInfo, BlSerialPortInfo, sizeof (SERIAL_PORT_INFO));

  return RETURN_SUCCESS;
}


/**
  It will build HOBs based on information from bootloaders.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildHobs (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_PEI_HOB_POINTERS             Hob;
  ACPI_TABLE_HOB                   *AcpiHob;
  EFI_HOB_GUID_TYPE                *GuidHob;
  ACPI_BOARD_INFO                  AcpiBoardInfo;
  ACPI_BOARD_INFO                  *NewAcpiBoardInfo;

  //
  // Look through the HOB list from bootloader.
  //
  Hob.Raw = (UINT8 *) GET_BOOTLOADER_PARAMETER(1);
  ASSERT (Hob.Raw != NULL);
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType != EFI_HOB_TYPE_HANDOFF) {
      // Add this hob to payload HOB
      AddNewHob (&Hob);
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  //
  // Get info from ACPI table and build a HOB for other modules
  //
  GuidHob  = GetFirstGuidHob (&gEfiAcpiTableGuid); //should use gEfiAcpi20TableGuid?
  if (GuidHob != NULL) {
    AcpiHob  = (ACPI_TABLE_HOB *) GET_GUID_HOB_DATA (GuidHob);
    ASSERT (AcpiHob->TableAddress != 0);

    Status = ParseAcpiInfo (AcpiHob->TableAddress, &AcpiBoardInfo);
    ASSERT_EFI_ERROR (Status);
    NewAcpiBoardInfo = BuildGuidHob (&gUefiAcpiBoardInfoGuid, sizeof (ACPI_BOARD_INFO));
    ASSERT (NewAcpiBoardInfo != NULL);
    CopyMem (NewAcpiBoardInfo, &AcpiBoardInfo, sizeof (ACPI_BOARD_INFO));
    DEBUG ((DEBUG_INFO, "Create acpi board info guid hob\n"));
  }

  //
  // The UEFI payload FV
  //
  BuildMemoryAllocationHob (PcdGet32 (PcdPayloadFdMemBase), PcdGet32 (PcdPayloadFdMemSize), EfiBootServicesData);

  // Use the HOB created in payload entry instead of bootloader HOB
  // since bootloader HOB might be overrided.
  SET_BOOTLOADER_PARAMETER(1, (UINTN)GetHobList());

  return EFI_SUCCESS;
}
