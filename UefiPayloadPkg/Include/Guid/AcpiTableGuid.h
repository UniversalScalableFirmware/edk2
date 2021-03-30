/** @file
  This file defines the hob structure for ACPI table.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ACPI_TABLE_GUID_H__
#define __ACPI_TABLE_GUID_H__

///
/// ACPI TABLE HOB GUID
///
extern EFI_GUID            gEfiAcpi20TableGuid;

#pragma pack(1) 
/// 
/// Bootloader acpi table hob 
/// 
typedef struct { 
   UINT64                  TableAddress;
} ACPI_TABLE_HOB;
#pragma pack()  

#endif
