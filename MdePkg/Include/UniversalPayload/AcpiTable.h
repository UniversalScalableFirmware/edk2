/** @file
 Define the structure for the Payload APCI table.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLD_ACPI_TABLE_H_
#define _PLD_ACPI_TABLE_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack(1)

typedef struct {
  PLD_GENERIC_HEADER   PldHeader;
  EFI_PHYSICAL_ADDRESS Rsdp;
} PLD_ACPI_TABLE;

#pragma pack()

extern GUID gPldAcpiTableGuid;

#endif //_PLD_ACPI_TABLE_H_
