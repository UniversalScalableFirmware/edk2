/** @file
  Initializes HOBs related to UniversalPayload.

  Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiPei.h"
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
#include <Library/SetUplDataLib.h>

STATIC UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  mNonExistAperture = { MAX_UINT64, 0 };

/**
  Utility function to initialize a PCI_ROOT_BRIDGE structure.

  @param[in]  Supports               Supported attributes.

  @param[in]  Attributes             Initial attributes.

  @param[in]  AllocAttributes        Allocation attributes.

  @param[in]  DmaAbove4G             DMA above 4GB memory.

  @param[in]  NoExtendedConfigSpace  No Extended Config Space.

  @param[in]  RootBusNumber          The bus number to store in RootBus.

  @param[in]  MaxSubBusNumber        The inclusive maximum bus number that can
                                     be assigned to any subordinate bus found
                                     behind any PCI bridge hanging off this
                                     root bus.

                                     The caller is repsonsible for ensuring
                                     that RootBusNumber <= MaxSubBusNumber. If
                                     RootBusNumber equals MaxSubBusNumber, then
                                     the root bus has no room for subordinate
                                     buses.

  @param[in]  Io                     IO aperture.

  @param[in]  Mem                    MMIO aperture.

  @param[in]  MemAbove4G             MMIO aperture above 4G.

  @param[in]  PMem                   Prefetchable MMIO aperture.

  @param[in]  PMemAbove4G            Prefetchable MMIO aperture above 4G.

  @param[out] RootBus                The PCI_ROOT_BRIDGE structure (allocated
                                     by the caller) that should be filled in by
                                     this function.

  @retval EFI_SUCCESS                Initialization successful. A device path
                                     consisting of an ACPI device path node,
                                     with UID = RootBusNumber, has been
                                     allocated and linked into RootBus.

  @retval EFI_OUT_OF_RESOURCES       Memory allocation failed.
**/
EFI_STATUS
EFIAPI
PeiPciHostBridgeUtilityInitRootBridge (
  IN  UINT64                                      Supports,
  IN  UINT64                                      Attributes,
  IN  UINT64                                      AllocAttributes,
  IN  BOOLEAN                                     DmaAbove4G,
  IN  BOOLEAN                                     NoExtendedConfigSpace,
  IN  UINT8                                       RootBusNumber,
  IN  UINT8                                       MaxSubBusNumber,
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  *Io,
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  *Mem,
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G,
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  *PMem,
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G,
  OUT UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE           *RootBus
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
  RootBus->Bus.Base             = RootBusNumber;
  RootBus->Bus.Limit            = MaxSubBusNumber;
  CopyMem (&RootBus->Io, Io, sizeof (*Io));
  CopyMem (&RootBus->Mem, Mem, sizeof (*Mem));
  CopyMem (&RootBus->MemAbove4G, MemAbove4G, sizeof (*MemAbove4G));
  CopyMem (&RootBus->PMem, PMem, sizeof (*PMem));
  CopyMem (&RootBus->PMemAbove4G, PMemAbove4G, sizeof (*PMemAbove4G));

  RootBus->NoExtendedConfigSpace = NoExtendedConfigSpace;

  RootBus->UID = RootBusNumber;
  RootBus->HID = EISA_PNP_ID (0x0A03);

  DEBUG ((
    DEBUG_INFO,
    "%a: populated root bus %d, with room for %d subordinate bus(es)\n",
    __FUNCTION__,
    RootBusNumber,
    MaxSubBusNumber - RootBusNumber
    ));
  return EFI_SUCCESS;
}

/**
  Utility function to return all the root bridge instances in an array.

  @param[out] Count                  The number of root bridge instances.

  @param[in]  Attributes             Initial attributes.

  @param[in]  AllocationAttributes        Allocation attributes.

  @param[in]  DmaAbove4G             DMA above 4GB memory.

  @param[in]  NoExtendedConfigSpace  No Extended Config Space.

  @param[in]  BusMin                 Minimum Bus number, inclusive.

  @param[in]  BusMax                 Maximum Bus number, inclusive.

  @param[in]  Io                     IO aperture.

  @param[in]  Mem                    MMIO aperture.

  @param[in]  MemAbove4G             MMIO aperture above 4G.

  @param[in]  PMem                   Prefetchable MMIO aperture.

  @param[in]  PMemAbove4G            Prefetchable MMIO aperture above 4G.

  @return                            All the root bridge instances in an array.
**/
UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE *
EFIAPI
PeiPciHostBridgeUtilityGetRootBridges (
  OUT UINTN                                       *Count,
  IN  UINT64                                      Attributes,
  IN  UINT64                                      AllocationAttributes,
  IN  BOOLEAN                                     DmaAbove4G,
  IN  BOOLEAN                                     NoExtendedConfigSpace,
  IN  UINTN                                       BusMin,
  IN  UINTN                                       BusMax,
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  *Io,
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  *Mem,
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G,
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  *PMem,
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G
  )
{
  EFI_STATUS                         Status;
  FIRMWARE_CONFIG_ITEM               FwCfgItem;
  UINTN                              FwCfgSize;
  UINT64                             ExtraRootBridges;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE  *Bridges;
  UINTN                              Initialized;
  UINTN                              LastRootBridgeNumber;
  UINTN                              RootBridgeNumber;

  *Count = 0;

  if ((BusMin > BusMax) || (BusMax > PCI_MAX_BUS)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid bus range with BusMin %Lu and BusMax "
      "%Lu\n",
      __FUNCTION__,
      (UINT64)BusMin,
      (UINT64)BusMax
      ));
    return NULL;
  }

  //
  // QEMU provides the number of extra root buses, shortening the exhaustive
  // search below. If there is no hint, the feature is missing.
  //
  Status = QemuFwCfgFindFile ("etc/extra-pci-roots", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status) || (FwCfgSize != sizeof ExtraRootBridges)) {
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
      DEBUG ((
        DEBUG_ERROR,
        "%a: invalid count of extra root buses (%Lu) "
        "reported by QEMU\n",
        __FUNCTION__,
        ExtraRootBridges
        ));
      return NULL;
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: %Lu extra root buses reported by QEMU\n",
      __FUNCTION__,
      ExtraRootBridges
      ));
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
       ++RootBridgeNumber)
  {
    UINTN  Device;

    for (Device = 0; Device <= PCI_MAX_DEVICE; ++Device) {
      if (PciRead16 (
            PCI_LIB_ADDRESS (
              RootBridgeNumber,
              Device,
              0,
              PCI_VENDOR_ID_OFFSET
              )
            ) != MAX_UINT16)
      {
        break;
      }
    }

    if (Device <= PCI_MAX_DEVICE) {
      //
      // Found the next root bus. We can now install the *previous* one,
      // because now we know how big a bus number range *that* one has, for any
      // subordinate buses that might exist behind PCI bridges hanging off it.
      //
      Status = PeiPciHostBridgeUtilityInitRootBridge (
                 Attributes,
                 Attributes,
                 AllocationAttributes,
                 DmaAbove4G,
                 NoExtendedConfigSpace,
                 (UINT8)LastRootBridgeNumber,
                 (UINT8)(RootBridgeNumber - 1),
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
  Status = PeiPciHostBridgeUtilityInitRootBridge (
             Attributes,
             Attributes,
             AllocationAttributes,
             DmaAbove4G,
             NoExtendedConfigSpace,
             (UINT8)LastRootBridgeNumber,
             (UINT8)BusMax,
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
  // while (Initialized > 0) {
  //  --Initialized;
  //  PciHostBridgeUtilityUninitRootBridge (&Bridges[Initialized]);
  // }

  FreePool (Bridges);
  return NULL;
}

/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.
**/
UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE *
EFIAPI
PeiPciHostBridgeGetRootBridges (
  UINTN  *Count
  )
{
  UINT64                                      Attributes;
  UINT64                                      AllocationAttributes;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  Io;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  Mem;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE  MemAbove4G;

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
    MemAbove4G.Base       = PcdGet64 (PcdPciMmio64Base);
    MemAbove4G.Limit      = PcdGet64 (PcdPciMmio64Base) +
                            PcdGet64 (PcdPciMmio64Size) - 1;
  } else {
    CopyMem (&MemAbove4G, &mNonExistAperture, sizeof (mNonExistAperture));
  }

  Io.Base   = PcdGet64 (PcdPciIoBase);
  Io.Limit  = PcdGet64 (PcdPciIoBase) + (PcdGet64 (PcdPciIoSize) - 1);
  Mem.Base  = PcdGet64 (PcdPciMmio32Base);
  Mem.Limit = PcdGet64 (PcdPciMmio32Base) + (PcdGet64 (PcdPciMmio32Size) - 1);

  return PeiPciHostBridgeUtilityGetRootBridges (
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
  Publish the FV that includes the UPL and Publish the HOBs required by UPL.

  @param  FileHandle      Handle of the file being invoked.
  @param  PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
UniversalPayloadInitialization (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_FIRMWARE_VOLUME_HEADER         *UplFv;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE  *RootBridge;
  UINTN                              RootBridgeCount;

  DEBUG ((DEBUG_INFO, "=====================Report UPL FV=======================================\n"));
  UplFv = (EFI_FIRMWARE_VOLUME_HEADER *)PcdGet32 (PcdOvmfPldFvBase);
  ASSERT (UplFv->FvLength == PcdGet32 (PcdOvmfPldFvSize));
  PeiServicesInstallFvInfoPpi (&UplFv->FileSystemGuid, UplFv, (UINT32)UplFv->FvLength, NULL, NULL);

  DEBUG ((DEBUG_INFO, "=====================Build UPL HOBs=======================================\n"));

  SetUplInteger (UPL_KEY_SERIAL_PORT_BAUDRATE, (UINT64)PcdGet32 (PcdSerialBaudRate));
  SetUplBoolean (UPL_KEY_SERIAL_PORT_USE_MMIO, PcdGetBool (PcdSerialUseMmio));
  SetUplInteger (UPL_KEY_SERIAL_PORT_REGISTER_BASE, (UINT64)PcdGet64 (PcdSerialRegisterBase));
  SetUplInteger (UPL_KEY_SERIAL_PORT_REGISTER_STRIDE, (UINT64)PcdGet32 (PcdSerialRegisterStride));

  RootBridge = PeiPciHostBridgeGetRootBridges (&RootBridgeCount);
  SetUplPciRootBridges (RootBridge, RootBridgeCount);
  SetUplBoolean (UPL_KEY_ROOT_BRIDGE_RESOURCE_ASSIGNED, FALSE);

  return EFI_SUCCESS;
}
