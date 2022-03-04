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

#define PAGE_HEAD_PRIVATE_SIGNATURE  SIGNATURE_32 ('P', 'H', 'D', 'R')

extern EFI_GUID  gEfiHobMemoryAllocModuleGuid = {
  0xF8E21975, 0x0899, 0x4F58, { 0xA4, 0xBE, 0x55, 0x25, 0xA9, 0xC6, 0xD7, 0x7A }
};
extern EFI_GUID  gEfiHobMemoryAllocStackGuid = {
  0x4ED4BF27, 0x4092, 0x42E9, { 0x80, 0x7D, 0x52, 0x7B, 0x1D, 0x00, 0xC9, 0xBD }
};
extern EFI_GUID  gUniversalPayloadMemoryMapGuid = {
  0x60ae3012, 0xea8d, 0x4010, { 0xa9, 0x9f, 0xb7, 0xe2, 0xf2, 0x90, 0x82, 0x55 }
};
extern EFI_GUID  gUefiAcpiBoardInfoGuid = {
  0xad3d31b, 0xb3d8, 0x4506, { 0xae, 0x71, 0x2e, 0xf1, 0x10, 0x6, 0xd9, 0xf }
};
extern EFI_GUID  gUniversalPayloadAcpiTableGuid = {
  0x9f9a9506, 0x5597, 0x4515, { 0xba, 0xb6, 0x8b, 0xcd, 0xe7, 0x84, 0xba, 0x87 }
};
extern EFI_GUID  gUniversalPayloadPciRootBridgeInfoGuid = {
  0xec4ebacb, 0x2638, 0x416e, { 0xbe, 0x80, 0xe5, 0xfa, 0x4b, 0x51, 0x19, 0x01 }
};
extern EFI_GUID  gUniversalPayloadSmbios3TableGuid = {
  0x92b7896c, 0x3362, 0x46ce, { 0x99, 0xb3, 0x4f, 0x5e, 0x3c, 0x34, 0xeb, 0x42 }
};
extern EFI_GUID  gUniversalPayloadSmbiosTableGuid = {
  0x590a0d26, 0x06e5, 0x4d20, { 0x8a, 0x82, 0x59, 0xea, 0x1b, 0x34, 0x98, 0x2d }
};
extern EFI_GUID  gUniversalPayloadExtraDataGuid = {
  0x15a5baf6, 0x1c91, 0x467d, { 0x9d, 0xfb, 0x31, 0x9d, 0x17, 0x8d, 0x4b, 0xb4 }
};
extern EFI_GUID  gUniversalPayloadSerialPortInfoGuid = {
  0xaa7e190d, 0xbe21, 0x4409, { 0x8e, 0x67, 0xa2, 0xcd, 0xf, 0x61, 0xe1, 0x70 }
};
extern EFI_GUID  gEfiMemoryTypeInformationGuid = {
  0x4C19049F, 0x4137, 0x4DD3, { 0x9C, 0x10, 0x8B, 0x97, 0xA8, 0x3F, 0xFD, 0xFA }
};
extern EFI_GUID  gEdkiiBootManagerMenuFileGuid = {
  0xdf939333, 0x42fc, 0x4b2a, { 0xa5, 0x9e, 0xbb, 0xae, 0x82, 0x81, 0xfe, 0xef }
};
extern EFI_GUID  gEfiHobMemoryAllocBspStoreGuid = {
  0x564B33CD, 0xC92A, 0x4593, { 0x90, 0xBF, 0x24, 0x73, 0xE4, 0x3C, 0x63, 0x22 }
};

extern VOID  *mHobList;
#define MAX_DEBUG_MESSAGE_LENGTH  0x100
char  TempFormat[MAX_DEBUG_MESSAGE_LENGTH];

extern UINTN  ErrorLineNumber;
extern CHAR8  *ErrorFileName;

BOOLEAN  IgnoreOtherAssert;

typedef struct {
  UINT32    Signature;
  VOID      *AllocatedBufffer;
  UINTN     TotalPages;
  VOID      *AlignedBuffer;
  UINTN     AlignedPages;
} PAGE_HEAD;

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

EFI_GUID *
CopyGuid (
  EFI_GUID        *DestinationGuid,
  CONST EFI_GUID  *SourceGuid
  )
{
  *DestinationGuid = *SourceGuid;

  return DestinationGuid;
}

EFIAPI
InternalAllocatePages (
  IN UINTN  Pages,
  IN EFI_MEMORY_TYPE MemoryType,
  OUT EFI_PHYSICAL_ADDRESS* Memory
  ) {
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS  *FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS  *FreeMemoryBottom;
  UINTN                 RemainingPages;
  UINTN                 Granularity;
  UINTN                 Padding;

  Hob.Raw     = (UINT8 *)mHobList;
  Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;

  FreeMemoryTop    = &(Hob.HandoffInformationTable->EfiFreeMemoryTop);
  FreeMemoryBottom = &(Hob.HandoffInformationTable->EfiFreeMemoryBottom);
  Padding          = *(FreeMemoryTop) & (Granularity - 1);

  if ((UINTN)(*FreeMemoryTop - *FreeMemoryBottom) < Padding) {
    printf ("AllocatePages failed: Out of space after padding\n");
    ASSERT (FALSE);
  }

  *(FreeMemoryTop) -= Padding;
  if (Padding >= EFI_PAGE_SIZE) {
    //
    // Create a memory allocation HOB to cover
    // the pages that we will lose to rounding
    //
    BuildMemoryAllocationHob (
      *(FreeMemoryTop),
      Padding & ~(UINTN)EFI_PAGE_MASK,
      EfiConventionalMemory
      );
  }

  RemainingPages = (UINTN)(*FreeMemoryTop - *FreeMemoryBottom) >> EFI_PAGE_SHIFT;
  Pages          = ALIGN_VALUE (Pages, EFI_SIZE_TO_PAGES (Granularity));
  if (RemainingPages < Pages) {
    printf ("AllocatePages failed: No 0x%lx Pages is available.\n", Pages);
    printf ("There is only left 0x%lx pages memory resource to be allocated.\n", RemainingPages);
    ASSERT (FALSE);
  }

  *(FreeMemoryTop) -= Pages * EFI_PAGE_SIZE;
  *Memory           = *(FreeMemoryTop);
  // printf ("Warning: Allocate page");
  BuildMemoryAllocationHob (
    *(FreeMemoryTop),
    Pages * EFI_PAGE_SIZE,
    MemoryType
    );
}

VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
{
  EFI_PHYSICAL_ADDRESS  Memory;

  InternalAllocatePages (Pages, EfiBootServicesData, &Memory);
  return (VOID *)Memory;
}

VOID *
EFIAPI
ZeroMem (
  OUT VOID  *Buffer,
  IN UINTN  Length
  )
{
  UINT8  Value = 0;

  for ( ; Length != 0; Length--) {
    ((UINT8 *)Buffer)[Length - 1] = Value;
  }

  return Buffer;
}

INTN
StrCmp (
  CONST CHAR16  *FirstString,
  CONST CHAR16  *SecondString
  )
{
  while ((*FirstString != L'\0') && (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
  }

  return *FirstString - *SecondString;
}

UINT64
ReadUnaligned64 (
  CONST UINT64  *Buffer
  )
{
  ASSERT (Buffer != NULL);

  return *Buffer;
}

BOOLEAN
EFIAPI
IsZeroGuid (
  IN CONST GUID  *Guid
  )
{
  UINT64  LowPartOfGuid;
  UINT64  HighPartOfGuid;

  LowPartOfGuid  = ReadUnaligned64 ((CONST UINT64 *)Guid);
  HighPartOfGuid = ReadUnaligned64 ((CONST UINT64 *)Guid + 1);

  // return (BOOLEAN)(LowPartOfGuid == 0 && HighPartOfGuid == 0);
  return TRUE;
}

BOOLEAN
EFIAPI
CompareGuid (
  IN CONST GUID  *Guid1,
  IN CONST GUID  *Guid2
  )
{
  UINT64  LowPartOfGuid1;
  UINT64  LowPartOfGuid2;
  UINT64  HighPartOfGuid1;
  UINT64  HighPartOfGuid2;

  LowPartOfGuid1  = ReadUnaligned64 ((CONST UINT64 *)Guid1);
  LowPartOfGuid2  = ReadUnaligned64 ((CONST UINT64 *)Guid2);
  HighPartOfGuid1 = ReadUnaligned64 ((CONST UINT64 *)Guid1 + 1);
  HighPartOfGuid2 = ReadUnaligned64 ((CONST UINT64 *)Guid2 + 1);

  return (BOOLEAN)(LowPartOfGuid1 == LowPartOfGuid2 && HighPartOfGuid1 == HighPartOfGuid2);
}

void
myassert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  BOOLEAN         x
  )
{
  CHAR8  *NewFileName;

  if (IgnoreOtherAssert) {
    return;
  }

  if (x) {
  } else {
    printf ("%s %d\n", FileName, LineNumber);
    NewFileName = strrchr (FileName, '\\');
    if ((LineNumber == ErrorLineNumber) && !(strcmp (NewFileName+1, ErrorFileName))) {
      ErrorLineNumber   = 0;
      IgnoreOtherAssert = TRUE;
      ErrorFileName     = NULL;
      return;
    }

    printf ("will assert\n");
  }

  assert (x);
}
