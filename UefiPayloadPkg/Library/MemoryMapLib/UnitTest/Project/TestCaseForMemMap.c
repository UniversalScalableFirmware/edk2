
#include "Declarations.h"

VOID
TestCaseForMemMap1 (
  VOID* HobList1,
  VOID* HobList2
  )
{
  UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob;
  EFI_MEMORY_DESCRIPTOR         *MemoryMap;
  UINTN                         Count;

  Count = 1;

  MemoryMapHob                  = BuildGuidHob (&gUniversalPayloadMemoryMapGuid, sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof (EFI_MEMORY_DESCRIPTOR) * Count);
  MemoryMapHob->Header.Revision = UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION;
  MemoryMapHob->Header.Length   = (UINT16)(sizeof (UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof (EFI_MEMORY_DESCRIPTOR) * Count);
  MemoryMap                     = MemoryMapHob->MemoryMap;
  MemoryMapHob->Count           = Count;
  MemoryMapHob->DescriptorSize  = sizeof (EFI_MEMORY_DESCRIPTOR);
  MemoryMap[0].PhysicalStart    = 0x1000;
  MemoryMap[0].NumberOfPages    = 1;
}


//
// Case that memory descriptor's physical start is not 1 page align
//
VOID
TestCaseForMemMap2(
  VOID* HobList1,
  VOID* HobList2
)
{
    UNIVERSAL_PAYLOAD_MEMORY_MAP *MemoryMapHob;
    EFI_MEMORY_DESCRIPTOR        *MemoryMap;
    UINTN                         Count;
    Count = 1;

    MemoryMapHob = BuildGuidHob(&gUniversalPayloadMemoryMapGuid, sizeof(UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof(EFI_MEMORY_DESCRIPTOR) * Count);
    MemoryMapHob->Header.Revision = UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION;
    MemoryMapHob->Header.Length = (UINT16)(sizeof(UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof(EFI_MEMORY_DESCRIPTOR) * Count);
    MemoryMap = MemoryMapHob->MemoryMap;
    MemoryMapHob->Count = Count;
    MemoryMapHob->DescriptorSize = sizeof(EFI_MEMORY_DESCRIPTOR);
    MemoryMap[0].PhysicalStart = 0x2200;
    MemoryMap[0].NumberOfPages = 1;
}

//
// Case where Memory descriptor's Number of pages is 0
//
VOID
TestCaseForMemMap3(
  VOID* HobList1,
  VOID* HobList2
)
{
    UNIVERSAL_PAYLOAD_MEMORY_MAP *MemoryMapHob;
    EFI_MEMORY_DESCRIPTOR        *MemoryMap;
    UINTN                        Count;
    Count = 1;

    MemoryMapHob = BuildGuidHob(&gUniversalPayloadMemoryMapGuid, sizeof(UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof(EFI_MEMORY_DESCRIPTOR) * Count);
    MemoryMapHob->Header.Revision = UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION;
    MemoryMapHob->Header.Length = (UINT16)(sizeof(UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof(EFI_MEMORY_DESCRIPTOR) * Count);
    MemoryMap = MemoryMapHob->MemoryMap;
    MemoryMapHob->Count = Count;
    MemoryMapHob->DescriptorSize = sizeof(EFI_MEMORY_DESCRIPTOR);
    MemoryMap[0].PhysicalStart = 0x3000;
    MemoryMap[0].NumberOfPages = 0;
}

//
// Case that memory descriptor's end address is less than start address
//
VOID
TestCaseForMemMap4(
  VOID* HobList1,
  VOID* HobList2
)
{
    UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob;
    EFI_MEMORY_DESCRIPTOR         *MemoryMap;
    UINTN                         Count;
    Count = 1;

    MemoryMapHob = BuildGuidHob(&gUniversalPayloadMemoryMapGuid, sizeof(UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof(EFI_MEMORY_DESCRIPTOR) * Count);
    MemoryMapHob->Header.Revision = UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION;
    MemoryMapHob->Header.Length = (UINT16)(sizeof(UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof(EFI_MEMORY_DESCRIPTOR) * Count);
    MemoryMap = MemoryMapHob->MemoryMap;
    MemoryMapHob->Count = Count;
    MemoryMapHob->DescriptorSize = sizeof(EFI_MEMORY_DESCRIPTOR);
    MemoryMap[0].PhysicalStart = 0x4000;
    MemoryMap[0].NumberOfPages = (0-0x1000)/0x1000;
}

//
// MemoryMapHob's descriptor size is less than EFI_MEMORY_DESCRIPTOR size
//
VOID
TestCaseForMemMap5(
  VOID* HobList1,
  VOID* HobList2
)
{
    UNIVERSAL_PAYLOAD_MEMORY_MAP *MemoryMapHob;
    EFI_MEMORY_DESCRIPTOR        *MemoryMap;
    UINTN                         Count;
    Count = 1;

    MemoryMapHob = BuildGuidHob(&gUniversalPayloadMemoryMapGuid, sizeof(UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof(EFI_MEMORY_DESCRIPTOR) * Count);
    MemoryMapHob->Header.Revision = UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION;
    MemoryMapHob->Header.Length = (UINT16)(sizeof(UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof(EFI_MEMORY_DESCRIPTOR) * Count);
    MemoryMap = MemoryMapHob->MemoryMap;
    MemoryMapHob->Count = Count;
    MemoryMapHob->DescriptorSize = sizeof(EFI_MEMORY_DESCRIPTOR) - 10;
    MemoryMap[0].PhysicalStart = 0x7000;
    MemoryMap[0].NumberOfPages = 3;
}


//
// Current Memory Map descriptor overlaps with another Memory map descriptor
//
VOID
TestCaseForMemMap6(
  VOID* HobList1,
  VOID* HobList2
)
{
    UNIVERSAL_PAYLOAD_MEMORY_MAP *MemoryMapHob;
    EFI_MEMORY_DESCRIPTOR        *MemoryMap;
    UINTN                         Count;
    Count = 2;

    MemoryMapHob = BuildGuidHob(&gUniversalPayloadMemoryMapGuid, sizeof(UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof(EFI_MEMORY_DESCRIPTOR) * Count);
    MemoryMapHob->Header.Revision = UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION;
    MemoryMapHob->Header.Length = (UINT16)(sizeof(UNIVERSAL_PAYLOAD_MEMORY_MAP) + sizeof(EFI_MEMORY_DESCRIPTOR) * Count);
    MemoryMap = MemoryMapHob->MemoryMap;
    MemoryMapHob->Count = Count;
    MemoryMapHob->DescriptorSize = sizeof(EFI_MEMORY_DESCRIPTOR);
    MemoryMap[0].PhysicalStart = 0x2000;
    MemoryMap[0].NumberOfPages = 2;
    MemoryMap[1].PhysicalStart = 0x3000;
    MemoryMap[1].NumberOfPages = 3;
}


TEST_CASE  gTestCaseForMemMap[] = {
  { RETURN_SUCCESS, TestCaseForMemMap1, 0, "MemoryMap.c"},
  { RETURN_UNSUPPORTED, TestCaseForMemMap2, 186, "MemoryMap.c"},
  //{ RETURN_UNSUPPORTED, TestCaseForMemMap3, 192, "MemoryMap.c"},
  { RETURN_UNSUPPORTED, TestCaseForMemMap4, 196, "MemoryMap.c"},
  { RETURN_UNSUPPORTED, TestCaseForMemMap5, 166, "MemoryMap.c"},
  { RETURN_UNSUPPORTED, TestCaseForMemMap6, 181, "MemoryMap.c"},
 };

UINTN
GetTestCaseForMemMapCount (
  VOID
  )
{
  return ARRAY_SIZE (gTestCaseForMemMap);
}
