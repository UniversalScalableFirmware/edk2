/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi/UefiSpec.h>
#include <UniversalPayload/MemoryMap.h>
#include <Library/MemoryMapLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/GetUplDataLib.h>

#define MEMORY_ATTRIBUTE_MASK  (EFI_RESOURCE_ATTRIBUTE_PRESENT             |        \
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

#define TESTED_MEMORY_ATTRIBUTES  (EFI_RESOURCE_ATTRIBUTE_PRESENT     |     \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

typedef struct {
  UINT64    Attribute;
  UINT64    Capability;
} ATTRIBUTE_CONVERSION_ENTRY;

STATIC ATTRIBUTE_CONVERSION_ENTRY  mAttributeConversionTable[] = {
  { EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE,             EFI_MEMORY_UC            },
  { EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED,       EFI_MEMORY_UCE           },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE,       EFI_MEMORY_WC            },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE, EFI_MEMORY_WT            },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE,    EFI_MEMORY_WB            },
  { EFI_RESOURCE_ATTRIBUTE_READ_PROTECTABLE,        EFI_MEMORY_RP            },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE,       EFI_MEMORY_WP            },
  { EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE,   EFI_MEMORY_XP            },
  { EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE,   EFI_MEMORY_RO            },
  { EFI_RESOURCE_ATTRIBUTE_PERSISTABLE,             EFI_MEMORY_NV,           },
  { EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE,           EFI_MEMORY_MORE_RELIABLE },
  { 0,                                              0                        }
};

/**
  Build a Handoff Information Table HOB

  This function initialize a HOB region from EfiMemoryBegin to
  EfiMemoryTop. And EfiFreeMemoryBottom and EfiFreeMemoryTop should
  be inside the HOB region.

  @param[in] EfiMemoryBottom       Total memory start address
  @param[in] EfiMemoryTop          Total memory end address.
  @param[in] EfiFreeMemoryBottom   Free memory start address
  @param[in] EfiFreeMemoryTop      Free memory end address.

  @return   The pointer to the handoff HOB table.

**/
EFI_HOB_HANDOFF_INFO_TABLE *
EFIAPI
HobConstructor (
  IN VOID  *EfiMemoryBottom,
  IN VOID  *EfiMemoryTop,
  IN VOID  *EfiFreeMemoryBottom,
  IN VOID  *EfiFreeMemoryTop
  );

EFI_RESOURCE_ATTRIBUTE_TYPE
ConvertCapabilitiesToResourceDescriptorHobAttributes (
  UINT64  Capabilities
  )
{
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attributes;
  ATTRIBUTE_CONVERSION_ENTRY   *Conversion;

  for (Attributes = 0, Conversion = mAttributeConversionTable; Conversion->Attribute != 0; Conversion++) {
    if (Capabilities & Conversion->Capability) {
      Attributes |= Conversion->Attribute;
    }
  }

  Attributes |= (EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED | EFI_RESOURCE_ATTRIBUTE_PRESENT);
  return Attributes;
}

EFI_RESOURCE_TYPE
ConvertEfiMemoryTypeToResourceDescriptorHobResourceType (
  EFI_MEMORY_TYPE  Type
  )
{
  switch (Type) {
    case EfiConventionalMemory:
      return EFI_RESOURCE_SYSTEM_MEMORY;

    case EfiReservedMemoryType:
      return EFI_RESOURCE_MEMORY_RESERVED;

    default:
      return EFI_RESOURCE_SYSTEM_MEMORY;
  }
}

/**
  Function to compare 2 Memory Allocation Hob.

  @param[in] Buffer1            pointer to the firstMemory Allocation Hob to compare
  @param[in] Buffer2            pointer to the second Memory Allocation Hob to compare

  @retval 0                     Buffer1 equal to Buffer2
  @retval <0                    Buffer1 is less than Buffer2
  @retval >0                    Buffer1 is greater than Buffer2
**/
INTN
EFIAPI
MemoryMapTableCompare (
  IN CONST VOID  *Buffer1,
  IN CONST VOID  *Buffer2
  )
{
  EFI_MEMORY_DESCRIPTOR  *Left;
  EFI_MEMORY_DESCRIPTOR  *Right;

  Left  = (EFI_MEMORY_DESCRIPTOR *)Buffer1;
  Right = (EFI_MEMORY_DESCRIPTOR *)Buffer2;

  if (Left->PhysicalStart == Right->PhysicalStart) {
    return 0;
  } else if (Left->PhysicalStart > Right->PhysicalStart) {
    return 1;
  } else {
    return -1;
  }
}

/**
  Create resource Hob that Uefi can use based on Memory Map Table.
  This function assume the memory map table is sorted, and doesn't have overlap with each other.
  Also this function assume the memory map table only contains physical memory range.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
RETURN_STATUS
CreateHobsBasedOnMemoryMap (
  IN UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  UINTN                        Index;
  EFI_MEMORY_DESCRIPTOR        *MemMapTable;
  EFI_MEMORY_DESCRIPTOR        *SortedMemMapTable;
  EFI_MEMORY_DESCRIPTOR        SortBuffer;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceDescriptor;
  EFI_PHYSICAL_ADDRESS         MemMapTableLimit;

  if (MemoryMapHob->DescriptorSize < sizeof (EFI_MEMORY_DESCRIPTOR)) {
    ASSERT (MemoryMapHob->DescriptorSize >= sizeof (EFI_MEMORY_DESCRIPTOR));
    return RETURN_UNSUPPORTED;
  }

  SortedMemMapTable = AllocatePages (EFI_SIZE_TO_PAGES (MemoryMapHob->DescriptorSize * MemoryMapHob->Count));
  if (SortedMemMapTable == NULL) {
    ASSERT (SortedMemMapTable != NULL);
    return RETURN_UNSUPPORTED;
  }

  SortedMemMapTable = CopyMem (SortedMemMapTable, MemoryMapHob->MemoryMap, MemoryMapHob->DescriptorSize * MemoryMapHob->Count);
  QuickSort (SortedMemMapTable, MemoryMapHob->Count, MemoryMapHob->DescriptorSize, MemoryMapTableCompare, &SortBuffer);
  for (Index = 0; Index < MemoryMapHob->Count; Index++) {
    MemMapTable = (EFI_MEMORY_DESCRIPTOR *)(((UINT8 *)SortedMemMapTable) + Index * MemoryMapHob->DescriptorSize);
    if ((Index != 0) && (MemMapTable->PhysicalStart < MemMapTableLimit)) {
      ASSERT (MemMapTable->PhysicalStart >= MemMapTableLimit);
      return RETURN_UNSUPPORTED;
    }

    if ((MemMapTable->PhysicalStart & (EFI_PAGE_SIZE - 1)) != 0) {
      ASSERT ((MemMapTable->PhysicalStart & (EFI_PAGE_SIZE - 1)) == 0);
      return RETURN_UNSUPPORTED;
    }

    if (MemMapTable->NumberOfPages == 0) {
      continue;
    }

    MemMapTableLimit = MemMapTable->PhysicalStart + EFI_PAGES_TO_SIZE (MemMapTable->NumberOfPages);
    if (MemMapTableLimit < MemMapTable->PhysicalStart) {
      ASSERT (MemMapTableLimit >= MemMapTable->PhysicalStart);
      return RETURN_UNSUPPORTED;
    }

    Hob.Raw            = GetHobList ();
    ResourceDescriptor = NULL;
    while (!END_OF_HOB_LIST (Hob)) {
      if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
        ResourceDescriptor = Hob.ResourceDescriptor;
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
    }

    //
    // Every Memory map range should be contained in one Resource Descriptor Hob.
    //
    if ((ResourceDescriptor != NULL) &&
        (ResourceDescriptor->ResourceAttribute == ConvertCapabilitiesToResourceDescriptorHobAttributes (MemMapTable->Attribute)) &&
        (ResourceDescriptor->ResourceType == ConvertEfiMemoryTypeToResourceDescriptorHobResourceType (MemMapTable->Type)) &&
        (ResourceDescriptor->PhysicalStart + ResourceDescriptor->ResourceLength == MemMapTable->PhysicalStart))
    {
      ResourceDescriptor->ResourceLength += EFI_PAGES_TO_SIZE (MemMapTable->NumberOfPages);
    } else {
      BuildResourceDescriptorHob (
        ConvertEfiMemoryTypeToResourceDescriptorHobResourceType (MemMapTable->Type),
        ConvertCapabilitiesToResourceDescriptorHobAttributes (MemMapTable->Attribute),
        MemMapTable->PhysicalStart,
        EFI_PAGES_TO_SIZE (MemMapTable->NumberOfPages)
        );
    }

    //
    // Every used Memory map range should be contained in Memory Allocation Hob.
    //
    if (MemMapTable->Type != EfiConventionalMemory) {
      BuildMemoryAllocationHob (
        MemMapTable->PhysicalStart,
        EFI_PAGES_TO_SIZE (MemMapTable->NumberOfPages),
        MemMapTable->Type
        );
    }
  }

  return RETURN_SUCCESS;
}

EFI_STATUS
FindFreeMemoryFromMemoryMapTable (
  IN  EFI_PEI_HOB_POINTERS  HobStart,
  IN  UINTN                 MinimalNeededSize,
  OUT EFI_PHYSICAL_ADDRESS  *FreeMemoryBottom,
  OUT EFI_PHYSICAL_ADDRESS  *FreeMemoryTop,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryBottom,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryTop
  )
{
  VOID                          *Hob;
  UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob;
  EFI_MEMORY_DESCRIPTOR         *MemoryMap;
  UINTN                         FindIndex;
  EFI_PHYSICAL_ADDRESS          TempPhysicalStart;
  UINTN                         Index;

  TempPhysicalStart = 0;
  Hob = GetNextGuidHob (&gUniversalPayloadMemoryMapGuid, HobStart.Raw);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  MemoryMapHob = (UNIVERSAL_PAYLOAD_MEMORY_MAP *)GET_GUID_HOB_DATA (Hob);
  FindIndex    = MemoryMapHob->Count;
  for (Index = 0; Index < MemoryMapHob->Count; Index++) {
    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryMapHob->MemoryMap) + Index * MemoryMapHob->DescriptorSize);

    //
    // Skip above 4G memory
    //
    if (MemoryMap->PhysicalStart + EFI_PAGES_TO_SIZE (MemoryMap->NumberOfPages) > BASE_4GB) {
      continue;
    }

    //
    // Make sure memory range is free and enough to contain PcdSystemMemoryUefiRegionSize
    //
    if ((MemoryMap->Type != EfiConventionalMemory) || (EFI_PAGES_TO_SIZE (MemoryMap->NumberOfPages) < MinimalNeededSize)) {
      continue;
    }

    //
    // Choose the highest range among all ranges meet the above requirement
    //
    if (MemoryMap->PhysicalStart < TempPhysicalStart) {
      continue;
    }

    TempPhysicalStart = MemoryMap->PhysicalStart;
    FindIndex = Index;
  }

  if (FindIndex == MemoryMapHob->Count) {
    return EFI_NOT_FOUND;
  }

  MemoryMap         = ((EFI_MEMORY_DESCRIPTOR *)MemoryMapHob->MemoryMap) + FindIndex;
  *MemoryTop        = EFI_PAGES_TO_SIZE (MemoryMap->NumberOfPages) + MemoryMap->PhysicalStart;
  *FreeMemoryTop    = *MemoryTop;
  *MemoryBottom     = *MemoryTop - MinimalNeededSize;
  *FreeMemoryBottom = *MemoryBottom;
  return EFI_SUCCESS;
}

/**
  Find the highest below 4G memory resource descriptor, except the input Resource Descriptor.

  @param[in] HobList                 Hob start address
  @param[in] MinimalNeededSize       Minimal needed size.
  @param[in] ExceptResourceHob       Ignore this Resource Descriptor.

  @retval     The pointer to the Resource Descriptor HOB.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
FindAnotherHighestBelow4GResourceDescriptor (
  IN VOID                         *HobList,
  IN UINTN                        MinimalNeededSize,
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *ExceptResourceHob
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ReturnResourceHob;

  ReturnResourceHob = NULL;

  for (Hob.Raw = (UINT8 *)HobList; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
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
    // Skip if the Resource Descriptor HOB equals to ExceptResourceHob
    //
    if (ResourceHob == ExceptResourceHob) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that are beyond 4G
    //
    if ((ResourceHob->PhysicalStart + ResourceHob->ResourceLength) > BASE_4GB) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that are too small
    //
    if (ResourceHob->ResourceLength < MinimalNeededSize) {
      continue;
    }

    //
    // Return the topest Resource Descriptor
    //
    if (ReturnResourceHob == NULL) {
      ReturnResourceHob = ResourceHob;
    } else {
      if (ReturnResourceHob->PhysicalStart < ResourceHob->PhysicalStart) {
        ReturnResourceHob = ResourceHob;
      }
    }
  }

  return ReturnResourceHob;
}

/**
  Found the Resource Descriptor HOB that contains a range (Base, Top)

  @param[in] HobList    Hob start address
  @param[in] Base       Memory start address
  @param[in] Top        Memory end address.

  @retval     The pointer to the Resource Descriptor HOB.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
FindResourceDescriptorByRange (
  IN VOID                  *HobList,
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN EFI_PHYSICAL_ADDRESS  Top
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;

  for (Hob.Raw = (UINT8 *)HobList; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
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

EFI_STATUS
FindFreeMemoryFromResourceDescriptorHob (
  IN  EFI_PEI_HOB_POINTERS  HobStart,
  IN  UINTN                 MinimalNeededSize,
  OUT EFI_PHYSICAL_ADDRESS  *FreeMemoryBottom,
  OUT EFI_PHYSICAL_ADDRESS  *FreeMemoryTop,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryBottom,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryTop
  )
{
  EFI_HOB_RESOURCE_DESCRIPTOR  *PhitResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;

  ASSERT (HobStart.Raw != NULL);
  ASSERT ((UINTN)HobStart.HandoffInformationTable->EfiFreeMemoryTop == HobStart.HandoffInformationTable->EfiFreeMemoryTop);
  ASSERT ((UINTN)HobStart.HandoffInformationTable->EfiMemoryTop == HobStart.HandoffInformationTable->EfiMemoryTop);
  ASSERT ((UINTN)HobStart.HandoffInformationTable->EfiFreeMemoryBottom == HobStart.HandoffInformationTable->EfiFreeMemoryBottom);
  ASSERT ((UINTN)HobStart.HandoffInformationTable->EfiMemoryBottom == HobStart.HandoffInformationTable->EfiMemoryBottom);

  //
  // Try to find Resource Descriptor HOB that contains Hob range EfiMemoryBottom..EfiMemoryTop
  //
  PhitResourceHob = FindResourceDescriptorByRange (HobStart.Raw, HobStart.HandoffInformationTable->EfiMemoryBottom, HobStart.HandoffInformationTable->EfiMemoryTop);
  if (PhitResourceHob == NULL) {
    //
    // Boot loader's Phit Hob is not in an available Resource Descriptor, find another Resource Descriptor for new Phit Hob
    //
    ResourceHob = FindAnotherHighestBelow4GResourceDescriptor (HobStart.Raw, MinimalNeededSize, NULL);
    if (ResourceHob == NULL) {
      return EFI_NOT_FOUND;
    }

    *MemoryBottom     = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MinimalNeededSize;
    *FreeMemoryBottom = *MemoryBottom;
    *FreeMemoryTop    = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    *MemoryTop        = *FreeMemoryTop;
  } else if (PhitResourceHob->PhysicalStart + PhitResourceHob->ResourceLength - HobStart.HandoffInformationTable->EfiMemoryTop >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right above memory top in old hob.
    //
    *MemoryBottom     = HobStart.HandoffInformationTable->EfiFreeMemoryTop;
    *FreeMemoryBottom = HobStart.HandoffInformationTable->EfiMemoryTop;
    *FreeMemoryTop    = *FreeMemoryBottom + MinimalNeededSize;
    *MemoryTop        = *FreeMemoryTop;
  } else if (HobStart.HandoffInformationTable->EfiMemoryBottom - PhitResourceHob->PhysicalStart >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right below memory bottom in old hob.
    //
    *MemoryBottom     = HobStart.HandoffInformationTable->EfiMemoryBottom - MinimalNeededSize;
    *FreeMemoryBottom = *MemoryBottom;
    *FreeMemoryTop    = HobStart.HandoffInformationTable->EfiMemoryBottom;
    *MemoryTop        = HobStart.HandoffInformationTable->EfiMemoryTop;
  } else {
    //
    // In the Resource Descriptor HOB contains boot loader Hob, there is no enough free memory size for payload hob
    // Find another Resource Descriptor Hob
    //
    ResourceHob = FindAnotherHighestBelow4GResourceDescriptor (HobStart.Raw, MinimalNeededSize, PhitResourceHob);
    if (ResourceHob == NULL) {
      return EFI_NOT_FOUND;
    }

    *MemoryBottom     = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MinimalNeededSize;
    *FreeMemoryBottom = *MemoryBottom;
    *FreeMemoryTop    = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    *MemoryTop        = *FreeMemoryTop;
  }

  return EFI_SUCCESS;
}

/**
  If the hob list created by bootloader contains Memory allocation hobs or
  resource descriptor hobs, they have high priority than hobs created based
  on memory map. Revmove the duplicated hobs.

  @param[in]  OldHob         The hob list created by bootloader.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
RemoveDuplicatedMemoryHobs (
  IN EFI_PEI_HOB_POINTERS  OldHob
  )
{
  EFI_PEI_HOB_POINTERS  OldHobCursor;
  EFI_PEI_HOB_POINTERS  NewHobCursor;
  EFI_PHYSICAL_ADDRESS  OldBase;
  EFI_PHYSICAL_ADDRESS  OldLimit;
  EFI_PHYSICAL_ADDRESS  NewBase;
  EFI_PHYSICAL_ADDRESS  NewLimit;

  OldHobCursor = OldHob;
  while (!END_OF_HOB_LIST (OldHobCursor)) {
    if ((OldHobCursor.Header->HobType != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) &&
        (OldHobCursor.Header->HobType != EFI_HOB_TYPE_MEMORY_ALLOCATION))
    {
      OldHobCursor.Raw = GET_NEXT_HOB (OldHobCursor);
      continue;
    }

    //
    // One resource descriptor hob is found in hoblist created by bootloader.
    // Modify the Hob created based on memory map to avoid overlap among hobs
    //
    if (OldHobCursor.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      OldBase          = OldHobCursor.ResourceDescriptor->PhysicalStart;
      OldLimit         = OldBase + OldHobCursor.ResourceDescriptor->ResourceLength;
      NewHobCursor.Raw = GetHobList ();
      NewHobCursor.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, NewHobCursor.Raw);
      while (NewHobCursor.Raw != NULL) {
        NewBase  = NewHobCursor.ResourceDescriptor->PhysicalStart;
        NewLimit = NewBase + NewHobCursor.ResourceDescriptor->ResourceLength;
        if ((OldBase == NewBase) && (OldLimit == NewLimit)) {
          NewHobCursor.ResourceDescriptor->Header.HobType = EFI_HOB_TYPE_UNUSED;
        } else if ((OldBase == NewBase) && (OldLimit < NewLimit)) {
          NewHobCursor.ResourceDescriptor->PhysicalStart  = OldLimit;
          NewHobCursor.ResourceDescriptor->ResourceLength = NewLimit - OldLimit;
        } else if ((OldBase > NewBase) && (OldLimit == NewLimit)) {
          NewHobCursor.ResourceDescriptor->ResourceLength = OldBase - NewBase;
        } else if ((OldBase > NewBase) && (OldLimit < NewLimit)) {
          NewHobCursor.ResourceDescriptor->ResourceLength = OldBase - NewBase;
          BuildResourceDescriptorHob (
            NewHobCursor.ResourceDescriptor->ResourceType,
            NewHobCursor.ResourceDescriptor->ResourceAttribute,
            OldLimit,
            NewLimit - OldLimit
            );
        } else {
          NewHobCursor.Raw = GET_NEXT_HOB (NewHobCursor.Raw);
          NewHobCursor.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, NewHobCursor.Raw);
          continue;
        }

        break;
      }
    }

    //
    // One memory allocation hob is found in hoblist created by bootloader.
    // Modify the Hob created based on memory map to avoid overlap among hobs
    //
    if (OldHobCursor.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      OldBase          = OldHobCursor.MemoryAllocation->AllocDescriptor.MemoryBaseAddress;
      OldLimit         = OldBase + OldHobCursor.MemoryAllocation->AllocDescriptor.MemoryLength;
      NewHobCursor.Raw = GetHobList ();
      NewHobCursor.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, NewHobCursor.Raw);
      while (NewHobCursor.Raw != NULL) {
        NewBase  = NewHobCursor.MemoryAllocation->AllocDescriptor.MemoryBaseAddress;
        NewLimit = NewBase + NewHobCursor.MemoryAllocation->AllocDescriptor.MemoryLength;
        if ((OldBase == NewBase) && (OldLimit == NewLimit)) {
          NewHobCursor.MemoryAllocation->Header.HobType = EFI_HOB_TYPE_UNUSED;
        } else if ((OldBase == NewBase) && (OldLimit < NewLimit)) {
          NewHobCursor.MemoryAllocation->AllocDescriptor.MemoryBaseAddress = OldLimit;
          NewHobCursor.MemoryAllocation->AllocDescriptor.MemoryLength      = NewLimit - OldLimit;
        } else if ((OldBase > NewBase) && (OldLimit == NewLimit)) {
          NewHobCursor.MemoryAllocation->AllocDescriptor.MemoryLength = OldBase - NewBase;
        } else if ((OldBase > NewBase) && (OldLimit < NewLimit)) {
          NewHobCursor.MemoryAllocation->AllocDescriptor.MemoryLength = OldBase - NewBase;
          BuildMemoryAllocationHob (
            OldLimit,
            NewLimit - OldLimit,
            NewHobCursor.MemoryAllocation->AllocDescriptor.MemoryType
            );
        } else {
          NewHobCursor.Raw = GET_NEXT_HOB (NewHobCursor.Raw);
          NewHobCursor.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, NewHobCursor.Raw);
          continue;
        }

        break;
      }
    }

    OldHobCursor.Raw = GET_NEXT_HOB (OldHobCursor.Raw);
  }

  return EFI_SUCCESS;
}

/**
  Create new Hand-off Hob and parse Memory Map.

  @param[in]  Hob            The hob list created by bootloader.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
CreateHandOffHobAndParseMemoryMap (
  IN EFI_PEI_HOB_POINTERS  Hob
  )
{
  EFI_STATUS            Status;
  UINTN                 MinimalNeededSize;
  EFI_PHYSICAL_ADDRESS  FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS  FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS  MemoryBottom;
  EFI_PHYSICAL_ADDRESS  MemoryTop;
  VOID                  *MemoryMapHob;

  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  MemoryMapHob      = GetNextGuidHob (&gUniversalPayloadMemoryMapGuid, Hob.Raw);
  if (MemoryMapHob == NULL) {
    Status = FindFreeMemoryFromResourceDescriptorHob (
               Hob,
               MinimalNeededSize,
               &FreeMemoryBottom,
               &FreeMemoryTop,
               &MemoryBottom,
               &MemoryTop
               );
  } else {
    Status = FindFreeMemoryFromMemoryMapTable (
               Hob,
               MinimalNeededSize,
               &FreeMemoryBottom,
               &FreeMemoryTop,
               &MemoryBottom,
               &MemoryTop
               );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)MemoryTop, (VOID *)(UINTN)FreeMemoryBottom, (VOID *)(UINTN)FreeMemoryTop);
  //
  // From now on, mHobList will point to the new Hob range.
  //

  //
  // Create resource Hob that Uefi can use based on Memory Map Table.
  //
  if (MemoryMapHob != NULL) {
    Status = CreateHobsBasedOnMemoryMap ((UNIVERSAL_PAYLOAD_MEMORY_MAP *)GET_GUID_HOB_DATA (MemoryMapHob));
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = RemoveDuplicatedMemoryHobs (Hob);
  }

  return Status;
}

/**
  Create resource Hob that Uefi can use based on Memory Map Table.
  This function assume the memory map table is sorted, and doesn't have overlap with each other.
  Also this function assume the memory map table only contains physical memory range.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
RETURN_STATUS
CreateHobsBasedOnMemoryMap10 (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  UINTN                        Index;
  EFI_MEMORY_DESCRIPTOR        *MemMapTable;
  EFI_MEMORY_DESCRIPTOR        *SortedMemMapTable;
  EFI_MEMORY_DESCRIPTOR        SortBuffer;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceDescriptor;
  EFI_PHYSICAL_ADDRESS         MemMapTableLimit;

  UINTN  Count;

  Count = 0;
  GetUplMemoryMap (NULL, &Count, 0);

  SortedMemMapTable = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (EFI_MEMORY_DESCRIPTOR) * Count));
  if (SortedMemMapTable == NULL) {
    ASSERT (FALSE);
    return RETURN_UNSUPPORTED;
  }
  GetUplMemoryMap (SortedMemMapTable, &Count, 0);
  QuickSort (SortedMemMapTable, Count, sizeof (EFI_MEMORY_DESCRIPTOR), MemoryMapTableCompare, &SortBuffer);
  for (Index = 0; Index < Count; Index++) {
    MemMapTable = (EFI_MEMORY_DESCRIPTOR *)(((UINT8 *)SortedMemMapTable) + Index * sizeof (EFI_MEMORY_DESCRIPTOR));
    if ((Index != 0) && (MemMapTable->PhysicalStart < MemMapTableLimit)) {
      ASSERT (FALSE);
      return RETURN_UNSUPPORTED;
    }

    if ((MemMapTable->PhysicalStart & (EFI_PAGE_SIZE - 1)) != 0) {
      ASSERT (FALSE);
      return RETURN_UNSUPPORTED;
    }

    if (MemMapTable->NumberOfPages == 0) {
      continue;
    }

    MemMapTableLimit = MemMapTable->PhysicalStart + EFI_PAGES_TO_SIZE (MemMapTable->NumberOfPages);
    if (MemMapTableLimit < MemMapTable->PhysicalStart) {
      ASSERT (FALSE);
      return RETURN_UNSUPPORTED;
    }

    Hob.Raw            = GetHobList ();
    ResourceDescriptor = NULL;
    while (!END_OF_HOB_LIST (Hob)) {
      if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
        ResourceDescriptor = Hob.ResourceDescriptor;
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
    }

    //
    // Every Memory map range should be contained in one Resource Descriptor Hob.
    //
    if ((ResourceDescriptor != NULL) &&
        (ResourceDescriptor->ResourceAttribute == ConvertCapabilitiesToResourceDescriptorHobAttributes (MemMapTable->Attribute)) &&
        (ResourceDescriptor->ResourceType == ConvertEfiMemoryTypeToResourceDescriptorHobResourceType (MemMapTable->Type)) &&
        (ResourceDescriptor->PhysicalStart + ResourceDescriptor->ResourceLength == MemMapTable->PhysicalStart))
    {
      ResourceDescriptor->ResourceLength += EFI_PAGES_TO_SIZE (MemMapTable->NumberOfPages);
    } else {
      BuildResourceDescriptorHob (
        ConvertEfiMemoryTypeToResourceDescriptorHobResourceType (MemMapTable->Type),
        ConvertCapabilitiesToResourceDescriptorHobAttributes (MemMapTable->Attribute),
        MemMapTable->PhysicalStart,
        EFI_PAGES_TO_SIZE (MemMapTable->NumberOfPages)
        );
    }

    //
    // Every used Memory map range should be contained in Memory Allocation Hob.
    //
    if (MemMapTable->Type != EfiConventionalMemory) {
      BuildMemoryAllocationHob (
        MemMapTable->PhysicalStart,
        EFI_PAGES_TO_SIZE (MemMapTable->NumberOfPages),
        MemMapTable->Type
        );
    }
  }

  return RETURN_SUCCESS;
}