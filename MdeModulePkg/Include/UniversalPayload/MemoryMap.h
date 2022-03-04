/** @file
 Define the structure for the Universal Payload Memory map.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Scalable Firmware Specification (https://universalscalablefirmware.github.io/documentation/index.html)
**/

#ifndef UNIVERSAL_PAYLOAD_MEMORY_MAP_H_
#define UNIVERSAL_PAYLOAD_MEMORY_MAP_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack (1)

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  UINT16                              Count;
  UINT16                              DescriptorSize;
  EFI_MEMORY_DESCRIPTOR               MemoryMap[0];
} UNIVERSAL_PAYLOAD_MEMORY_MAP;

#pragma pack()

#define UNIVERSAL_PAYLOAD_MEMORY_MAP_REVISION  1

extern GUID  gUniversalPayloadMemoryMapGuid;
#endif
