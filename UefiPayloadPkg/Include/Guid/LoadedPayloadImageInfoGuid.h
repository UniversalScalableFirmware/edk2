/** @file
  This file defines the hob structure used for Platform information.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LOADED_PAYLOAD_IMAGE_INFO_GUID_H__
#define __LOADED_PAYLOAD_IMAGE_INFO_GUID_H__

extern EFI_GUID gLoadedPayloadImageInfoGuid;

#pragma pack(1)

typedef struct {
  CHAR8                   Name[16];
  EFI_PHYSICAL_ADDRESS    Base;
  UINT64                  Size;
} PAYLOAD_IMAGE_ENTRY;

typedef struct {
  UINT8                  Revision;
  UINT8                  Reserved[3];
  UINT32                 EntryNum;
  PAYLOAD_IMAGE_ENTRY    Entry[0];
} LOADED_PAYLOAD_IMAGE_INFO;

#pragma pack()

#endif
