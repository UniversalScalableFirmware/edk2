/** @file
  Scan the entire PCI bus for root bridges from PCI Root Bridge Info Hob

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <IndustryStandard/Pci.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/PciLib.h>
#include <Guid/PciRootBridgeInfoGuid.h>
#include "PciHostBridge.h"

/**

  Parse/Get PCI Root Bridge Resource Ranges from PCI Root Bridge Info Hob

  @param[in]  PciRootBridgeInfo       Pointer of PCI Root Bridge Info Hob
  @param[in]  EntryIndex              An entry index of PCI Root Bridge Info Hob entries
  @param[in]  Io                      IO aperture.
  @param[in]  Mem                     MMIO aperture.
  @param[in]  MemAbove4G              MMIO aperture above 4G.
  @param[in]  PMem                    Prefetchable MMIO aperture.
  @param[in]  PMemAbove4G             Prefetchable MMIO aperture above 4G.

  @retval     EFI_SUCCESS             Getting resource range successful.
  @retval     EFI_INVALID_PARAMETER   Invalid parameter

**/
STATIC
EFI_STATUS
GetPciRootBridgeResourceRanges (
  IN  PCI_ROOT_BRIDGE_INFO_HOB  *PciRootBridgeInfo,
  IN  UINT8                      EntryIndex,
  IN  PCI_ROOT_BRIDGE_APERTURE  *Io,
  IN  PCI_ROOT_BRIDGE_APERTURE  *Mem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G
  )
{
  UINT8                         Index;
  PCI_ROOT_BRIDGE_APERTURE      *Aperture;

  if (PciRootBridgeInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (EntryIndex > (PciRootBridgeInfo->Count - 1)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < PCI_MAX_BAR; Index++) {
    switch (Index) {
      case 0x0:
      case 0x1:
        Aperture = Io;
        break;
      case 0x2:
        Aperture = Mem;
        break;
      case 0x3:
        Aperture = PMem;
        break;
      case 0x4:
        Aperture = MemAbove4G;
        break;
      case 0x5:
        Aperture = PMemAbove4G;
        break;
      default:
        Aperture = NULL;
        break;
    }

    if (Aperture != NULL) {
      Aperture->Base  = PciRootBridgeInfo->Entry[EntryIndex].Resource[Index].ResBase;
      if (PciRootBridgeInfo->Entry[EntryIndex].Resource[Index].ResLength == 0) {
        Aperture->Base  = MAX_UINT64;
        Aperture->Limit = 0;
      } else {
        Aperture->Limit = Aperture->Base + PciRootBridgeInfo->Entry[EntryIndex].Resource[Index].ResLength - 1;
      }
    }

  }

  return EFI_SUCCESS;
}

/**
  Scan for all root bridges from PciRootBridgeInfoHob

  @param[in]  PciRootBridgeInfo    Pointer of PCI Root Bridge Info Hob
  @param[out] NumberOfRootBridges  Number of root bridges detected

  @retval     Pointer to the allocated PCI_ROOT_BRIDGE structure array.

**/
PCI_ROOT_BRIDGE *
ScanForRootBridgesFromHob (
  IN  PCI_ROOT_BRIDGE_INFO_HOB  *PciRootBridgeInfo,
  OUT UINTN                     *NumberOfRootBridges
)
{
  PCI_ROOT_BRIDGE               *PciRootBridges;
  PCI_ROOT_BRIDGE_APERTURE      Io, Mem, MemAbove4G, PMem, PMemAbove4G;
  UINTN                         Size;
  UINT8                         EntryIndex;
  UINT64                        Attributes;
  UINT8                         PrimaryBus;
  UINT8                         MaxSubBus;
  UINT8                         Bus;
  UINT8                         Device;
  UINT8                         Function;
  UINTN                         Address;
  PCI_TYPE01                    Pci;
  EFI_STATUS                    Status;

  ASSERT (PciRootBridgeInfo != NULL);
  if (PciRootBridgeInfo == NULL) {
    return NULL;
  }

  Size = PciRootBridgeInfo->Count * sizeof (PCI_ROOT_BRIDGE);
  PciRootBridges = (PCI_ROOT_BRIDGE *) AllocatePool (Size);
  ASSERT (PciRootBridges != NULL);
  if (PciRootBridges == NULL) {
    return NULL;
  }
  ZeroMem (PciRootBridges, PciRootBridgeInfo->Count * sizeof (PCI_ROOT_BRIDGE));

  //
  // Create all root bridges with PciRootBridgeInfoHob
  //
  for (EntryIndex = 0; EntryIndex < PciRootBridgeInfo->Count; EntryIndex++) {
    Attributes = 0;

    //
    // Read resource range for each rootbridge
    //
    ZeroMem (&Io,          sizeof (PCI_ROOT_BRIDGE_APERTURE));
    ZeroMem (&Mem,         sizeof (PCI_ROOT_BRIDGE_APERTURE));
    ZeroMem (&MemAbove4G,  sizeof (PCI_ROOT_BRIDGE_APERTURE));
    ZeroMem (&PMem,        sizeof (PCI_ROOT_BRIDGE_APERTURE));
    ZeroMem (&PMemAbove4G, sizeof (PCI_ROOT_BRIDGE_APERTURE));
    Io.Base = Mem.Base = MemAbove4G.Base = PMem.Base = PMemAbove4G.Base = MAX_UINT64;

    Status = GetPciRootBridgeResourceRanges (
      PciRootBridgeInfo, EntryIndex, &Io, &Mem, &MemAbove4G, &PMem, &PMemAbove4G);

    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return NULL;
    }

    //
    // Update Attributes
    //
    PrimaryBus  = PciRootBridgeInfo->Entry[EntryIndex].BusBase;
    MaxSubBus   = PciRootBridgeInfo->Entry[EntryIndex].BusLimit;

    for (Bus = PrimaryBus; Bus <= MaxSubBus; Bus++) {
      for (Device = 0; Device <= PCI_MAX_DEVICE; Device++) {
        for (Function = 0; Function <= PCI_MAX_FUNC; Function++) {
          Address = PCI_LIB_ADDRESS (PrimaryBus, Device, Function, 0);

          //
          // Read the Vendor ID from the PCI Configuration Header
          //
          if (PciRead16 (Address) == MAX_UINT16) {
            if (Function == 0) {
              //
              // If the PCI Configuration Read fails, or a PCI device does not
              // exist, then skip this entire PCI device
              //
              break;
            } else {
              //
              // If PCI function != 0, VendorId == 0xFFFF, we continue to search
              // PCI function.
              //
              continue;
            }
          }

          //
          // Read the entire PCI Configuration Header
          //
          PciReadBuffer (Address, sizeof (Pci), &Pci);

          //
          // Look for devices with the VGA Palette Snoop enabled in the COMMAND
          // register of the PCI Config Header
          //
          if ((Pci.Hdr.Command & EFI_PCI_COMMAND_VGA_PALETTE_SNOOP) != 0) {
            Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
            Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;
          }

          //
          // PCI-PCI Bridge
          //
          if (IS_PCI_BRIDGE (&Pci)) {
            //
            // Look at the PPB Configuration for legacy decoding attributes
            //
            if ((Pci.Bridge.BridgeControl & EFI_PCI_BRIDGE_CONTROL_ISA)
                == EFI_PCI_BRIDGE_CONTROL_ISA) {
              Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO;
              Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO_16;
              Attributes |= EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO;
            }
            if ((Pci.Bridge.BridgeControl & EFI_PCI_BRIDGE_CONTROL_VGA)
                == EFI_PCI_BRIDGE_CONTROL_VGA) {
              Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
              Attributes |= EFI_PCI_ATTRIBUTE_VGA_MEMORY;
              Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO;
              if ((Pci.Bridge.BridgeControl & EFI_PCI_BRIDGE_CONTROL_VGA_16)
                  != 0) {
                Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;
                Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO_16;
              }
            }
          }

          //
          // See if the PCI device is an IDE controller
          //
          if (IS_CLASS2 (&Pci, PCI_CLASS_MASS_STORAGE,
                         PCI_CLASS_MASS_STORAGE_IDE)) {
            if (Pci.Hdr.ClassCode[0] & 0x80) {
              Attributes |= EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO;
              Attributes |= EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO;
            }
            if (Pci.Hdr.ClassCode[0] & 0x01) {
              Attributes |= EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO;
            }
            if (Pci.Hdr.ClassCode[0] & 0x04) {
              Attributes |= EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO;
            }
          }

          //
          // See if the PCI device is a legacy VGA controller or
          // a standard VGA controller
          //
          if (IS_CLASS2 (&Pci, PCI_CLASS_OLD, PCI_CLASS_OLD_VGA) ||
              IS_CLASS2 (&Pci, PCI_CLASS_DISPLAY, PCI_CLASS_DISPLAY_VGA)
             ) {
            Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
            Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;
            Attributes |= EFI_PCI_ATTRIBUTE_VGA_MEMORY;
            Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO;
            Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO_16;
          }

          //
          // See if the PCI Device is a PCI - ISA or PCI - EISA
          // or ISA_POSITIVE_DECODE Bridge device
          //
          if (Pci.Hdr.ClassCode[2] == PCI_CLASS_BRIDGE) {
            if (Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_ISA ||
                Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_EISA ||
                Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_ISA_PDECODE) {
              Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO;
              Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO_16;
              Attributes |= EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO;
            }
          }

          //
          // If this device is not a multi function device, then skip the rest
          // of this PCI device
          //
          if (Function == 0 && !IS_PCI_MULTI_FUNC (&Pci)) {
            break;
          }
        }
      }
    }

    InitRootBridge (
      Attributes, Attributes, 0,
      PrimaryBus, MaxSubBus,
      &Io, &Mem, &MemAbove4G, &PMem, &PMemAbove4G,
      &PciRootBridges[EntryIndex]
    );

    PciRootBridges[EntryIndex].ResourceAssigned = TRUE;
  }

  if (NumberOfRootBridges != NULL) {
    *NumberOfRootBridges = PciRootBridgeInfo->Count;
  }
  return PciRootBridges;
}
