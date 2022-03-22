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
extern TEST_CASE_STRUCT gTestCase[];

VOID*
CreateHandOffHob(
  VOID
)
{
  VOID*   MemBottom;
  VOID*   MemTop;
  VOID*   FreeMemBottom;
  VOID*   FreeMemTop;
  VOID*   Range;
  VOID*   AlignedRange;
  UINTN   AlignmentMask;
  UINTN   Alignment;
  UINT64  Length;

  Length = SIZE_8MB;
  Alignment = SIZE_4KB;
  AlignmentMask = Alignment - 1;

  Range = malloc(SIZE_16MB);
  if (Range == NULL) {
      printf("Memory is not allocated\n");
      ASSERT(FALSE);
  }

  AlignedRange = (VOID*)(((UINTN)Range + AlignmentMask) & ~AlignmentMask);

  MemBottom = AlignedRange;
  MemTop = (VOID*)((UINTN)MemBottom + Length);
  FreeMemBottom = (VOID*)((UINTN)MemBottom);
  FreeMemTop = (VOID*)((UINTN)MemTop);

  HobConstructor(MemBottom, MemTop, FreeMemBottom, FreeMemTop);
  return Range;
}

int
main (
  )
{
  EFI_STATUS Status;
  VOID*      Range;
  UINTN      Index;
  UINTN      TestCaseCount;

  TestCaseCount = GetTestCaseCount();

  Range = CreateHandOffHob ();
  SetUplDataLibConstructor();

  for (Index = 0; Index < TestCaseCount; Index++) {
      printf("Run TestCase for %d times\n", Index + 1);
      Status = gTestCase[Index].FunctionName();
      assert(Status == gTestCase[Index].ExpectedStatus);
  }

  free(Range);
  return 0;
}
