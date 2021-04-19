/** @file
  This file defines the hob structure for image base for Payload

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLD_IMAGE_BASE_GUID_H__
#define __PLD_IMAGE_BASE_GUID_H__

///
/// PLD Fv image base GUID
///
extern GUID gPldImageBaseGuid;

typedef struct {
  UINT64             Base;
} PLD_IMAGE_BASE_HOB;

#endif
