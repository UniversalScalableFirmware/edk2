/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"
#include "UniversalPayloadCommon.h"
#include <Library/GetUplDataLib.h>

EFI_STATUS
InitHobList (
  )
{
  UINTN                  MemoryMapCount;
  UINTN                  MemoryMapNumber;
  EFI_MEMORY_DESCRIPTOR  MemoryMap;
  UINTN                  FindIndex;
  UINTN                  Index;
  EFI_PHYSICAL_ADDRESS   MemoryBottom;
  EFI_PHYSICAL_ADDRESS   MemoryTop;
  EFI_PHYSICAL_ADDRESS   TempPhysicalStart;
  EFI_STATUS             Status;

  MemoryMapCount    = 0;
  TempPhysicalStart = 0;
  Status            = GetUplMemoryMap (&MemoryMap, &MemoryMapCount, 0);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_NOT_FOUND;
  }

  FindIndex = MemoryMapCount;
  DEBUG ((DEBUG_INFO, "MemoryMapCount%x\n", MemoryMapCount));
  for (Index = 0; Index < MemoryMapCount; Index++) {
    DEBUG ((DEBUG_INFO, "Index%x\n", Index));
    MemoryMapNumber = 1;
    Status          =  GetUplMemoryMap (&MemoryMap, &MemoryMapNumber, Index);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }

    DEBUG ((DEBUG_INFO, "MemoryMap.PhysicalStart %lx\n", MemoryMap.PhysicalStart));
    DEBUG ((DEBUG_INFO, "MemoryMap.NumberOfPages %lx %lx\n", MemoryMap.NumberOfPages, FixedPcdGet32 (PcdSystemMemoryUefiRegionSize)));
    DEBUG ((DEBUG_INFO, "MemoryMap.Type %lx\n", MemoryMap.Type));
    //
    // Skip above 4G memory
    //
    if (MemoryMap.PhysicalStart + EFI_PAGES_TO_SIZE (MemoryMap.NumberOfPages) > BASE_4GB) {
      continue;
    }

    //
    // Make sure memory range is free and enough to contain PcdSystemMemoryUefiRegionSize
    //
    if ((MemoryMap.Type != EfiConventionalMemory) || (EFI_PAGES_TO_SIZE (MemoryMap.NumberOfPages) < FixedPcdGet32 (PcdSystemMemoryUefiRegionSize))) {
      continue;
    }

    //
    // Choose the highest range among all ranges meet the above requirement
    //
    if (MemoryMap.PhysicalStart < TempPhysicalStart) {
      continue;
    }

    TempPhysicalStart = MemoryMap.PhysicalStart;

    FindIndex = Index;
  }

  if (FindIndex == MemoryMapCount) {
    return EFI_NOT_FOUND;
  }

  MemoryMapNumber = 1;
  Status          = GetUplMemoryMap (&MemoryMap, &MemoryMapNumber, FindIndex);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  MemoryTop    = MemoryMap.PhysicalStart + EFI_PAGES_TO_SIZE (MemoryMap.NumberOfPages);
  MemoryBottom = MemoryTop - FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)MemoryTop, (VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)MemoryTop);

  return EFI_SUCCESS;
}

EFI_STATUS
CreateHobsBasedOnMemoryMap10 (
  VOID
  );

EFI_STATUS
CreateSerialPortInfoHobBasedOnUplData (
  VOID
  )
{
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *Serial;
  UINT64                              BaudRate;
  UINT64                              RegisterBase;
  UINT64                              RegisterStride;
  BOOLEAN                             UseMmio;
  EFI_STATUS                          Status;

  Serial                  = BuildGuidHob (&gUniversalPayloadSerialPortInfoGuid, sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO));
  Serial->Header.Revision = UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION;
  Serial->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO);
  Status                  =  GetUplInteger (UPL_KEY_SERIAL_PORT_BAUDRATE, &BaudRate);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Serial->BaudRate = (UINT32)BaudRate;
  Status           =  GetUplInteger (UPL_KEY_SERIAL_PORT_REGISTER_BASE, &RegisterBase);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Serial->RegisterBase = RegisterBase;

  Status =  GetUplInteger (UPL_KEY_SERIAL_PORT_REGISTER_STRIDE, &RegisterStride);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  ASSERT((UINT8)RegisterStride == RegisterStride);
  Serial->RegisterStride = (UINT8)RegisterStride;

  Status =  GetUplBoolean (UPL_KEY_SERIAL_PORT_USE_MMIO, &UseMmio);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Serial->UseMmio = UseMmio;

  return EFI_SUCCESS;
}

EFI_STATUS
CreateAcpiTableHobBasedOnUplData (
  VOID
  )
{
  UINT64                        Rsdp;
  ACPI_BOARD_INFO               *AcpiBoardInfo;
  UNIVERSAL_PAYLOAD_ACPI_TABLE  *AcpiTableHob;
  EFI_STATUS                    Status;

  Status = GetUplInteger (UPL_KEY_ACPI_TABLE_RSDP, &Rsdp);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UPL data doesn't contain HobList\n"));
  } else {
    AcpiTableHob                  = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)BuildGuidHob (&gUniversalPayloadAcpiTableGuid, sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE));
    AcpiTableHob->Rsdp            = (EFI_PHYSICAL_ADDRESS)Rsdp;
    AcpiTableHob->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE);
    AcpiTableHob->Header.Revision = UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION;
    AcpiBoardInfo                 = BuildHobFromAcpi (Rsdp);
    ASSERT (AcpiBoardInfo != NULL);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
CreateCpuHobBasedOnUplData (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      SizeOfMemorySpace;
  UINT64      SizeOfIoSpace;

  Status = GetUplInteger (UPL_KEY_MEMORY_SPACE, &SizeOfMemorySpace);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UPL data doesn't contain MemorySpace\n"));
    return EFI_NOT_FOUND;
  }

  Status = GetUplInteger (UPL_KEY_IO_SPACE, &SizeOfIoSpace);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UPL data doesn't contain IoSpace\n"));
    return EFI_NOT_FOUND;
  }

  ASSERT ((UINT8)SizeOfMemorySpace == SizeOfMemorySpace);
  ASSERT ((UINT8)SizeOfIoSpace == SizeOfIoSpace);
  BuildCpuHob ((UINT8)SizeOfMemorySpace, (UINT8)SizeOfIoSpace);
  return EFI_SUCCESS;
}

EFI_STATUS
CreatePciRootBridgeHobBasedOnUplData (
  VOID
  )
{
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE   *RootBridge;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridgeInfo;
  BOOLEAN                             ResourceAssigned;
  UINTN                               Count;
  EFI_STATUS                          Status;

  Count  = 0;
  Status = GetUplPciRootBridges (NULL, &Count, 0);
  if (Status != RETURN_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_INFO, "UPL data doesn't contain PciRootBridges info\n"));
  } else {
    RootBridge = AllocatePool (Count * sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));

    Status = GetUplPciRootBridges (RootBridge, &Count, 0);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }

    DEBUG ((DEBUG_INFO, "GetUplPciRootBridges... %d\n", Count));

    PciRootBridgeInfo = BuildGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid, sizeof (PciRootBridgeInfo) + sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));
    CopyMem (PciRootBridgeInfo->RootBridge, RootBridge, sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));
    PciRootBridgeInfo->Count           = (UINT8)Count;
    PciRootBridgeInfo->Header.Length   = (UINT16)(Count * sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE) + sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES));
    PciRootBridgeInfo->Header.Revision = UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION;

    Status = GetUplBoolean (UPL_KEY_ROOT_BRIDGE_RESOURCE_ASSIGNED, &ResourceAssigned);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "UPL data doesn't contain ResourceAssigned\n"));
      ResourceAssigned = FALSE;
    }

    PciRootBridgeInfo->ResourceAssigned = ResourceAssigned;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
CreateFvHobsBasedOnUplData (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY  *ExtraData;
  UINTN                               Count;
  UINTN                               Index;

  //
  // Get DXE FV location
  //
  Count = 0;
  GetUplExtraData (NULL, &Count, 0);
  ExtraData = AllocatePool (Count * sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY));

  GetUplExtraData (ExtraData, &Count, 0);
  for (Index = 0; Index < Count; Index++) {
    if (AsciiStrCmp (ExtraData[Index].Identifier, "uefi_fv") == 0) {
      *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)ExtraData[Index].Base;
      ASSERT ((*DxeFv)->FvLength == ExtraData[Index].Size);
      BuildFvHob ((EFI_PHYSICAL_ADDRESS)(UINTN)*DxeFv, (*DxeFv)->FvLength);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
CreateOtherHobsBasedOnUplData (
  VOID
  )
{
  EFI_STATUS            Status;
  UINT64                HobList;
  EFI_PEI_HOB_POINTERS  Hob;

  Status = GetUplInteger ("HobList", &HobList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UPL data doesn't contain HobList\n"));
  } else {
    Hob.Raw = (VOID *)(UINTN)HobList;
    PrintHob (Hob.Raw);
    //
    // Since payload created new Hob, move all hobs except PHIT from boot loader hob list.
    //
    while (!END_OF_HOB_LIST (Hob)) {
      if (IsHobNeed (Hob)) {
        // Add this hob to payload HOB
        AddNewHob (&Hob);
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
    }
  }

  //
  // Create guid hob for acpi board information
  //
  Status = CreateAcpiTableHobBasedOnUplData ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Build CPU Hob
  //
  Status = CreateCpuHobBasedOnUplData ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Build gUniversalPayloadPciRootBridgeInfoGuid Hob
  //
  CreatePciRootBridgeHobBasedOnUplData ();

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN  BootloaderParameter
  )
{
  EFI_STATUS                  Status;
  EFI_FIRMWARE_VOLUME_HEADER  *DxeFv;
  PHYSICAL_ADDRESS            DxeCoreEntryPoint;
  EFI_PEI_HOB_POINTERS        HobList;

  Status = InitUplFromBuffer ((UINTN *)BootloaderParameter);
  ASSERT_EFI_ERROR (Status);

  InitHobList ();
  Status = CreateSerialPortInfoHobBasedOnUplData ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Serial port information is not found\n"));
  }

  // Call constructor for all libraries
  ProcessLibraryConstructorList ();
  DEBUG ((DEBUG_INFO, "Entering Universal Payload...\n"));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof (UINTN)));
  InitializeFloatingPointUnits ();

  Status = CreateHobsBasedOnMemoryMap10 ();
  ASSERT_EFI_ERROR (Status);

  Status = CreateFvHobsBasedOnUplData (&DxeFv);
  ASSERT_EFI_ERROR (Status);

  Status = CreateOtherHobsBasedOnUplData ();
  ASSERT_EFI_ERROR (Status);

  FixUpPcdDatabase (DxeFv);
  Status = UniversalLoadDxeCore (DxeFv, &DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, 0xFF);

  HobList.Raw = GetHobList ();
  HandOffToDxeCore (DxeCoreEntryPoint, HobList);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
