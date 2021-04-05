/** @file
  PCI Segment Information Library that returns one segment whose
  segment base address is retrieved from AcpiBoardInfo HOB.

  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/PciSegmentInfoLib.h>
#include <Library/DebugLib.h>

STATIC PCI_SEGMENT_INFO mPciSegment0 = {
  0,  // Segment number
  0,  // To be fixed later
  0,  // Start bus number
  255 // End bus number
};


RETURN_STATUS
EFIAPI
PciSegmentInfoInitialize (
  VOID
  )
{
  mPciSegment0.BaseAddress = PcdGet64 (PcdPciExpressBaseAddress);
  return RETURN_SUCCESS;
}

/**
  Return an array of PCI_SEGMENT_INFO holding the segment information.

  Note: The returned array/buffer is owned by callee.

  @param  Count  Return the count of segments.

  @retval A callee owned array holding the segment information.
**/
PCI_SEGMENT_INFO *
EFIAPI
GetPciSegmentInfo (
  UINTN  *Count
  )
{
  ASSERT (Count != NULL);
  if (Count == NULL) {
    return NULL;
  }

  *Count = 1;
  return &mPciSegment0;
}
