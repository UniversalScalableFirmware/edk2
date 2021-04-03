/** @file
  This file defines the hob structure for SMBIOS table.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMBIOS_TABLE_GUID_H__
#define __SMBIOS_TABLE_GUID_H__

///
/// SMBIOS TABLE HOB GUID
///
extern EFI_GUID       gEfiSmbiosTableGuid;
extern EFI_GUID       gEfiSmbios3TableGuid;

#pragma pack(1) 
/// 
/// Bootloader SMBIOS table hob 
/// 
typedef struct { 
   UINT64                  TableAddress;
} SMBIOS_TABLE_HOB; 
#pragma pack()  

#endif
