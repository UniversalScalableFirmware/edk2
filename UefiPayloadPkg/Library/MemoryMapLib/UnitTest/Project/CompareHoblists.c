#include "Declarations.h"

typedef struct {
  BOOLEAN    IsPhysicalMem;
  UINT32     ResType;
  UINT32     MemType;
  UINT64     Attribute;
  UINTN      ResourceHobIndex;
  UINTN      MemHobIndex;
  UINTN      HobListIndex;
} INFO_STRUCT;

VOID
GetInfoOfPointFromMemoryMap (
  IN CONST UINT64                  Point,
  IN UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob,
  OUT      INFO_STRUCT             *Info
  )
{
  UINTN    Index;
  BOOLEAN  ContainInMemMap;

  Info->Attribute     = MAX_UINT64;
  Info->IsPhysicalMem = FALSE;
  Info->MemType       = MAX_UINT32;
  Info->ResType       = MAX_UINT32;
  ContainInMemMap     = FALSE;
  for (Index = 0; Index < MemoryMapHob->Count; Index++) {
    if (  (Point >= MemoryMapHob->MemoryMap[Index].PhysicalStart)
       && (Point < (MemoryMapHob->MemoryMap[Index].PhysicalStart + MemoryMapHob->MemoryMap[Index].NumberOfPages * EFI_PAGE_SIZE)))
    {
      if (ContainInMemMap) {
        printf ("Point(%llx) already exists in another memory map descriptor\n", Point);
        ASSERT (!ContainInMemMap);
      }

      if (MemoryMapHob->MemoryMap[Index].Type != EfiConventionalMemory) {
        Info->MemType = MemoryMapHob->MemoryMap[Index].Type;
      }

      ContainInMemMap     = TRUE;
      Info->IsPhysicalMem = TRUE;
      Info->ResType       = ConvertEfiMemoryTypeToResourceDescriptorHobResourceType (MemoryMapHob->MemoryMap[Index].Type);
      Info->Attribute     = ConvertCapabilitiesToResourceDescriptorHobAttributes (MemoryMapHob->MemoryMap[Index].Attribute);
    }
  }
}

VOID
GetInfoOfPointFromHob (
  IN CONST UINT64       Point,
  IN CONST VOID         *HobList,
  OUT      INFO_STRUCT  *Info,
  IN  UINTN             HoblistIndex
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 Index;
  BOOLEAN               ContainInResourceHob;
  BOOLEAN               ContainInMemHob;
  BOOLEAN               ContainInHobList;
  UINTN                 ResourceHobIndex;
  UINTN                 MemHobIndex;
  UINTN                 HobListIndex;

  ResourceHobIndex    = MAX_UINT32;
  MemHobIndex         = MAX_UINT32;
  HobListIndex        = MAX_UINT32;
  Info->Attribute     = MAX_UINT64;
  Info->IsPhysicalMem = FALSE;
  Info->MemType       = MAX_UINT32;
  Info->ResType       = MAX_UINT32;

  Hob.Raw              = (UINT8 *)HobList;
  Index                = 0;
  ContainInResourceHob = FALSE;
  ContainInMemHob      = FALSE;
  ContainInHobList     = FALSE;

  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (  (Point >= Hob.ResourceDescriptor->PhysicalStart)
         && (Point < (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength)))
      {
        if (ContainInResourceHob) {
          printf ("Error: Point(%llx) already exist in more than one resource hob, hob[%d] and hob[%d] in hoblist%d\n", Point, ResourceHobIndex, Index, HoblistIndex);
        }

        Info->ResType        = Hob.ResourceDescriptor->ResourceType;
        Info->Attribute      = Hob.ResourceDescriptor->ResourceAttribute;
        ResourceHobIndex     = Index;
        Info->IsPhysicalMem  = TRUE;
        ContainInResourceHob = TRUE;
      }
    }

    if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      if (  (Point >= Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress)
         && (Point < (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength)))
      {
        if (ContainInMemHob) {
          printf ("Error: Point(%llx) already exist in more than one memory hob, hob[%d] and hob[%d] in hoblist%d\n", Point, MemHobIndex, Index, HoblistIndex);
        }

        if ((Index == 1) && (HoblistIndex == 2)) {
          // One exception: The Hob[1] in the HobList2 is a memory allocation hob, used when transfer mem map back to hob.
          // The information about this hob is different.
        } else {
          Info->MemType = Hob.MemoryAllocation->AllocDescriptor.MemoryType;
          MemHobIndex   = Index;
        }
      }
    }

    if (Hob.Header->HobType == EFI_HOB_TYPE_HANDOFF) {
      if (  (Point >= (UINT64)(UINTN)Hob.Raw)
         && (Point < (((Hob.HandoffInformationTable->EfiFreeMemoryBottom) + SIZE_4KB - 1) & (~(SIZE_4KB - 1)))))
      {
        if (ContainInHobList) {
          printf ("Error: Point(%llx) already exist in more than one resource hob, hob[%d] and hob[%d] in ", Point, ResourceHobIndex, Index);
        }

        ResourceHobIndex = Index;
        ContainInHobList = TRUE;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Index++;
  }

  if (ContainInHobList && (HoblistIndex == 1)) {
    ASSERT (!ContainInMemHob);
    Info->MemType = EfiBootServicesData;
  }

  if (ContainInHobList || ContainInMemHob) {
    ASSERT (ContainInResourceHob);
  }

  Info->ResourceHobIndex = ResourceHobIndex;
  Info->MemHobIndex      = MemHobIndex;
  Info->HobListIndex     = HobListIndex;
}

VOID
GetKeyPointFromMemoryMap (
  IN UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob,
  IN UINT64                        *Array
  )
{
  UINTN  Index;
  UINTN  i = 0;

  for (Index = 0; Index < MemoryMapHob->Count; Index++) {
    Array[i++] = MemoryMapHob->MemoryMap[Index].PhysicalStart;
    Array[i++] = MemoryMapHob->MemoryMap[Index].PhysicalStart + 1;
    Array[i++] = MemoryMapHob->MemoryMap[Index].PhysicalStart - 1;
    Array[i++] = MemoryMapHob->MemoryMap[Index].PhysicalStart + MemoryMapHob->MemoryMap[Index].NumberOfPages * EFI_PAGE_SIZE;
    Array[i++] = MemoryMapHob->MemoryMap[Index].PhysicalStart + MemoryMapHob->MemoryMap[Index].NumberOfPages * EFI_PAGE_SIZE + 1;
    Array[i++] = MemoryMapHob->MemoryMap[Index].PhysicalStart + MemoryMapHob->MemoryMap[Index].NumberOfPages * EFI_PAGE_SIZE - 1;
    Array[i++] = (MemoryMapHob->MemoryMap[Index].PhysicalStart + (MemoryMapHob->MemoryMap[Index].PhysicalStart + MemoryMapHob->MemoryMap[Index].NumberOfPages * EFI_PAGE_SIZE)) / 2;
  }
}

VOID
GetKeyPointFromHob (
  IN CONST VOID  *Start,
  IN UINT64      *Array
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 i = 0;

  Hob.Raw = (UINT8 *)Start;
  while (!END_OF_HOB_LIST (Hob)) {
    if ((Hob.Header->HobType != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) && (Hob.Header->HobType != EFI_HOB_TYPE_MEMORY_ALLOCATION)) {
      Hob.Raw = GET_NEXT_HOB (Hob);
      continue;
    } else if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      Array[i++] = Hob.ResourceDescriptor->PhysicalStart;
      Array[i++] = Hob.ResourceDescriptor->PhysicalStart + 1;
      Array[i++] = Hob.ResourceDescriptor->PhysicalStart - 1;
      Array[i++] = Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength;
      Array[i++] = Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength + 1;
      Array[i++] = Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength - 1;
      Array[i++] = (Hob.ResourceDescriptor->PhysicalStart + (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength)) / 2;
    } else if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      Array[i++] = Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress;
      Array[i++] = Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + 1;
      Array[i++] = Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress - 1;
      Array[i++] = Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength;
      Array[i++] = Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength + 1;
      Array[i++] = Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength - 1;
      Array[i++] = (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength)) / 2;
    } else if (Hob.Header->HobType == EFI_HOB_TYPE_HANDOFF) {
      Array[i++] = Hob.HandoffInformationTable->EfiFreeMemoryBottom;
      Array[i++] = Hob.HandoffInformationTable->EfiFreeMemoryBottom + 1;
      Array[i++] = Hob.HandoffInformationTable->EfiFreeMemoryBottom - 1;
      Array[i++] = Hob.HandoffInformationTable->EfiFreeMemoryTop;
      Array[i++] = Hob.HandoffInformationTable->EfiFreeMemoryTop + 1;
      Array[i++] = Hob.HandoffInformationTable->EfiFreeMemoryTop - 1;
      Array[i++] = Hob.HandoffInformationTable->EfiMemoryBottom;
      Array[i++] = Hob.HandoffInformationTable->EfiMemoryBottom + 1;
      Array[i++] = Hob.HandoffInformationTable->EfiMemoryBottom - 1;
      Array[i++] = Hob.HandoffInformationTable->EfiMemoryTop;
      Array[i++] = Hob.HandoffInformationTable->EfiMemoryTop + 1;
      Array[i++] = Hob.HandoffInformationTable->EfiMemoryTop - 1;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }
}

VOID
VerifyHob (
  IN CONST VOID  *HobList1Start,
  IN CONST VOID  *HobList2Start
  )
{
  EFI_PEI_HOB_POINTERS  HobList1;
  EFI_PEI_HOB_POINTERS  HobList2;
  INFO_STRUCT           Info1, Info2;
  UINT64                *AllArray;
  UINTN                 HobList1Count;
  UINTN                 HobList2Count;
  UINTN                 AllHobCount;

  HobList1.Raw  = (UINT8 *)HobList1Start;
  HobList2.Raw  = (UINT8 *)HobList2Start;
  HobList1Count = 0;
  HobList2Count = 0;
  while (!END_OF_HOB_LIST (HobList1)) {
    if ((HobList1.Header->HobType != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) && (HobList1.Header->HobType != EFI_HOB_TYPE_MEMORY_ALLOCATION)) {
      HobList1.Raw = GET_NEXT_HOB (HobList1);
      continue;
    }

    HobList1Count++;
    HobList1.Raw = GET_NEXT_HOB (HobList1);
  }

  while (!END_OF_HOB_LIST (HobList2)) {
    if ((HobList2.Header->HobType != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) && (HobList2.Header->HobType != EFI_HOB_TYPE_MEMORY_ALLOCATION)) {
      HobList2.Raw = GET_NEXT_HOB (HobList2);
      continue;
    }

    HobList2Count++;
    HobList2.Raw = GET_NEXT_HOB (HobList2);
  }

  AllHobCount = HobList1Count + HobList2Count;
  // For each resource hob or memory hob, pick 7 key point
  // For each phit hob, pick 12 key point
  AllArray = (UINT64 *)malloc (sizeof (UINT64) * (AllHobCount * 7 + 12 * 2));
  if (AllArray == NULL) {
    printf ("Memory cannot be allocated\n");
    ASSERT (FALSE);
  }

  GetKeyPointFromHob (HobList1Start, AllArray);
  GetKeyPointFromHob (HobList2Start, AllArray + HobList1Count + 12);

  for (UINTN j = 0; j < AllHobCount * 7; j++) {
    GetInfoOfPointFromHob (AllArray[j], HobList1Start, &Info1, 1);
    GetInfoOfPointFromHob (AllArray[j], HobList2Start, &Info2, 2);

    if ((Info1.Attribute == Info2.Attribute) && (Info1.MemType == Info2.MemType) && (Info1.ResType == Info2.ResType) && (Info1.IsPhysicalMem == Info2.IsPhysicalMem)) {
    } else {
      if ((Info1.Attribute == Info2.Attribute) && (Info1.MemType == Info2.MemType) && (Info1.IsPhysicalMem == Info2.IsPhysicalMem) && (Info2.MemType == EfiReservedMemoryType) && (Info2.ResType == EFI_RESOURCE_MEMORY_RESERVED)) {
        // case that in hoblist1, it is in non-reserved resource hob but reserved memory hob
      } else if ((Info1.Attribute == Info2.Attribute) && (Info1.ResType == Info2.ResType) && (Info1.IsPhysicalMem == Info2.IsPhysicalMem) && (Info2.MemType == EfiReservedMemoryType) && (Info2.ResType == EFI_RESOURCE_MEMORY_RESERVED)) {
        // case that in hoblist1, it is in reserved resource hob but no in reserved memory hob
      } else if ((Info1.Attribute == Info2.Attribute) && (Info1.ResType == Info2.ResType) && (Info1.IsPhysicalMem == Info2.IsPhysicalMem) && (Info1.MemType == EfiConventionalMemory) && (Info2.MemType == MAX_UINT32) && (Info2.ResType == EFI_RESOURCE_SYSTEM_MEMORY)) {
        // case that in hoblist1, it is in conventional memory hob but not in any memory hob in hoblist2
      } else {
        printf ("The point which has issue is %llx\n", AllArray[j]);
        printf ("Index in hob 1\n");

        printf ("Index of ResourceHobIndex is %d\n", Info1.ResourceHobIndex);
        printf ("Index of MemHobIndex is %d\n", Info1.MemHobIndex);
        printf ("Index of HobListIndex is %d\n", Info1.HobListIndex);

        printf ("Index in hob 2\n");

        printf ("Index of ResourceHobIndex is %d\n", Info2.ResourceHobIndex);
        printf ("Index of MemHobIndex is %d\n", Info2.MemHobIndex);
        printf ("Index of HobListIndex is %d\n", Info2.HobListIndex);

        ASSERT (FALSE);
      }
    }
  }

  if (AllArray != NULL) {
    free (AllArray);
  }
}

VOID
VerifyHobList1WithMemoryMap (
  IN CONST VOID                    *HobList1Start,
  IN UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob
  )
{
  EFI_PEI_HOB_POINTERS  HobList1;
  INFO_STRUCT           Info1, Info2;
  UINT64                *AllArray;
  UINTN                 HobList1Count;
  UINTN                 AllCount;

  HobList1.Raw  = (UINT8 *)HobList1Start;
  HobList1Count = 0;
  while (!END_OF_HOB_LIST (HobList1)) {
    if ((HobList1.Header->HobType != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) && (HobList1.Header->HobType != EFI_HOB_TYPE_MEMORY_ALLOCATION)) {
      HobList1.Raw = GET_NEXT_HOB (HobList1);
      continue;
    }

    HobList1Count++;
    HobList1.Raw = GET_NEXT_HOB (HobList1);
  }

  AllCount = HobList1Count + MemoryMapHob->Count;
  // For each resource hob or memory hob, pick 7 key point
  // For each phit hob, pick 12 key point
  AllArray = (UINT64 *)malloc (sizeof (UINT64) * (AllCount * 7 + 12));
  if (AllArray == NULL) {
    printf ("Memory cannot be allocated\n");
    ASSERT (FALSE);
  }

  GetKeyPointFromHob (HobList1Start, AllArray);
  GetKeyPointFromMemoryMap (MemoryMapHob, AllArray + HobList1Count + 12);

  for (UINTN j = 0; j < AllCount * 7; j++) {
    GetInfoOfPointFromHob (AllArray[j], HobList1Start, &Info1, 1);
    GetInfoOfPointFromMemoryMap (AllArray[j], MemoryMapHob, &Info2);

    if ((Info1.Attribute == Info2.Attribute) && (Info1.MemType == Info2.MemType) && (Info1.ResType == Info2.ResType) && (Info1.IsPhysicalMem == Info2.IsPhysicalMem)) {
    } else {
      if ((Info1.Attribute == Info2.Attribute) && (Info1.MemType == Info2.MemType) && (Info1.IsPhysicalMem == Info2.IsPhysicalMem) && (Info2.MemType == EfiReservedMemoryType) && (Info2.ResType == EFI_RESOURCE_MEMORY_RESERVED)) {
        // case that in hoblist1, it is in non-reserved resource hob but reserved memory hob
      } else if ((Info1.Attribute == Info2.Attribute) && (Info1.ResType == Info2.ResType) && (Info1.IsPhysicalMem == Info2.IsPhysicalMem) && (Info2.MemType == EfiReservedMemoryType) && (Info2.ResType == EFI_RESOURCE_MEMORY_RESERVED)) {
        // case that in hoblist1, it is in reserved resource hob but no in reserved memory hob
      } else if ((Info1.Attribute == Info2.Attribute) && (Info1.ResType == Info2.ResType) && (Info1.IsPhysicalMem == Info2.IsPhysicalMem) && (Info1.MemType == EfiConventionalMemory) && (Info2.MemType == MAX_UINT32) && (Info2.ResType == EFI_RESOURCE_SYSTEM_MEMORY)) {
        // case that in hoblist1, it is in conventional memory hob but not in any memory hob in hoblist2
      } else {
        printf ("The point which has issue is %llx\n", AllArray[j]);
        printf ("Index in hob 1\n");

        printf ("Index of ResourceHobIndex is %d\n", Info1.ResourceHobIndex);
        printf ("Index of MemHobIndex is %d\n", Info1.MemHobIndex);
        printf ("Index of HobListIndex is %d\n", Info1.HobListIndex);

        ASSERT (FALSE);
      }
    }
  }

  if (AllArray != NULL) {
    free (AllArray);
  }
}

VOID
VerifyHobList2WithMemoryMap (
  IN CONST VOID                    *HobList2Start,
  IN UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob
  )
{
  EFI_PEI_HOB_POINTERS  HobList2;
  INFO_STRUCT           Info1, Info2;
  UINT64                *AllArray;
  UINTN                 HobList2Count;
  UINTN                 AllCount;

  HobList2.Raw  = (UINT8 *)HobList2Start;
  HobList2Count = 0;
  while (!END_OF_HOB_LIST (HobList2)) {
    if ((HobList2.Header->HobType != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) && (HobList2.Header->HobType != EFI_HOB_TYPE_MEMORY_ALLOCATION)) {
      HobList2.Raw = GET_NEXT_HOB (HobList2);
      continue;
    }

    HobList2Count++;
    HobList2.Raw = GET_NEXT_HOB (HobList2);
  }

  AllCount = HobList2Count + MemoryMapHob->Count;
  // For each resource hob or memory hob, pick 7 key point
  // For each phit hob, pick 12 key point
  AllArray = (UINT64 *)malloc (sizeof (UINT64) * (AllCount * 7 + 12));
  if (AllArray == NULL) {
    printf ("Memory cannot be allocated\n");
    ASSERT (FALSE);
  }

  GetKeyPointFromHob (HobList2Start, AllArray);
  GetKeyPointFromMemoryMap (MemoryMapHob, AllArray + HobList2Count + 12);

  for (UINTN j = 0; j < AllCount * 7; j++) {
    GetInfoOfPointFromHob (AllArray[j], HobList2Start, &Info1, 2);
    GetInfoOfPointFromMemoryMap (AllArray[j], MemoryMapHob, &Info2);

    if ((Info1.Attribute == Info2.Attribute) && (Info1.MemType == Info2.MemType) && (Info1.ResType == Info2.ResType) && (Info1.IsPhysicalMem == Info2.IsPhysicalMem)) {
    } else {
      printf ("The point which has issue is %llx\n", AllArray[j]);
      printf ("Index in hob 1\n");

      printf ("Index of ResourceHobIndex is %d\n", Info1.ResourceHobIndex);
      printf ("Index of MemHobIndex is %d\n", Info1.MemHobIndex);
      printf ("Index of HobListIndex is %d\n", Info1.HobListIndex);

      ASSERT (FALSE);
    }
  }

  if (AllArray != NULL) {
    free (AllArray);
  }
}
