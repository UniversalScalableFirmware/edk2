/** @file
  Platform Hook Library instance for UART device.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Library/PciLib.h>
#include <Library/PlatformHookLib.h>
#include <Library/BlParseLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>


/**
  Performs platform specific initialization required for the CPU to access
  the hardware associated with a SerialPortLib instance.  This function does
  not initialize the serial port hardware itself.  Instead, it initializes
  hardware devices that are required for the CPU to access the serial port
  hardware.  This function may be called more than once.

  @retval RETURN_SUCCESS       The platform specific initialization succeeded.
  @retval RETURN_DEVICE_ERROR  The platform specific initialization could not be completed.

**/
RETURN_STATUS
EFIAPI
PlatformHookSerialPortInitialize (
  VOID
  )
{
  RETURN_STATUS         Status;
  SERIAL_PORT_INFO      *SerialPortInfo;
  UINT8                 *GuidHob;

  GuidHob = GetNextGuidHob (&gUefiSerialPortInfoGuid, (VOID*)(UINTN)GET_BOOTLOADER_PARAMETER ());
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }
  SerialPortInfo  = (SERIAL_PORT_INFO *) GET_GUID_HOB_DATA (GuidHob);

  Status = PcdSetBoolS (PcdSerialUseMmio, SerialPortInfo->UseMmio);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet64S (PcdSerialRegisterBase, SerialPortInfo->RegisterBase);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet32S (PcdSerialRegisterStride, SerialPortInfo->RegisterWidth);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet32S (PcdSerialBaudRate, SerialPortInfo->BaudRate);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet64S (PcdUartDefaultBaudRate, SerialPortInfo->BaudRate);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  return RETURN_SUCCESS;
}

