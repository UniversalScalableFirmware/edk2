/** @file
 Define the GUID gPldSmbios3TableGuid and gPldSmbiosTableGuid HOB struct.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMBIOS_TABLE_HOB_H_
#define _SMBIOS_TABLE_HOB_H_

#include <Uefi.h>

#pragma pack(1)

typedef struct {
  EFI_PHYSICAL_ADDRESS          SmBiosEntryPoint;
} PLD_SMBIOS_TABLE_HOB;

#pragma pack()

extern GUID gPldSmbios3TableGuid;
extern GUID gPldSmbiosTableGuid;
#endif
