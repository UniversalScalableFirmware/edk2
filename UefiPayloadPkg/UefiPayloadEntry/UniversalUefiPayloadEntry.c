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
  EFI_PEI_HOB_POINTERS             Hob;

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
  // The UEFI payload FV
  //
  BuildMemoryAllocationHob (PcdGet32 (PcdPayloadFdMemBase), PcdGet32 (PcdPayloadFdMemSize), EfiBootServicesData);

  // Use the HOB created in payload entry instead of bootloader HOB
  // since bootloader HOB might be overrided.
  SET_BOOTLOADER_PARAMETER(1, (UINTN)GetHobList());

  return EFI_SUCCESS;
}


/**
  Entry point to the C language phase of UEFI payload.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
PayloadEntry (
  IN UINTN                     BootloaderParameter
  )
{
  EFI_STATUS                    Status;
  PHYSICAL_ADDRESS              DxeCoreEntryPoint;
  EFI_HOB_HANDOFF_INFO_TABLE    *HandoffHobTable;
  UINTN                         MemBase;
  UINTN                         MemSize;
  UINTN                         HobMemBase;
  UINTN                         HobMemTop;
  EFI_PEI_HOB_POINTERS          Hob;
  SERIAL_PORT_INFO              SerialPortInfo;



  Status = ParseSerialInfo (&SerialPortInfo);
  if (!EFI_ERROR (Status)) {
    CopyMem (GET_BOOTLOADER_PARAMETER_ADDR(3), &SerialPortInfo, sizeof(SERIAL_PORT_INFO));
  }

  // Call constructor for all libraries
  ProcessLibraryConstructorList ();
  DEBUG ((DEBUG_INFO, "GET_BOOTLOADER_PARAMETER() = 0x%lx\n", GET_BOOTLOADER_PARAMETER(1)));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof(UINTN)));

  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  // HOB region is used for HOB and memory allocation for this module
  MemBase    = PcdGet32 (PcdPayloadFdMemBase);
  HobMemBase = ALIGN_VALUE (MemBase + PcdGet32 (PcdPayloadFdMemSize), SIZE_1MB);
  HobMemTop  = HobMemBase + FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  // DXE core assumes the memory below HOB region could be used, so include the FV region memory into HOB range.
  MemSize    = HobMemTop - MemBase;
  HandoffHobTable = HobConstructor ((VOID *)MemBase, MemSize, (VOID *)HobMemBase, (VOID *)HobMemTop);

  // Build HOB based on information from Bootloader
  Status = BuildHobs ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BuildHobs Status = %r\n", Status));
    return Status;
  }

  // Load the DXE Core
  Status = LoadDxeCore (&DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "DxeCoreEntryPoint = 0x%lx\n", DxeCoreEntryPoint));

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE,  0xFF);

  Hob.HandoffInformationTable = HandoffHobTable;
  HandOffToDxeCore (DxeCoreEntryPoint, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
