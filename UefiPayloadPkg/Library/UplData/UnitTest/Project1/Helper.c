/** @file
  @copyright
  INTEL CONFIDENTIAL
  Copyright 2020 Intel Corporation. <BR>

  The source code contained or described herein and all documents related to the
  source code ("Material") are owned by Intel Corporation or its suppliers or
  licensors. Title to the Material remains with Intel Corporation or its suppliers
  and licensors. The Material may contain trade secrets and proprietary    and
  confidential information of Intel Corporation and its suppliers and licensors,
  and is protected by worldwide copyright and trade secret laws and treaty
  provisions. No part of the Material may be used, copied, reproduced, modified,
  published, uploaded, posted, transmitted, distributed, or disclosed in any way
  without Intel's prior express written permission.

  No license under any patent, copyright, trade secret or other intellectual
  property right is granted to or conferred upon you by disclosure or delivery
  of the Materials, either expressly, by implication, inducement, estoppel or
  otherwise. Any license under such intellectual property rights must be
  express and approved by Intel in writing.

  Unless otherwise agreed by Intel in writing, you may not remove or alter
  this notice or any other notice embedded in Materials by Intel or
  Intel's suppliers or licensors in any way.
**/
#include "Declarations.h"

extern EFI_GUID gUefiPayloadPkgTokenSpaceGuid = {
    0x1d127ea, 0xf6f1, 0x4ef6, {0x94, 0x15, 0x8a, 0x0, 0x0, 0x93, 0xf8, 0x9d }
};
extern EFI_GUID  gEfiHobMemoryAllocModuleGuid = {
  0xF8E21975, 0x0899, 0x4F58, { 0xA4, 0xBE, 0x55, 0x25, 0xA9, 0xC6, 0xD7, 0x7A }
};
extern EFI_GUID  gEfiHobMemoryAllocStackGuid = {
  0x4ED4BF27, 0x4092, 0x42E9, { 0x80, 0x7D, 0x52, 0x7B, 0x1D, 0x00, 0xC9, 0xBD }
};
extern EFI_GUID CborRootmapHobGuid = {
  0xd585b7f5, 0xd49e, 0x42ef, { 0xa7, 0x28, 0xce, 0x73, 0x3a, 0xd0, 0x58, 0xab }
};
extern EFI_GUID gCborBufferGuid = {
  0xa1ff7424, 0x7a1a, 0x478e, { 0xa9, 0xe4, 0x92, 0xf3, 0x57, 0xd1, 0x28, 0x31 }
};

#define MAX_DEBUG_MESSAGE_LENGTH  0x100
char  TempFormat[MAX_DEBUG_MESSAGE_LENGTH];

BOOLEAN
EFIAPI
DebugPrintEnabled (
  VOID
  )
{
  return TRUE;
}

char *
strrpc (
  CONST char  *str,
  char        *oldstr,
  char        *newstr
  )
{
  memset (TempFormat, 0, sizeof (TempFormat));

  for (UINTN i = 0; i < strlen (str); i++) {
    if (!strncmp (str + i, oldstr, strlen (oldstr))) {
      strncat_s (TempFormat, MAX_DEBUG_MESSAGE_LENGTH, newstr, sizeof (newstr));

      i += strlen (oldstr) - 1;
    } else {
      strncat_s (TempFormat, MAX_DEBUG_MESSAGE_LENGTH, str + i, 1);
    }
  }

  return TempFormat;
}

VOID
EFIAPI
DebugVPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      VaListMarker
  )
{
  CHAR8  Buffer[MAX_DEBUG_MESSAGE_LENGTH];
  char   TempFormat1[MAX_DEBUG_MESSAGE_LENGTH];
  char   *TempFormat2;

  TempFormat2 = strrpc (Format, "%lx", "%llx");

  memset (TempFormat1, 0, sizeof (TempFormat1));
  strcpy_s (TempFormat1, MAX_DEBUG_MESSAGE_LENGTH, TempFormat2);
  TempFormat2 = strrpc (TempFormat1, "%g", "%lx");

  memset (TempFormat1, 0, sizeof (TempFormat1));
  strcpy_s (TempFormat1, MAX_DEBUG_MESSAGE_LENGTH, TempFormat2);
  TempFormat2 = strrpc (TempFormat1, "%a", "%s");

  vsprintf_s (Buffer, sizeof (Buffer), TempFormat2, VaListMarker);
  printf ("%s", Buffer);
}

VOID
EFIAPI
DebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST  Marker;

  VA_START (Marker, Format);
  DebugVPrint (ErrorLevel, Format, Marker);
  VA_END (Marker);
}

BOOLEAN
EFIAPI
DebugPrintLevelEnabled (
  IN  CONST UINTN  ErrorLevel
  )
{
  if (ErrorLevel == DEBUG_VERBOSE) {
    return FALSE;
  }

  return TRUE;
}

VOID*
EFIAPI
ZeroMem(
  OUT VOID* Buffer,
  IN UINTN  Length
)
{
  UINT8  Value = 0;

  for (; Length != 0; Length--) {
    ((UINT8*)Buffer)[Length - 1] = Value;
  }

  return Buffer;
}

EFI_GUID*
CopyGuid(
  EFI_GUID* DestinationGuid,
  CONST EFI_GUID* SourceGuid
)
{
  *DestinationGuid = *SourceGuid;

  return DestinationGuid;
}

UINT64
ReadUnaligned64(
  CONST UINT64* Buffer
)
{
  ASSERT(Buffer != NULL);

  return *Buffer;
}

BOOLEAN
EFIAPI
CompareGuid(
  IN CONST GUID* Guid1,
  IN CONST GUID* Guid2
)
{
  UINT64  LowPartOfGuid1;
  UINT64  LowPartOfGuid2;
  UINT64  HighPartOfGuid1;
  UINT64  HighPartOfGuid2;

  LowPartOfGuid1 = ReadUnaligned64((CONST UINT64*)Guid1);
  LowPartOfGuid2 = ReadUnaligned64((CONST UINT64*)Guid2);
  HighPartOfGuid1 = ReadUnaligned64((CONST UINT64*)Guid1 + 1);
  HighPartOfGuid2 = ReadUnaligned64((CONST UINT64*)Guid2 + 1);

  return (BOOLEAN)(LowPartOfGuid1 == LowPartOfGuid2 && HighPartOfGuid1 == HighPartOfGuid2);
}

EFIAPI
InternalAllocatePages(
  IN UINTN  Pages,
  IN EFI_MEMORY_TYPE MemoryType,
  OUT EFI_PHYSICAL_ADDRESS* Memory
) 
{
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS* FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS* FreeMemoryBottom;
  UINTN                 RemainingPages;
  UINTN                 Granularity;
  UINTN                 Padding;

  Hob.Raw = (UINT8*)mHobList;
  Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;

  FreeMemoryTop = &(Hob.HandoffInformationTable->EfiFreeMemoryTop);
  FreeMemoryBottom = &(Hob.HandoffInformationTable->EfiFreeMemoryBottom);
  Padding = *(FreeMemoryTop) & (Granularity - 1);

  if ((UINTN)(*FreeMemoryTop - *FreeMemoryBottom) < Padding) {
    printf("AllocatePages failed: Out of space after padding\n");
    ASSERT(FALSE);
  }

  *(FreeMemoryTop) -= Padding;
  if (Padding >= EFI_PAGE_SIZE) {
    //
    // Create a memory allocation HOB to cover
    // the pages that we will lose to rounding
    //
    BuildMemoryAllocationHob(
      *(FreeMemoryTop),
      Padding & ~(UINTN)EFI_PAGE_MASK,
      EfiConventionalMemory
    );
  }

  RemainingPages = (UINTN)(*FreeMemoryTop - *FreeMemoryBottom) >> EFI_PAGE_SHIFT;
  Pages = ALIGN_VALUE(Pages, EFI_SIZE_TO_PAGES(Granularity));
  if (RemainingPages < Pages) {
    printf("AllocatePages failed: No 0x%lx Pages is available.\n", Pages);
    printf("There is only left 0x%lx pages memory resource to be allocated.\n", RemainingPages);
    ASSERT(FALSE);
  }

  *(FreeMemoryTop) -= Pages * EFI_PAGE_SIZE;
  *Memory = *(FreeMemoryTop);

  BuildMemoryAllocationHob(
    *(FreeMemoryTop),
    Pages * EFI_PAGE_SIZE,
    MemoryType
  );
}

VOID*
EFIAPI
AllocatePages(
  IN UINTN  Pages
)
{
  EFI_PHYSICAL_ADDRESS  Memory;

  InternalAllocatePages(Pages, EfiBootServicesData, &Memory);
  return (VOID*)Memory;
}
