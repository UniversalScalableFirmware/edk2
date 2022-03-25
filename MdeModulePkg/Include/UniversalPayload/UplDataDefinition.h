/** @file
  Universal Payload data definition used by SetUplDataLib.h and GetUplDataLib.h

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UPL_DATA_DEFINITION_H__
#define __UPL_DATA_DEFINITION_H__

#include <Uefi/UefiSpec.h>
#include <UniversalPayload/PciRootBridges.h>
#include <UniversalPayload/ExtraData.h>

#define UPL_KEY_ACPI_TABLE_RSDP                 "AcpiTableRsdp"
#define UPL_KEY_MEMORY_SPACE                    "MemorySpace"
#define UPL_KEY_IO_SPACE                        "IoSpace"
#define UPL_KEY_MEMORY_MAP                      "MemoryMap"
#define UPL_KEY_HOB_LIST                        "HobList"
#define UPL_KEY_BASE                            "Base"
#define UPL_KEY_NUMBER_OF_PAGES                 "NumberOfPages"
#define UPL_KEY_TYPE                            "Type"
#define UPL_KEY_ATTRIBUTE                       "Attribute"
#define UPL_KEY_SERIAL_PORT_BAUDRATE            "SerialPortBaudRate"
#define UPL_KEY_SERIAL_PORT_USE_MMIO            "SerialPortUseMmio"
#define UPL_KEY_SERIAL_PORT_REGISTER_BASE       "SerialPortRegisterBase"
#define UPL_KEY_SERIAL_PORT_REGISTER_STRIDE     "SerialPortRegisterStride"
#define UPL_KEY_ROOT_BRIDGE_RESOURCE_ASSIGNED   "RootBridgeResourceAssigned"
#define UPL_KEY_UPL_EXTRA_DATA                  "UplExtradata"
#define UPL_KEY_IDENTIFIER                      "Identifier"
#define UPL_KEY_SIZE                            "Size"
#define UPL_KEY_ROOT_BRIDGE_INFO                "RootBridgeInfo"
#define UPL_KEY_SEGMENT                         "Segment"
#define UPL_KEY_SUPPORTS                        "Supports"
#define UPL_KEY_DMA_ABOVE_4G                    "DmaAbove4G"
#define UPL_KEY_NO_EXTENDED_CONFIG_SPACE        "NoExtendedConfigSpace"
#define UPL_KEY_ALLOCATION_ATTRIBUTES           "AllocationAttributes"
#define UPL_KEY_BUS_BASE                        "BusBase"
#define UPL_KEY_BUS_LIMIT                       "BusLimit"
#define UPL_KEY_BUS_TRANSLATION                 "BusTranslation"
#define UPL_KEY_IO_BASE                         "IoBase"
#define UPL_KEY_IO_LIMIT                        "IoLimit"
#define UPL_KEY_IO_TRANSLATION                  "IoTranslation"
#define UPL_KEY_MEM_BASE                        "MemBase"
#define UPL_KEY_MEM_LIMIT                       "MemLimit"
#define UPL_KEY_MEM_TRANSLATION                 "MemTranslation"
#define UPL_KEY_MEM_ABOVE_4G_BASE               "MemAbove4GBase"
#define UPL_KEY_MEM_ABOVE_4G_LIMIT              "MemAbove4GLimit"
#define UPL_KEY_MEM_ABOVE_4G_TRANSLATION        "MemAbove4GTranslation"
#define UPL_KEY_PMEM_BASE                       "PMemBase"
#define UPL_KEY_PMEM_LIMIT                      "PMemLimit"
#define UPL_KEY_PMEM_TRANSLATION                "PMemTranslation"
#define UPL_KEY_PMEM_ABOVE_4G_BASE              "PMemAbove4GBase"
#define UPL_KEY_PMEM_ABOVE_4G_LIMIT             "PMemAbove4GLimit"
#define UPL_KEY_PMEM_ABOVE_4G_TRANSLATION       "PMemAbove4GTranslation"
#define UPL_KEY_ROOT_BRIDGE_HID                 "HID"
#define UPL_KEY_ROOT_BRIDGE_UID                 "UID"
#define UPL_KEY_RESOURCE                        "Resource"
#define UPL_KEY_OWNER                           "Owner"
#define UPL_KEY_LENGTH                          "Length"
#define UPL_KEY_RESOURCE_ALLOCATION              "ResourceAllocation"
#define UPL_KEY_NAME                            "Name"

typedef struct {
  EFI_GUID                    Owner;
  EFI_RESOURCE_TYPE           ResourceType;
  EFI_RESOURCE_ATTRIBUTE_TYPE ResourceAttribute;
  EFI_PHYSICAL_ADDRESS        PhysicalStart;
  UINT64                      ResourceLength;
} UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR;

typedef struct {
  EFI_GUID                Name;
  EFI_PHYSICAL_ADDRESS    MemoryBaseAddress;
  UINT64                  MemoryLength;
  EFI_MEMORY_TYPE         MemoryType;
} UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION;

#endif
