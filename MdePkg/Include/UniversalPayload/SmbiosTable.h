/** @file
 Define the structure for the Payload SmBios.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLD_SMBIOS_TABL_H_
#define _PLD_SMBIOS_TABL_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack (1)

typedef struct {
  PLD_GENERIC_HEADER   PldHeader;
  EFI_PHYSICAL_ADDRESS SmBiosEntryPoint;
} PLD_SMBIOS_TABLE;

#pragma pack()

extern GUID gPldSmbios3TableGuid;
extern GUID gPldSmbiosTableGuid;
#endif //_PLD_SMBIOS_TABL_H_
