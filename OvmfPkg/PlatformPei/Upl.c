/** @file
  Build FV related hobs for platform.

  Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiPei.h"
#include "Platform.h"
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Guid/SerialPortInfo.h>
#include <Guid/AcpiBoardInfoGuid.h>

#include <OvmfPlatforms.h>

/**
  Publish the FV that includes the UPL.
  Publish the UPL required HOBs.

**/
VOID
UplInitialization (
  VOID
  )
{
  EFI_FIRMWARE_VOLUME_HEADER        *UplFv;
  PLD_SERIAL_PORT_INFO              *Serial;
  ACPI_BOARD_INFO                   *AcpiBoardInfo;
  UINT16                            HostBridgeDevId;
  UINTN                             Pmba;

  DEBUG ((DEBUG_INFO, "=====================Report UPL FV=======================================\n"));
  UplFv = (EFI_FIRMWARE_VOLUME_HEADER *) PcdGet32 (PcdOvmfPldFvBase);
  ASSERT (UplFv->FvLength == PcdGet32 (PcdOvmfPldFvSize));

  PeiServicesInstallFvInfoPpi (&UplFv->FileSystemGuid, UplFv, (UINT32) UplFv->FvLength, NULL, NULL);

  DEBUG ((DEBUG_INFO, "=====================Build UPL HOBs=======================================\n"));
  //
  // Query Host Bridge DID to determine platform type
  //
  HostBridgeDevId = PcdGet16 (PcdOvmfHostBridgePciDevId);
  switch (HostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      Pmba = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMBA);
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      Pmba = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __FUNCTION__, HostBridgeDevId));
      ASSERT (FALSE);
  }


  Serial = BuildGuidHob (&gPldSerialPortInfoGuid, sizeof (PLD_SERIAL_PORT_INFO));
  Serial->BaudRate = PcdGet32 (PcdSerialBaudRate);
  Serial->RegisterBase = PcdGet64 (PcdSerialRegisterBase);
  Serial->RegisterWidth = (UINT8) PcdGet32 (PcdSerialRegisterStride);
  Serial->Revision = 1;
  Serial->UseMmio = PcdGetBool (PcdSerialUseMmio);


  AcpiBoardInfo = BuildGuidHob (&gUefiAcpiBoardInfoGuid, sizeof (ACPI_BOARD_INFO));
  AcpiBoardInfo->PcieBaseAddress = PcdGet64 (PcdPciExpressBaseAddress);
  AcpiBoardInfo->PcieBaseSize = SIZE_256MB;

  AcpiBoardInfo->PmTimerRegBase = (PciRead32 (Pmba) & ~PMBA_RTE) + ACPI_TIMER_OFFSET;
}

