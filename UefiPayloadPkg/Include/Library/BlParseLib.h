/** @file
  This library will parse the coreboot table in memory and extract those required
  information.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Guid/GraphicsInfoHob.h>
#include <Guid/MemoryMapInfoGuid.h>
#include <Guid/SerialPortInfoGuid.h>
#include <Guid/SmBiosTableHob.h>
#include <Guid/AcpiTableHob.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/SmmRegisterInfoGuid.h>
#include <Guid/SmmS3CommunicationInfoGuid.h>
#include <Guid/SpiFlashInfoGuid.h>
#include <Guid/NvVariableInfoGuid.h>

#ifndef __BOOTLOADER_PARSE_LIB__
#define __BOOTLOADER_PARSE_LIB__

#define GET_BOOTLOADER_PARAMETER(num)         (*(UINTN *)(UINTN)(PcdGet32(PcdPayloadStackTop) - num * sizeof(UINT64)))
#define GET_BOOTLOADER_PARAMETER_ADDR(num)    ((UINTN *)(UINTN)(PcdGet32(PcdPayloadStackTop) - num * sizeof(UINT64)))
#define SET_BOOTLOADER_PARAMETER(num, Value)  GET_BOOTLOADER_PARAMETER(num) = Value

typedef RETURN_STATUS \
        (*BL_MEM_INFO_CALLBACK) (MEMROY_MAP_ENTRY *MemoryMapEntry, VOID *Param);

/**
  This function retrieves the parameter base address from boot loader.

  This function will get bootloader specific parameter address for UEFI payload.
  e.g. HobList pointer for Slim Bootloader, and coreboot table header for Coreboot.

  @retval NULL            Failed to find the GUID HOB.
  @retval others          GUIDed HOB data pointer.

**/
VOID *
EFIAPI
GetParameterBase (
  VOID
  );

/**
  Acquire the memory map information.

  @param  MemInfoCallback     The callback routine
  @param  Params              Pointer to the callback routine parameter

  @retval RETURN_SUCCESS     Successfully find out the memory information.
  @retval RETURN_NOT_FOUND   Failed to find the memory information.

**/
RETURN_STATUS
EFIAPI
ParseMemoryInfo (
  IN  BL_MEM_INFO_CALLBACK       MemInfoCallback,
  IN  VOID                       *Params
  );

/**
  Acquire smbios table from slim bootloader

  @param  SystemTableInfo           Pointer to the SMBIOS table info

  @retval RETURN_SUCCESS            Successfully find out the tables.
  @retval RETURN_NOT_FOUND          Failed to find the tables.

**/
RETURN_STATUS
EFIAPI
ParseSmbiosTable (
  OUT PLD_SMBIOS_TABLE_HOB  *SmbiosTable
  );

/**
  Acquire acpi table from slim bootloader

  @param  AcpiTableHob              Pointer to the ACPI table info

  @retval RETURN_SUCCESS            Successfully find out the tables.
  @retval RETURN_NOT_FOUND          Failed to find the tables.

**/
RETURN_STATUS
EFIAPI
ParseAcpiTableInfo (
  OUT PLD_ACPI_TABLE_HOB   *AcpiTableHob
  );


/**
  Find the serial port information

  @param  SERIAL_PORT_INFO   Pointer to serial port info structure

  @retval RETURN_SUCCESS     Successfully find the serial port information.
  @retval RETURN_NOT_FOUND   Failed to find the serial port information .

**/
RETURN_STATUS
EFIAPI
ParseSerialInfo (
  OUT SERIAL_PORT_INFO     *SerialPortInfo
  );


/**
  Find the video frame buffer information

  @param  GfxInfo             Pointer to the EFI_PEI_GRAPHICS_INFO_HOB structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information .

**/
RETURN_STATUS
EFIAPI
ParseGfxInfo (
  OUT EFI_PEI_GRAPHICS_INFO_HOB       *GfxInfo
  );

/**
  Find the video frame buffer device information

  @param  GfxDeviceInfo      Pointer to the EFI_PEI_GRAPHICS_DEVICE_INFO_HOB structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information .

**/
RETURN_STATUS
EFIAPI
ParseGfxDeviceInfo (
  OUT EFI_PEI_GRAPHICS_DEVICE_INFO_HOB       *GfxDeviceInfo
  );


#endif
