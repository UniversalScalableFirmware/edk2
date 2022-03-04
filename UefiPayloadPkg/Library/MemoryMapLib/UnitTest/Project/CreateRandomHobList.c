#include "Declarations.h"
#include <time.h>

#define  N_ARRAY_MAX  500
#define  BIT16        0x00010000
#define  BIT32        0x0000000100000000ULL
#define  MAX_UINT64   ((UINT64)0xFFFFFFFFFFFFFFFFULL)

UINT64                       TempArray[N_ARRAY_MAX]; // for sort
EFI_RESOURCE_ATTRIBUTE_TYPE  LastAttribute = 0;

UINT16
Get16BitRandomNumber (
  VOID
  )
{
  UINT16  retval;

  retval = rand () * 2 + rand () % 2;
  return retval;
}

UINT32
Get32BitRandomNumber (
  VOID
  )
{
  UINT32  retval;

  retval = ((UINT32)Get16BitRandomNumber ()) * BIT16 + (UINT32)Get16BitRandomNumber ();
  return retval;
}

UINT64
Get64BitRandomNumber (
  VOID
  )
{
  UINT64  retval;

  retval = ((UINT64)Get32BitRandomNumber ()) * BIT32 + (UINT64)Get32BitRandomNumber ();
  return retval;
}

UINT64
GetRandomNumber (
  UINT64  Base,
  UINT64  Limit,
  UINT64  AlignSize
  )
{
  UINT64  SpecialArray[4];
  UINTN   SpecialCount;
  UINTN   Index;

  SpecialCount = 0;
  Index        = 0;

  Base  = (Base + AlignSize - 1) & (~(AlignSize - 1));
  Limit = Limit & (~(AlignSize - 1));

  if (Base == Limit) {
    return Base;
  }

  if (Base > Limit) {
    return MAX_UINT64;
  }

  SpecialArray[SpecialCount] = Base;
  SpecialCount++;
  SpecialArray[SpecialCount] = Limit;
  SpecialCount++;
  SpecialArray[SpecialCount] = Base + AlignSize;
  SpecialCount++;
  SpecialArray[SpecialCount] = Limit - AlignSize;
  SpecialCount++;

  if (Get16BitRandomNumber () % 100 < 2) {
    // special case
    Index = Get32BitRandomNumber () % SpecialCount;
    return SpecialArray[Index];
  } else {
    return Base + (Get64BitRandomNumber () % (Limit / AlignSize - Base / AlignSize)) * AlignSize;
  }
}

UINTN
GetNRandomNumber (
  UINT64  Base,
  UINT64  Limit,
  UINT64  AlignSize,
  UINTN   N,
  UINT64  **RandomList
  )
{
  UINTN   AvailibleRandom1 = 0;
  UINTN   AvailibleRandom2 = 0;
  UINT64  *InitList        = *RandomList;

  if (N == 0) {
    return 0;
  }

  UINT64  x = GetRandomNumber (Base, Limit, AlignSize);

  if (x == MAX_UINT64) {
    return 0;
  }

  **RandomList = x;
  (*RandomList)++;

  if (x == 0) {
    AvailibleRandom1 = 0;
  } else {
    AvailibleRandom1 = GetNRandomNumber (
                         (x > (Base + Limit) / 2) ? x + 1 : Base,
                         (x > (Base + Limit) / 2) ? Limit : x - 1,
                         AlignSize,
                         (N - 1) / 2,
                         RandomList
                         );
  }

  AvailibleRandom2 = GetNRandomNumber (
                       (x > (Base + Limit) / 2) ? Base : x + 1,
                       (x > (Base + Limit) / 2) ? x - 1 : Limit,
                       AlignSize,
                       N - 1 - AvailibleRandom1,
                       RandomList
                       );
  CopyMem (TempArray, InitList, (AvailibleRandom1 + AvailibleRandom2 + 1) * sizeof (UINT64));
  if (x > (Base + Limit) / 2) {
    // x, larger than x, smaller than x
    CopyMem (InitList, TempArray + AvailibleRandom1 + 1, AvailibleRandom2 * sizeof (UINT64));
    CopyMem ((InitList) + AvailibleRandom2, TempArray, 1 * sizeof (UINT64));
    CopyMem ((InitList) + AvailibleRandom2 + 1, TempArray + 1, AvailibleRandom1 * sizeof (UINT64));
  } else {
    // x, smaller than x, larger than x
    CopyMem (InitList, TempArray + 1, AvailibleRandom1 * sizeof (UINT64));
    CopyMem ((InitList) + AvailibleRandom1, TempArray, 1 * sizeof (UINT64));
    CopyMem ((InitList) + AvailibleRandom1 + 1, TempArray + 1 + AvailibleRandom1, AvailibleRandom2 * sizeof (UINT64));
  }

  return AvailibleRandom1 + AvailibleRandom2 + 1;
}

EFI_RESOURCE_ATTRIBUTE_TYPE  AllResourceAttribute[] = {
  EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE,
  EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED,
  EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE,
  EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE,
  EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE,
  EFI_RESOURCE_ATTRIBUTE_READ_PROTECTABLE,
  EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE,
  EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE,
  EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE,
  EFI_RESOURCE_ATTRIBUTE_PRESENT,
  EFI_RESOURCE_ATTRIBUTE_INITIALIZED,
  EFI_RESOURCE_ATTRIBUTE_TESTED,
  EFI_RESOURCE_ATTRIBUTE_PERSISTABLE,
  EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE
};

EFI_RESOURCE_ATTRIBUTE_TYPE
RandomResourceAttribute (
  )
{
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute = 0;
  UINT64                       Random            = Get64BitRandomNumber ();
  UINTN                        Index;

  if (rand () % 2 == 0) {
    ResourceAttribute = LastAttribute;
  } else {
    for (Index = 0; Index < ARRAY_SIZE (AllResourceAttribute); Index++) {
      ResourceAttribute += (Random & 1 ? AllResourceAttribute[Index] : 0);
      Random             = Random >> 1;
    }

    LastAttribute = ResourceAttribute;
  }

  ResourceAttribute |= (EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED | EFI_RESOURCE_ATTRIBUTE_PRESENT);
  return ResourceAttribute;
}

EFI_RESOURCE_ATTRIBUTE_TYPE
RandomResourceAttribute_EX (
  )
{
  return RandomResourceAttribute ();
}

VOID
CreateRandomMemoryHob (
  UINT64             Base,
  UINT64             Limit,
  UINT64             AlignSize,
  VOID               *HobList1,
  VOID               *HobList2,
  EFI_RESOURCE_TYPE  ResourceType
  )
{
  BOOLEAN          ContainHobList1 = TRUE;
  BOOLEAN          ContainHobList2 = TRUE;
  UINTN            Random;
  EFI_MEMORY_TYPE  type;

  if ((Limit <= ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList1)->EfiMemoryBottom) || (Base >= ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList1)->EfiMemoryTop)) {
    ContainHobList1 = FALSE;
  }

  if ((Limit <= ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList2)->EfiMemoryBottom) || (Base >= ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList2)->EfiMemoryTop)) {
    ContainHobList2 = FALSE;
  }

  if (ContainHobList1 && !ContainHobList2) {
    // Hoblist normally doesn't be included in any memory allocation hob
    return;
  }

  if (ContainHobList1 && ContainHobList2) {
    // Only contain Hoblist2
    Base = GetRandomNumber (((EFI_HOB_HANDOFF_INFO_TABLE *)HobList1)->EfiMemoryTop, ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList2)->EfiMemoryBottom, AlignSize);
  }

  if (ContainHobList2) {
    return;
  } else {
    if (rand () % 2 == 0) {
      return;
    } else if (ResourceType == EFI_RESOURCE_MEMORY_RESERVED) {
      type = EfiReservedMemoryType;
    } else {
      Random = rand () % 5;
      switch (Random) {
        case 1:
          type = EfiBootServicesData;
          break;
        case 2:
          type = EfiReservedMemoryType;
          break;
        default:
          type = rand () % 14;
          break;
      }
    }
  }

  BuildMemoryAllocationHob (Base, (Limit - Base), type);
}

VOID
CreateRandomResourceHob (
  UINT64  Base,
  UINT64  Limit,
  UINT64  AlignSize,
  VOID    *HobList1,
  VOID    *HobList2,
  UINTN   MemoryPointArrayMaxCount
  )
{
  BOOLEAN            ContainHobList1 = TRUE;
  BOOLEAN            ContainHobList2 = TRUE;
  EFI_RESOURCE_TYPE  ResourceType;
  UINT64             MemoryPointArray[N_ARRAY_MAX];
  UINT64             *MemoryPointArrayPointer;
  UINTN              MemoryPointArrayCount;
  UINTN              Index = 0;

  MemoryPointArrayPointer = MemoryPointArray;

  if ((Limit <= ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList1)->EfiMemoryBottom) || (Base >= ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList1)->EfiMemoryTop)) {
    ContainHobList1 = FALSE;
  }

  if ((Limit <= ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList2)->EfiMemoryBottom) || (Base >= ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList2)->EfiMemoryTop)) {
    ContainHobList2 = FALSE;
  }

  if (ContainHobList1 || ContainHobList2) {
    ResourceType = EFI_RESOURCE_SYSTEM_MEMORY;
  } else {
    if (rand () % 2 == 0) {
      return;
    }

    if (rand () % 2 == 0) {
      ResourceType = EFI_RESOURCE_SYSTEM_MEMORY;
    } else {
      ResourceType = EFI_RESOURCE_MEMORY_RESERVED;
    }
  }

  BuildResourceDescriptorHob (ResourceType, RandomResourceAttribute_EX (), Base, (Limit - Base));
  if (ContainHobList2) {
    MemoryPointArrayCount = GetNRandomNumber (
                              Base,
                              ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList2)->EfiMemoryBottom,
                              AlignSize,
                              (UINTN)GetRandomNumber (1, MemoryPointArrayMaxCount - 1, 1),
                              &MemoryPointArrayPointer
                              );
    MemoryPointArrayCount += GetNRandomNumber (
                               ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList2)->EfiMemoryTop,
                               Limit,
                               AlignSize,
                               (UINTN)GetRandomNumber (1, MemoryPointArrayMaxCount - MemoryPointArrayCount, 1),
                               &MemoryPointArrayPointer
                               );
  } else {
    MemoryPointArrayCount = GetNRandomNumber (Base, Limit, AlignSize, (UINTN)GetRandomNumber (0, MemoryPointArrayMaxCount, 1), &MemoryPointArrayPointer);
  }

  while (MemoryPointArrayCount > 1) {
    CreateRandomMemoryHob (
      MemoryPointArray[Index],
      MemoryPointArray[Index + 1],
      AlignSize,
      HobList1,
      HobList2,
      ResourceType
      );
    Index                 += 1;
    MemoryPointArrayCount -= 1;
  }
}

VOID
CreateRemainingHobs (
  VOID    *HobList1,
  VOID    *HobList2,
  UINT64  Base,
  UINT64  Limit
  )
{
  time_t  t;

  UINTN   ResourcePointArrayMaxCount = 50;       // must no larger than ResourcePointArray, and is an even number
  UINTN   MemoryPointArrayMaxCount   = 50;
  UINT64  ResourcePointArray[N_ARRAY_MAX];
  UINT64  *ResourcePointArrayPointer;
  UINTN   ResourcePointArrayCount;
  UINTN   Index     = 0;
  UINT64  AlignSize = 0x1000;

  srand ((unsigned)time (&t));

  // [ ...range1...hoblist1...range2...hostlist2... range3...]
  // There must be a resource hob cover hostlist1 and hostlist2

  ResourcePointArrayPointer = ResourcePointArray;
  ResourcePointArrayCount   = GetNRandomNumber (min (Base, ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList1)->EfiMemoryBottom), ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList1)->EfiMemoryBottom, AlignSize, (UINTN)GetRandomNumber (1, ResourcePointArrayMaxCount - 1, 1), &ResourcePointArrayPointer);
  ResourcePointArrayCount  += GetNRandomNumber (((EFI_HOB_HANDOFF_INFO_TABLE *)HobList1)->EfiMemoryTop, ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList2)->EfiMemoryBottom, AlignSize, (UINTN)GetRandomNumber (0, ResourcePointArrayMaxCount - 1 - ResourcePointArrayCount, 1), &ResourcePointArrayPointer);
  ResourcePointArrayCount  += GetNRandomNumber (((EFI_HOB_HANDOFF_INFO_TABLE *)HobList2)->EfiMemoryTop, max (Limit, ((EFI_HOB_HANDOFF_INFO_TABLE *)HobList2)->EfiMemoryTop), AlignSize, (UINTN)GetRandomNumber (1, ResourcePointArrayMaxCount - ResourcePointArrayCount, 1), &ResourcePointArrayPointer);

  while (ResourcePointArrayCount > 1) {
    CreateRandomResourceHob (
      ResourcePointArray[Index],
      ResourcePointArray[Index + 1],
      AlignSize,
      HobList1,
      HobList2,
      MemoryPointArrayMaxCount
      );
    Index                   += 1;
    ResourcePointArrayCount -= 1;
  }
}
