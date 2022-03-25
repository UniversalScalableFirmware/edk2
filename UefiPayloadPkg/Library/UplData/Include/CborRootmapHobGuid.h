/** @file
  This file defines the CBOR root map structure.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CBOR_ROOTMAP_HOB_GUID_H_
#define __CBOR_ROOTMAP_HOB_GUID_H_

extern EFI_GUID CborRootmapHobGuid;

typedef struct {
  VOID *RootEncoder ;
  VOID *RootMapEncoder;
  VOID *Buffer;
} CBOR_ROOTMAP_INFO;

#endif
