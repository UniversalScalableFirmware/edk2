/** @file
  This file defines the hob structure for the SPI flash variable info.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SPI_FLASH_INFO_GUID_H__
#define __SPI_FLASH_INFO_GUID_H__

#include <IndustryStandard/Acpi.h>
//
// SPI Flash infor hob GUID
//
extern EFI_GUID gSpiFlashInfoGuid;

//
// Set this bit if platform need disable SMM write protection when writing flash
// in SMM mode using this method:  -- AsmWriteMsr32 (0x1FE, MmioRead32 (0xFED30880) | BIT0);
//
#define FLAGS_SPI_DISABLE_SMM_WRITE_PROTECT     BIT0

//
// Reuse ACPI definition
//
typedef EFI_ACPI_3_0_GENERIC_ADDRESS_STRUCTURE PLD_GENERIC_ADDRESS;
#define SPACE_ID_PCI_CONFIGURATION              EFI_ACPI_3_0_PCI_CONFIGURATION_SPACE
#define REGISTER_BIT_WIDTH_DWORD                EFI_ACPI_3_0_DWORD

typedef struct {
  UINT8                        Revision;
  UINT8                        Reserved;
  UINT16                       Flags;
  PLD_GENERIC_ADDRESS          SpiAddress; 
} SPI_FLASH_INFO;

#endif
