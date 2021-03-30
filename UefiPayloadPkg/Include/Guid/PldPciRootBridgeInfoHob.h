/** @file
  This file defines the hob structure for the PCI Root Bridge Info.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLD_PCI_ROOT_BRIDGE_INFO_HOB_H__
#define __PLD_PCI_ROOT_BRIDGE_INFO_HOB_H__

#include <IndustryStandard/Pci.h>
#include <Library/PciHostBridgeLib.h>

#pragma pack(1)

///
/// Payload PCI Root Bridge Information HOB
///
typedef struct {
  UINT32                   Segment;               ///< Segment number.
  UINT64                   Supports;              ///< Supported attributes.
                                                  ///< Refer to EFI_PCI_ATTRIBUTE_xxx used by GetAttributes()
                                                  ///< and SetAttributes() in EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  UINT64                   Attributes;            ///< Initial attributes.
                                                  ///< Refer to EFI_PCI_ATTRIBUTE_xxx used by GetAttributes()
                                                  ///< and SetAttributes() in EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  BOOLEAN                  DmaAbove4G;            ///< DMA above 4GB memory.
                                                  ///< Set to TRUE when root bridge supports DMA above 4GB memory.
  BOOLEAN                  NoExtendedConfigSpace; ///< When FALSE, the root bridge supports
                                                  ///< Extended (4096-byte) Configuration Space.
                                                  ///< When TRUE, the root bridge supports
                                                  ///< 256-byte Configuration Space only.
  BOOLEAN                  ResourceAssigned;      ///< Resource assignment status of the root bridge.
                                                  ///< Set to TRUE if Bus/IO/MMIO resources for root bridge have been assigned.
  UINT64                   AllocationAttributes;  ///< Allocation attributes.
                                                  ///< Refer to EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM and
                                                  ///< EFI_PCI_HOST_BRIDGE_MEM64_DECODE used by GetAllocAttributes()
                                                  ///< in EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.
  PCI_ROOT_BRIDGE_APERTURE Bus;                   ///< Bus aperture which can be used by the root bridge.
  PCI_ROOT_BRIDGE_APERTURE Io;                    ///< IO aperture which can be used by the root bridge.
  PCI_ROOT_BRIDGE_APERTURE Mem;                   ///< MMIO aperture below 4GB which can be used by the root bridge.
  PCI_ROOT_BRIDGE_APERTURE MemAbove4G;            ///< MMIO aperture above 4GB which can be used by the root bridge.
  PCI_ROOT_BRIDGE_APERTURE PMem;                  ///< Prefetchable MMIO aperture below 4GB which can be used by the root bridge.
  PCI_ROOT_BRIDGE_APERTURE PMemAbove4G;           ///< Prefetchable MMIO aperture above 4GB which can be used by the root bridge.
  UINT32                   HID;
  UINT32                   UID;
} PLD_PCI_ROOT_BRIDGE;

typedef struct {
  UINT8                     Revision;
  UINT8                     Count;
  PLD_PCI_ROOT_BRIDGE       RootBridge[0];
} PLD_PCI_ROOT_BRIDGE_INFO_HOB;

#pragma pack()

#endif // __PLD_PCI_ROOT_BRIDGE_INFO_HOB_H__
