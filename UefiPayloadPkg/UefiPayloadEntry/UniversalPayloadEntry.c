/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

#define MEMORY_ATTRIBUTE_MASK         (EFI_RESOURCE_ATTRIBUTE_PRESENT             | \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED         | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED              | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED      | \
                                       EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED     | \
                                       EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_16_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_32_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_64_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_PERSISTENT          )

#define TESTED_MEMORY_ATTRIBUTES      (EFI_RESOURCE_ATTRIBUTE_PRESENT     | \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

extern VOID  *mHobList;

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
    return;
  }
  NewHob.Header = CreateHob (Hob->Header->HobType, Hob->Header->HobLength);

  if (NewHob.Header != NULL) {
    CopyMem (NewHob.Header + 1, Hob->Header + 1, Hob->Header->HobLength - sizeof (EFI_HOB_GENERIC_HEADER));
  }

}

/**
  Found the Resource Descriptor HOB that contains a range

  @param[in] Base       Memory start address
  @param[in] Top        Memory Top.

  @return     The pointer to the Resource Descriptor HOB.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
FindResourceDescriptorByRange (
  VOID                      *HobList,
  EFI_PHYSICAL_ADDRESS      Base,
  EFI_PHYSICAL_ADDRESS      Top
  )
{
  EFI_PEI_HOB_POINTERS             Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR      *ResourceHob;

  for (Hob.Raw = (UINT8 *) HobList; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    //
    // Skip all HOBs except Resource Descriptor HOBs
    //
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not describe tested system memory
    //
    ResourceHob = Hob.ResourceDescriptor;
    if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
      continue;
    }
    if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not contain the PHIT range EfiFreeMemoryBottom..EfiFreeMemoryTop
    //
    if (Base < ResourceHob->PhysicalStart) {
      continue;
    }
    if (Top > (ResourceHob->PhysicalStart + ResourceHob->ResourceLength)) {
      continue;
    }
    return ResourceHob;
  }
  return NULL;
}

/**
  It will build HOBs based on information from bootloaders.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildHobs (
  IN UINTN                     BootloaderParameter
  )
{
  EFI_PEI_HOB_POINTERS             Hob;
  UINTN                            MinimalNeededSize;
  EFI_PHYSICAL_ADDRESS             FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS             FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS             MemoryBottom;
  EFI_PHYSICAL_ADDRESS             MemoryTop;
  EFI_HOB_RESOURCE_DESCRIPTOR      *ResourceHob;

  Hob.Raw = (UINT8 *) BootloaderParameter;
  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  ASSERT (Hob.Raw != NULL);
  ASSERT ((UINTN) Hob.HandoffInformationTable->EfiFreeMemoryTop == Hob.HandoffInformationTable->EfiFreeMemoryTop);
  ASSERT ((UINTN) Hob.HandoffInformationTable->EfiMemoryTop == Hob.HandoffInformationTable->EfiMemoryTop);
  ASSERT ((UINTN) Hob.HandoffInformationTable->EfiFreeMemoryBottom == Hob.HandoffInformationTable->EfiFreeMemoryBottom);
  ASSERT ((UINTN) Hob.HandoffInformationTable->EfiMemoryBottom == Hob.HandoffInformationTable->EfiMemoryBottom);


  //
  // Try to find Resource Descriptor HOB that contains Hob range EfiMemoryBottom..EfiMemoryTop
  //
  ResourceHob = FindResourceDescriptorByRange(Hob.Raw, Hob.HandoffInformationTable->EfiMemoryBottom, Hob.HandoffInformationTable->EfiMemoryTop);
  if (ResourceHob == NULL) {
    return EFI_NOT_FOUND;
  }

  if (ResourceHob->PhysicalStart + ResourceHob->ResourceLength - Hob.HandoffInformationTable->EfiMemoryTop >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right above memory top in old hob.
    //
    MemoryBottom     = Hob.HandoffInformationTable->EfiFreeMemoryTop;
    FreeMemoryBottom = Hob.HandoffInformationTable->EfiMemoryTop;
    FreeMemoryTop    = FreeMemoryBottom + MinimalNeededSize;
    MemoryTop        = FreeMemoryTop;
    HobConstructor ((VOID *) (UINTN) MemoryBottom, (VOID *) (UINTN) MemoryTop, (VOID *) (UINTN) FreeMemoryBottom, (VOID *) (UINTN) FreeMemoryTop);
  } else if (Hob.HandoffInformationTable->EfiMemoryBottom - ResourceHob->PhysicalStart >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right below memory bottom in old hob.
    //
    MemoryBottom     = Hob.HandoffInformationTable->EfiMemoryBottom - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = Hob.HandoffInformationTable->EfiMemoryBottom;
    MemoryTop        = Hob.HandoffInformationTable->EfiMemoryTop;
    HobConstructor ((VOID *) (UINTN) MemoryBottom, (VOID *) (UINTN) MemoryTop, (VOID *) (UINTN) FreeMemoryBottom, (VOID *) (UINTN) FreeMemoryTop);
  } else {
    //
    // In the Resource Descriptor HOB contains boot loader Hob, there is no enough free memory size for payload hob
    //
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Since payload created new Hob, move all hobs except PHIT from boot loader hob list.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType != EFI_HOB_TYPE_HANDOFF) {
      // Add this hob to payload HOB
      AddNewHob (&Hob);
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  //
  // Use the HOB created in payload entry instead of bootloader HOB
  // since bootloader HOB might be overrided.
  //
  SET_BOOTLOADER_PARAMETER((UINTN) GetHobList ());

  return EFI_SUCCESS;
}

/**
  Entry point to the C language phase of UEFI payload.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN                     BootloaderParameter
  )
{
  EFI_STATUS                    Status;
  PHYSICAL_ADDRESS              DxeCoreEntryPoint;
  EFI_HOB_HANDOFF_INFO_TABLE    *HandoffHobTable;
  EFI_PEI_HOB_POINTERS          Hob;
  PLD_EXTRA_DATA                *ExtraData;
  UINT8                         *GuidHob;
  EFI_FIRMWARE_VOLUME_HEADER    *DxeFv;

  mHobList = (VOID *) BootloaderParameter;
  // Call constructor for all libraries
  ProcessLibraryConstructorList ();

  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof(UINTN)));

  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  // Build HOB based on information from Bootloader
  Status = BuildHobs (BootloaderParameter);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BuildHobs Status = %r\n", Status));
    return Status;
  }

  //
  // Get DXE FV location
  //
  GuidHob = GetFirstGuidHob(&gPldExtraDataGuid);
  ASSERT (GuidHob != NULL);
  ExtraData = (PLD_EXTRA_DATA *) GET_GUID_HOB_DATA (GuidHob);
  ASSERT (ExtraData->Count == 1);
  ASSERT (AsciiStrCmp (ExtraData->Entry[0].Identifier, "uefi_fv") == 0);

  DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) ExtraData->Entry[0].Base;
  ASSERT (DxeFv->FvLength == ExtraData->Entry[0].Size);
  Status = UniversalLoadDxeCore (DxeFv, &DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "DxeCoreEntryPoint = 0x%lx\n", DxeCoreEntryPoint));

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE,  0xFF);

  HandoffHobTable = (EFI_HOB_HANDOFF_INFO_TABLE *) GetFirstHob(EFI_HOB_TYPE_HANDOFF);
  Hob.HandoffInformationTable = HandoffHobTable;
  HandOffToDxeCore (DxeCoreEntryPoint, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
