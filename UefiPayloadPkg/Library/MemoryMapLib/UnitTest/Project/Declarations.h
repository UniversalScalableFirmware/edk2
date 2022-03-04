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

#ifndef _DECLARATIONS_
#define _DECLARATIONS_

#include <Base.h>
#include <Uefi.h>
#include <PiPei.h>
#include <Uefi/UefiSpec.h>
#include <Pi/PiHob.h>
#include <Library/DebugLib.h>
#include <UniversalPayload/MemoryMap.h>
#include <Library/MemoryMapLib.h>

#include <Library/HobLib.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

VOID
PrintHob (
  IN CONST VOID  *HobStart
  );

VOID *
EFIAPI
CreateHandoffTableHob (
  VOID
  );

EFI_HOB_HANDOFF_INFO_TABLE *
EFIAPI
HobConstructor (
  IN VOID  *EfiMemoryBottom,
  IN VOID  *EfiMemoryTop,
  IN VOID  *EfiFreeMemoryBottom,
  IN VOID  *EfiFreeMemoryTop
  );

UINT64
ReadUnaligned64 (
  CONST UINT64  *Buffer
  );

INTN
StrCmp (
  CONST CHAR16  *FirstString,
  CONST CHAR16  *SecondString
  );

VOID
ConstructNewHandOffHob (
  VOID
  );

VOID
EFIAPI
BuildResourceDescriptorHob (
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes
  );

VOID
CreateRemainingHobs (
  VOID    *HobList1,
  VOID    *HobList2,
  UINT64  Base,
  UINT64  Limit
  );

EFI_STATUS
EFIAPI
CreateHobsBasedOnMemoryMap (
  IN UNIVERSAL_PAYLOAD_MEMORY_MAP  *MemoryMapHob
  );

VOID
VerifyHob (
  IN CONST VOID  *HobList1Start,
  IN CONST VOID  *HobList2Start
  );

UINT64
Get64BitRandomNumber (
  VOID
  );

typedef
VOID
(EFIAPI *TEST_CASE_TO_CREATE_HOBS)(
  VOID   *HobList1,
  VOID   *HobList2
  );

typedef struct {
  RETURN_STATUS               ExpectedStatus;
  TEST_CASE_TO_CREATE_HOBS    TestCaseFunction;
  UINTN                       LineNumber;
  CHAR8                       *FileName;
} TEST_CASE;


EFI_RESOURCE_ATTRIBUTE_TYPE
ConvertCapabilitiesToResourceDescriptorHobAttributes(
  UINT64  Capabilities
);

EFI_RESOURCE_TYPE
ConvertEfiMemoryTypeToResourceDescriptorHobResourceType(
  EFI_MEMORY_TYPE  Type
);

#endif // _DECLARATIONS_
