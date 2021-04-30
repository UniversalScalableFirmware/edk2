/** @file
  This file defines the EFI Payload Platfrom BootManager Protocol.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This Protocol is introduced in UEFI Specification 2.6

**/

#ifndef __EFI_PLD_PLATFORM_BOOTMANAGER_H__
#define __EFI_PLD_PLATFORM_BOOTMANAGER_H__

typedef struct _EFI_PLD_PLATFORM_BOOTMANAGER_PROTOCOL  EFI_PLD_PLATFORM_BOOTMANAGER_PROTOCOL;

typedef
VOID
(EFIAPI *EFI_BEFORE_CONSOLE) (
  VOID
  );

typedef
VOID
(EFIAPI *EFI_AFTER_CONSOLE) (
  VOID
  );

typedef
VOID
(EFIAPI *EFI_WAIT_CALLBACK) (
  UINT16          TimeoutRemain
  );

typedef
VOID
(EFIAPI *EFI_UNABLE_TO_BOOT) (
  VOID
  );

struct _EFI_PLD_PLATFORM_BOOTMANAGER_PROTOCOL {
  EFI_BEFORE_CONSOLE        BeforeConsole;
  EFI_AFTER_CONSOLE         AfterConsole;
  EFI_WAIT_CALLBACK         WaitCallback;
  EFI_UNABLE_TO_BOOT        UnableToBoot;
};

extern EFI_GUID gEfiPldPlatformBootManagerProtocolGuid;

#endif
