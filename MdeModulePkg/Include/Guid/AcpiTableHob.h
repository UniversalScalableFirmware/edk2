/** @file
 Define the GUID gPldAcpiTableGuid HOB struct.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ACPI_TABLE_HOB_H_
#define _ACPI_TABLE_HOB_H_

#include <Uefi.h>

#pragma pack(1)
typedef struct {
  EFI_PHYSICAL_ADDRESS          Rsdp;
} PLD_ACPI_TABLE_HOB;

#pragma pack()

extern GUID gPldAcpiTableGuid;

#endif
