/** @file
  Platform BDS customizations.

  Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PlatformBootManagerLib.h>
#include <Protocol/PlatformBootManagerOverride.h>


STATIC UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_PROTOCOL mUniversalPayloadPlatformBootManager = {
  PlatformBootManagerBeforeConsole,
  PlatformBootManagerAfterConsole,
  PlatformBootManagerWaitCallback,
  PlatformBootManagerUnableToBoot,
};


/**
  Entry point for this driver.

  @param  FileHandle      Handle of the file being invoked.
  @param  PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/

EFI_STATUS
EFIAPI
PlatformBootManagerDriverEntry (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
    EFI_STATUS                   Status;
    Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gUniversalPayloadPlatformBootManagerOverrideProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mUniversalPayloadPlatformBootManager
                  );
    return Status;
}