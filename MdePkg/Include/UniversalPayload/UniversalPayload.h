/** @file
  Universal Payload general definations.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UNIVERSAL_PAYLOAD_H__
#define __UNIVERSAL_PAYLOAD_H__

typedef  VOID   (EFIAPI *UNIVERSAL_PAYLOAD_ENTRY) (VOID *HobList);

#define PLD_IDENTIFIER                   SIGNATURE_32('U', 'P', 'L', 'D')
#define PLD_INFO_SEC_NAME                ".upld_info"
#define PLD_EXTRA_SEC_NAME_PREFIX        ".upld."
#define PLD_EXTRA_SEC_NAME_PREFIX_LENGTH (sizeof (PLD_EXTRA_SEC_NAME_PREFIX) - 1)

#pragma pack(1)

typedef struct {
  UINT32                          Identifier;
  UINT32                          HeaderLength;
  UINT16                          SpecRevision;
  UINT8                           Reserved[2];
  UINT32                          Revision;
  UINT32                          Attribute;
  UINT32                          Capability;
  CHAR8                           ProducerId[16];
  CHAR8                           ImageId[16];
} PLD_INFO_HEADER;

typedef struct {
  UINT8 Revision;
  UINT8 Reserved[3];
} PLD_GENERIC_HEADER;

#pragma pack()

#define PLD_GENERIC_HEADER_REVISION 0

#endif // __UNIVERSAL_PAYLOAD_H__
