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
#include <UniversalPayload/SerialPortInfo.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <Protocol/DevicePath.h>
#include <UniversalPayload/PciRootBridges.h>
#include <Library/QemuFwCfgLib.h>
#include <OvmfPlatforms.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/NvVariableInfoGuid.h>
#include <Guid/SpiFlashInfoGuid.h>
STATIC PLD_PCI_ROOT_BRIDGE_APERTURE mNonExistAperture = { MAX_UINT64, 0 };

EFI_STATUS
EFIAPI
PciHostBridgeUtilityInitRootBridge1 (
  IN  UINT64                   Supports,
  IN  UINT64                   Attributes,
  IN  UINT64                   AllocAttributes,
  IN  BOOLEAN                  DmaAbove4G,
  IN  BOOLEAN                  NoExtendedConfigSpace,
  IN  UINT8                    RootBusNumber,
  IN  UINT8                    MaxSubBusNumber,
  IN  PLD_PCI_ROOT_BRIDGE_APERTURE *Io,
  IN  PLD_PCI_ROOT_BRIDGE_APERTURE *Mem,
  IN  PLD_PCI_ROOT_BRIDGE_APERTURE *MemAbove4G,
  IN  PLD_PCI_ROOT_BRIDGE_APERTURE *PMem,
  IN  PLD_PCI_ROOT_BRIDGE_APERTURE *PMemAbove4G,
  OUT PLD_PCI_ROOT_BRIDGE      *RootBus
  )
{


  //
  // Be safe if other fields are added to PCI_ROOT_BRIDGE later.
  //
  ZeroMem (RootBus, sizeof *RootBus);

  RootBus->Segment = 0;

  RootBus->Supports   = Supports;
  RootBus->Attributes = Attributes;

  RootBus->DmaAbove4G = DmaAbove4G;

  RootBus->AllocationAttributes = AllocAttributes;
  RootBus->Bus.Base  = RootBusNumber;
  RootBus->Bus.Limit = MaxSubBusNumber;
  CopyMem (&RootBus->Io, Io, sizeof (*Io));
  CopyMem (&RootBus->Mem, Mem, sizeof (*Mem));
  CopyMem (&RootBus->MemAbove4G, MemAbove4G, sizeof (*MemAbove4G));
  CopyMem (&RootBus->PMem, PMem, sizeof (*PMem));
  CopyMem (&RootBus->PMemAbove4G, PMemAbove4G, sizeof (*PMemAbove4G));

  RootBus->NoExtendedConfigSpace = NoExtendedConfigSpace;


  RootBus->UID = RootBusNumber;
  RootBus->HID = EISA_PNP_ID(0x0A03);


  DEBUG ((DEBUG_INFO,
    "%a: populated root bus %d, with room for %d subordinate bus(es)\n",
    __FUNCTION__, RootBusNumber, MaxSubBusNumber - RootBusNumber));
  return EFI_SUCCESS;
}

PLD_PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeUtilityGetRootBridges (
  OUT UINTN                    *Count,
  IN  UINT64                   Attributes,
  IN  UINT64                   AllocationAttributes,
  IN  BOOLEAN                  DmaAbove4G,
  IN  BOOLEAN                  NoExtendedConfigSpace,
  IN  UINTN                    BusMin,
  IN  UINTN                    BusMax,
  IN  PLD_PCI_ROOT_BRIDGE_APERTURE *Io,
  IN  PLD_PCI_ROOT_BRIDGE_APERTURE *Mem,
  IN  PLD_PCI_ROOT_BRIDGE_APERTURE *MemAbove4G,
  IN  PLD_PCI_ROOT_BRIDGE_APERTURE *PMem,
  IN  PLD_PCI_ROOT_BRIDGE_APERTURE *PMemAbove4G
  )
{
  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;
  UINT64               ExtraRootBridges;
  PLD_PCI_ROOT_BRIDGE  *Bridges;
  UINTN                Initialized;
  UINTN                LastRootBridgeNumber;
  UINTN                RootBridgeNumber;

  *Count = 0;

  if (BusMin > BusMax || BusMax > PCI_MAX_BUS) {
    DEBUG ((DEBUG_ERROR, "%a: invalid bus range with BusMin %Lu and BusMax "
      "%Lu\n", __FUNCTION__, (UINT64)BusMin, (UINT64)BusMax));
    return NULL;
  }

  //
  // QEMU provides the number of extra root buses, shortening the exhaustive
  // search below. If there is no hint, the feature is missing.
  //
  Status = QemuFwCfgFindFile ("etc/extra-pci-roots", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status) || FwCfgSize != sizeof ExtraRootBridges) {
    ExtraRootBridges = 0;
  } else {
    QemuFwCfgSelectItem (FwCfgItem);
    QemuFwCfgReadBytes (FwCfgSize, &ExtraRootBridges);

    //
    // Validate the number of extra root bridges. As BusMax is inclusive, the
    // max bus count is (BusMax - BusMin + 1). From that, the "main" root bus
    // is always a given, so the max count for the "extra" root bridges is one
    // less, i.e. (BusMax - BusMin). If the QEMU hint exceeds that, we have
    // invalid behavior.
    //
    if (ExtraRootBridges > BusMax - BusMin) {
      DEBUG ((DEBUG_ERROR, "%a: invalid count of extra root buses (%Lu) "
        "reported by QEMU\n", __FUNCTION__, ExtraRootBridges));
      return NULL;
    }
    DEBUG ((DEBUG_INFO, "%a: %Lu extra root buses reported by QEMU\n",
      __FUNCTION__, ExtraRootBridges));
  }

  //
  // Allocate the "main" root bridge, and any extra root bridges.
  //
  Bridges = AllocatePool ((1 + (UINTN)ExtraRootBridges) * sizeof *Bridges);
  if (Bridges == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __FUNCTION__, EFI_OUT_OF_RESOURCES));
    return NULL;
  }
  Initialized = 0;

  //
  // The "main" root bus is always there.
  //
  LastRootBridgeNumber = BusMin;

  //
  // Scan all other root buses. If function 0 of any device on a bus returns a
  // VendorId register value different from all-bits-one, then that bus is
  // alive.
  //
  for (RootBridgeNumber = BusMin + 1;
       RootBridgeNumber <= BusMax && Initialized < ExtraRootBridges;
       ++RootBridgeNumber) {
    UINTN Device;

    for (Device = 0; Device <= PCI_MAX_DEVICE; ++Device) {
      if (PciRead16 (PCI_LIB_ADDRESS (RootBridgeNumber, Device, 0,
                       PCI_VENDOR_ID_OFFSET)) != MAX_UINT16) {
        break;
      }
    }
    if (Device <= PCI_MAX_DEVICE) {
      //
      // Found the next root bus. We can now install the *previous* one,
      // because now we know how big a bus number range *that* one has, for any
      // subordinate buses that might exist behind PCI bridges hanging off it.
      //
      Status = PciHostBridgeUtilityInitRootBridge1 (
        Attributes,
        Attributes,
        AllocationAttributes,
        DmaAbove4G,
        NoExtendedConfigSpace,
        (UINT8) LastRootBridgeNumber,
        (UINT8) (RootBridgeNumber - 1),
        Io,
        Mem,
        MemAbove4G,
        PMem,
        PMemAbove4G,
        &Bridges[Initialized]
        );
      if (EFI_ERROR (Status)) {
        goto FreeBridges;
      }
      ++Initialized;
      LastRootBridgeNumber = RootBridgeNumber;
    }
  }

  //
  // Install the last root bus (which might be the only, ie. main, root bus, if
  // we've found no extra root buses).
  //
  Status = PciHostBridgeUtilityInitRootBridge1 (
    Attributes,
    Attributes,
    AllocationAttributes,
    DmaAbove4G,
    NoExtendedConfigSpace,
    (UINT8) LastRootBridgeNumber,
    (UINT8) BusMax,
    Io,
    Mem,
    MemAbove4G,
    PMem,
    PMemAbove4G,
    &Bridges[Initialized]
    );
  if (EFI_ERROR (Status)) {
    goto FreeBridges;
  }
  ++Initialized;

  *Count = Initialized;
  return Bridges;

FreeBridges:
  //while (Initialized > 0) {
  //  --Initialized;
  //  PciHostBridgeUtilityUninitRootBridge (&Bridges[Initialized]);
  //}

  FreePool (Bridges);
  return NULL;
}


PLD_PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges1 (
  UINTN *Count
  )
{
  UINT64               Attributes;
  UINT64               AllocationAttributes;
  PLD_PCI_ROOT_BRIDGE_APERTURE Io;
  PLD_PCI_ROOT_BRIDGE_APERTURE Mem;
  PLD_PCI_ROOT_BRIDGE_APERTURE MemAbove4G;

  ZeroMem (&Io, sizeof (Io));
  ZeroMem (&Mem, sizeof (Mem));
  ZeroMem (&MemAbove4G, sizeof (MemAbove4G));

  Attributes = EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO |
    EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO |
    EFI_PCI_ATTRIBUTE_ISA_IO_16 |
    EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO |
    EFI_PCI_ATTRIBUTE_VGA_MEMORY |
    EFI_PCI_ATTRIBUTE_VGA_IO_16 |
    EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;

  AllocationAttributes = 1;
  if (PcdGet64 (PcdPciMmio64Size) > 0) {
    AllocationAttributes |= 2;
    MemAbove4G.Base = PcdGet64 (PcdPciMmio64Base);
    MemAbove4G.Limit = PcdGet64 (PcdPciMmio64Base) +
                       PcdGet64 (PcdPciMmio64Size) - 1;
  } else {
    CopyMem (&MemAbove4G, &mNonExistAperture, sizeof (mNonExistAperture));
  }

  Io.Base = PcdGet64 (PcdPciIoBase);
  Io.Limit = PcdGet64 (PcdPciIoBase) + (PcdGet64 (PcdPciIoSize) - 1);
  Mem.Base = PcdGet64 (PcdPciMmio32Base);
  Mem.Limit = PcdGet64 (PcdPciMmio32Base) + (PcdGet64 (PcdPciMmio32Size) - 1);

  return PciHostBridgeUtilityGetRootBridges (
    Count,
    Attributes,
    AllocationAttributes,
    FALSE,
    PcdGet16 (PcdOvmfHostBridgePciDevId) != INTEL_Q35_MCH_DEVICE_ID,
    0,
    PCI_MAX_BUS,
    &Io,
    &Mem,
    &MemAbove4G,
    &mNonExistAperture,
    &mNonExistAperture
    );
}



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
  PLD_PCI_ROOT_BRIDGES              *PciRootBridgeInfo;
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
  Serial->PldHeader.Revision = 0;
  Serial->BaudRate = PcdGet32 (PcdSerialBaudRate);
  Serial->RegisterBase = PcdGet64 (PcdSerialRegisterBase);
  Serial->RegisterWidth = (UINT8) PcdGet32 (PcdSerialRegisterStride);
  Serial->Revision = 1;
  Serial->UseMmio = PcdGetBool (PcdSerialUseMmio);


  AcpiBoardInfo = BuildGuidHob (&gUefiAcpiBoardInfoGuid, sizeof (ACPI_BOARD_INFO));
  AcpiBoardInfo->PcieBaseAddress = PcdGet64 (PcdPciExpressBaseAddress);
  AcpiBoardInfo->PcieBaseSize = SIZE_256MB;

  AcpiBoardInfo->PmTimerRegBase = (PciRead32 (Pmba) & ~PMBA_RTE) + ACPI_TIMER_OFFSET;



  PLD_PCI_ROOT_BRIDGE * RootBridge;
  UINTN         RootBridgeCount;
  RootBridge = PciHostBridgeGetRootBridges1(&RootBridgeCount);

  PciRootBridgeInfo = BuildGuidHob (&gPldPciRootBridgeInfoGuid, sizeof (PciRootBridgeInfo) + sizeof (PLD_PCI_ROOT_BRIDGE));
  CopyMem(PciRootBridgeInfo->RootBridge, RootBridge, sizeof (PLD_PCI_ROOT_BRIDGE));
  PciRootBridgeInfo->Count = (UINT8)RootBridgeCount;
  DEBUG ((DEBUG_ERROR, "%a: PciRootBridgeInfo->Count: 0x%04x\n",  __FUNCTION__, RootBridgeCount));
  DEBUG ((DEBUG_ERROR, "%a: PciRootBridgeInfo->RootBridge[0].ResourceAssigned: 0x%04x\n",  __FUNCTION__, PciRootBridgeInfo->RootBridge[0].ResourceAssigned));
  DEBUG ((DEBUG_ERROR, "%a: PciRootBridgeInfo->RootBridge[0].ResourceAssigned: 0x%x\n",  __FUNCTION__, (UINTN)PciRootBridgeInfo->RootBridge[0].Bus.Limit));
  //PciRootBridgeInfo->RootBridge[0].ResourceAssigned = FALSE;

  SPI_FLASH_INFO                   *NewSpiFlashInfo;
  NewSpiFlashInfo = BuildGuidHob (&gSpiFlashInfoGuid, sizeof (SPI_FLASH_INFO));
  NewSpiFlashInfo->Flags = TRUE;
  NewSpiFlashInfo->SpiAddress.AddressSpaceId =  EFI_ACPI_3_0_PCI_CONFIGURATION_SPACE;
  NewSpiFlashInfo->SpiAddress.RegisterBitWidth =  32;
  NewSpiFlashInfo->SpiAddress.RegisterBitOffset =  0;
  NewSpiFlashInfo->SpiAddress.AccessSize =  EFI_ACPI_3_0_DWORD;
  NewSpiFlashInfo->SpiAddress.Address = (UINT64) PcdGet32 (PcdOvmfFdBaseAddress);

  NV_VARIABLE_INFO                   *NewNvVariableInfo;
  NewNvVariableInfo = BuildGuidHob (&gNvVariableInfoGuid, sizeof (NV_VARIABLE_INFO));
  NewNvVariableInfo->VariableStoreBase = FixedPcdGet32 (PcdFlashNvStorageVariableBase64);
  NewNvVariableInfo->VariableStoreSize= 2 * (FixedPcdGet32 (PcdFlashNvStorageVariableSize) + 0x2000);
  // 0x2000 is a hard code value in UefiPayloadPkg\FvbRuntimeDxe\FvbInfo.c, line 95:   FtwWorkingSize = 0x2000;

}

