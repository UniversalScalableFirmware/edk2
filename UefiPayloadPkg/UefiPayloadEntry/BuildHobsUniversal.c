/** @file

  Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"
#define GET_BOOTLOADER_PARAMETER()      (*(UINTN *)(UINTN)(PcdGet32(PcdPayloadStackTop) - sizeof(UINT64)))
#define SET_BOOTLOADER_PARAMETER(Value) GET_BOOTLOADER_PARAMETER()=Value


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
  ResourceHob = FindResourceDescriptorByRange(
                  Hob.Raw,
                  Hob.HandoffInformationTable->EfiMemoryBottom,
                  Hob.HandoffInformationTable->EfiMemoryTop
                  );
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
  } else if (Hob.HandoffInformationTable->EfiMemoryBottom - ResourceHob->PhysicalStart >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right below memory bottom in old hob.
    //
    MemoryBottom     = Hob.HandoffInformationTable->EfiMemoryBottom - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = Hob.HandoffInformationTable->EfiMemoryBottom;
    MemoryTop        = Hob.HandoffInformationTable->EfiMemoryTop;
  } else {
    //
    // In the Resource Descriptor HOB contains boot loader Hob, there is no enough free memory size for payload hob
    //
    return EFI_OUT_OF_RESOURCES;
  }
  HobConstructor (MemoryBottom, MemoryTop, FreeMemoryBottom, FreeMemoryTop);

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
