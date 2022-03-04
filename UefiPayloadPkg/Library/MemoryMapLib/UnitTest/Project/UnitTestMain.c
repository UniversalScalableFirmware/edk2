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

extern TEST_CASE  gTestCase[];

extern TEST_CASE  gTestCaseForMemMap[];
CHAR8             *ErrorFileName;
UINTN             ErrorLineNumber;
extern BOOLEAN    IgnoreOtherAssert;
UINTN
GetTestCaseCount (
  VOID
  );

UINTN
GetTestCaseForMemMapCount (
  VOID
  );

typedef enum {
  InTestCaseForHob    = 0,
  InTestCaseForMemMap = 1,
  InTestCaseRamdom    = 2
} TEST_CASE_TYPE;

VOID
VerifyHobList1WithMemoryMap (
  IN CONST VOID                    *HobList1Start,
  IN UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob
  );

VOID
VerifyHobList2WithMemoryMap (
  IN CONST VOID                    *HobList2Start,
  IN UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob
  );

VOID  *mHobList;

VOID *
EFIAPI
AllocateRangeForHobLists (
  VOID  **HobList1,
  VOID  **HobList2,
  VOID  **HobListCopy
  )
{
  VOID    *MemBottom;
  VOID    *MemTop;
  VOID    *FreeMemBottom;
  VOID    *FreeMemTop;
  VOID    *Range;
  VOID    *AlignedRange;
  UINTN   AlignmentMask;
  UINTN   Alignment;
  UINT64  Length;

  Length        = SIZE_32MB;
  Alignment     = SIZE_4KB;
  AlignmentMask = Alignment - 1;

  Range = malloc (SIZE_128MB + SIZE_128MB);
  if (Range == NULL) {
    printf ("Memory is not allocated\n");
    ASSERT (FALSE);
  }

  AlignedRange = (VOID *)(((UINTN)Range + AlignmentMask) & ~AlignmentMask);

  MemBottom     = AlignedRange;
  MemTop        = (VOID *)((UINTN)MemBottom + Length);
  FreeMemBottom = (VOID *)((UINTN)MemBottom);
  FreeMemTop    = (VOID *)((UINTN)MemTop);

  *HobList1 = HobConstructor (MemBottom, MemTop, FreeMemBottom, FreeMemTop);

  MemBottom     = (VOID *)((UINTN)MemBottom + SIZE_64MB);
  MemTop        = (VOID *)((UINTN)MemBottom + Length);
  FreeMemBottom = (VOID *)((UINTN)MemBottom);
  FreeMemTop    = (VOID *)((UINTN)MemTop);

  *HobList2    = HobConstructor (MemBottom, MemTop, FreeMemBottom, FreeMemTop);
  *HobListCopy = (VOID *)((UINTN)AlignedRange + SIZE_128MB);
  return Range;
}

VOID
EFIAPI
FixUpHobList1 (
  VOID  *HobList1,
  VOID  *HobList1Backup
  )
{
  EFI_PEI_HOB_POINTERS  Hob1;
  EFI_PEI_HOB_POINTERS  HobBackup;

  Hob1.Raw      = (UINT8 *)HobList1;
  HobBackup.Raw = (UINT8 *)HobList1Backup;

  while (TRUE) {
    if (HobBackup.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      Hob1.Header->HobType = EFI_HOB_TYPE_RESOURCE_DESCRIPTOR;
    } else if (HobBackup.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      Hob1.Header->HobType = EFI_HOB_TYPE_MEMORY_ALLOCATION;
    } else if (HobBackup.Header->HobType == EFI_HOB_TYPE_END_OF_HOB_LIST) {
      break;
    }

    Hob1.Raw      = GET_NEXT_HOB (Hob1);
    HobBackup.Raw = GET_NEXT_HOB (HobBackup);
  }
}

int
main (
  )
{
  VOID  *MemoryMapHob;

  VOID  *HobList1;
  VOID  *HobList2;
  VOID  *HobList1Backup;
  VOID  *Range;

  UINTN  Index;
  UINTN  TestCaseCount;
  UINTN  TestCaseCountForMemMap;

  RETURN_STATUS   Status;

  TestCaseCount          = GetTestCaseCount ();
  TestCaseCountForMemMap = GetTestCaseForMemMapCount ();
  IgnoreOtherAssert      = FALSE;
  printf ("TestCaseForHobs\n");

  for (Index = 0; Index < TestCaseCount; Index++) {
    printf ("Run TestCaseForHobs for %d times\n", Index + 1);

    Range           = AllocateRangeForHobLists (&HobList1, &HobList2, &HobList1Backup);
    mHobList        = HobList1;
    ErrorLineNumber = gTestCase[Index].LineNumber;
    ErrorFileName   = gTestCase[Index].FileName;
    gTestCase[Index].TestCaseFunction (HobList1, HobList2);
    CopyMem (HobList1Backup, HobList1, SIZE_64MB);

    Status            = BuildMemoryMapHob ();
    IgnoreOtherAssert = FALSE;
    assert (Status == gTestCase[Index].ExpectedStatus);
    if (Status != RETURN_SUCCESS) {
      assert (ErrorLineNumber == 0);
      assert (ErrorFileName == NULL);
    }

    free (Range);
  }

  for (Index = 0; Index < TestCaseCountForMemMap; Index++) {
    printf ("Run gTestCaseForMemMap for %d times\n", Index + 1);

    Range    = AllocateRangeForHobLists (&HobList1, &HobList2, &HobList1Backup);
    mHobList = HobList1;

    //
    // Directly create memory map table
    //
    ErrorLineNumber = gTestCaseForMemMap[Index].LineNumber;
    ErrorFileName   = gTestCaseForMemMap[Index].FileName;
    gTestCaseForMemMap[Index].TestCaseFunction (HobList1, HobList2);
    MemoryMapHob      = GetNextGuidHob (&gUniversalPayloadMemoryMapGuid, mHobList);
    mHobList          = HobList2;
    Status            = CreateHobsBasedOnMemoryMap ((UNIVERSAL_PAYLOAD_MEMORY_MAP *)GET_GUID_HOB_DATA (MemoryMapHob));
    IgnoreOtherAssert = FALSE;
    assert (Status == gTestCaseForMemMap[Index].ExpectedStatus);
    if (Status != RETURN_SUCCESS) {
      assert (ErrorLineNumber == 0);
      assert (ErrorFileName == NULL);
    } else {
      VerifyHobList2WithMemoryMap (HobList2, (UNIVERSAL_PAYLOAD_MEMORY_MAP *)GET_GUID_HOB_DATA (MemoryMapHob));
    }

    free (Range);
  }

  for (Index = 0; Index < 1000000; Index++) {
    printf ("Run Random TestCase for %d times\n", Index + 1);

    Range    = AllocateRangeForHobLists (&HobList1, &HobList2, &HobList1Backup);
    mHobList = HobList1;
    CreateRemainingHobs (HobList1, HobList2, 0, Get64BitRandomNumber ()); // Create end which is above hoblist2.limit

    CopyMem (HobList1Backup, HobList1, SIZE_64MB);

    Status = BuildMemoryMapHob ();

    FixUpHobList1 (HobList1, HobList1Backup);
    MemoryMapHob = GetNextGuidHob (&gUniversalPayloadMemoryMapGuid, mHobList);
    VerifyHobList1WithMemoryMap (HobList1, (UNIVERSAL_PAYLOAD_MEMORY_MAP *)GET_GUID_HOB_DATA (MemoryMapHob));

    mHobList = HobList2;
    CreateHobsBasedOnMemoryMap ((UNIVERSAL_PAYLOAD_MEMORY_MAP *)GET_GUID_HOB_DATA (MemoryMapHob));
    VerifyHob (HobList1, HobList2);
    VerifyHobList2WithMemoryMap (HobList2, (UNIVERSAL_PAYLOAD_MEMORY_MAP *)GET_GUID_HOB_DATA (MemoryMapHob));
    // PrintHob (HobList1);
    // PrintHob (HobList2);
    free (Range);
  }

  return 0;
}
