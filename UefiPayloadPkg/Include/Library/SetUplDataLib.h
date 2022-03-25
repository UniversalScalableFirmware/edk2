/** @file
  Set Universal Payload data library

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SET_UPL_DATA_LIB_H__
#define __SET_UPL_DATA_LIB_H__

#include <Base.h>
#include <UniversalPayload/UplDataDefinition.h>

/**
  Stores an Integer value with a specified key into Universal Payload Data.

  @param[in] Key    The key string of the value to store.
  @param[in] Value  The value to store.

  @retval RETURN_SUCCESS            The information is stored successfully.
  @retval RETURN_INVALID_PARAMETER  No enough resource to save the information.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
SetUplInteger (
  IN CHAR8   *Key,
  IN UINT64  Value
  );

/**
  Stores raw binary data with a specified key into Universal Payload Data.

  @param[in] Key    The key string of the value to store.
  @param[in] Value  The value to store.

  @retval RETURN_SUCCESS            The information is stored successfully.
  @retval RETURN_INVALID_PARAMETER  No enough resource to save the information.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
SetUplBinary (
  IN CHAR8  *Key,
  IN VOID   *Value,
  IN UINTN  Size
  );

/**
  Stores an Ascii string with a specified key into Universal Payload Data.

  @param[in] Key    The key string of the value to store.
  @param[in] Value  The value to store.

  @retval RETURN_SUCCESS            The information is stored successfully.
  @retval RETURN_INVALID_PARAMETER  No enough resource to save the information.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
SetUplAsciiString (
  IN CHAR8  *Key,
  IN UINT8  *Value,
  IN UINTN  Size
  );

/**
  Stores a Boolean value with a specified key into Universal Payload Data.

  @param[in] Key    The key string of the value to store.
  @param[in] Value  The value to store.

  @retval RETURN_SUCCESS            The information is stored successfully.
  @retval RETURN_INVALID_PARAMETER  No enough resource to save the information.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
SetUplBoolean (
  IN CHAR8    *Key,
  IN BOOLEAN  Value
  );

/**
  Lock Universal Payload Data and return buffer.

  @param[out] Buff  Pointer to the buffer containing data.
  @param[out] Size  Size of used buffer.

  @retval RETURN_SUCCESS            Retrieved the buffer successfully.
  @retval RETURN_INVALID_PARAMETER  No enough resource to save the information.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
LockUplAndGetBuffer (
  OUT VOID   **Buff,
  OUT UINTN  *Size
  );

/**
  Stores Universal payload extra data information into Universal Payload Data.

  @param[in] Data    Universal payload extra data information to store.
  @param[in] Count   Count of extra data entries.

  @retval RETURN_SUCCESS            The information is stored successfully.
  @retval RETURN_INVALID_PARAMETER  No enough resource to save the information.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
SetUplExtraData (
  IN UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY       *Data,
  IN UINTN                                    Count
  );

/**
  Stores Universal payload memory map information into Universal Payload Data.

  @param[in] Data    Universal payload memory map information to store.
  @param[in] Count   Count of memory descriptors.

  @retval RETURN_SUCCESS            The information is stored successfully.
  @retval RETURN_INVALID_PARAMETER  No enough resource to save the information.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
SetUplMemoryMap (
  IN EFI_MEMORY_DESCRIPTOR  *Data,
  IN UINTN                  Count
  );

/**
  Stores Universal payload Pci root bridge information into Universal Payload Data.

  @param[in] Data    Universal payload Pci root bridge information to store.
  @param[in] Count   Count of root bridges.

  @retval RETURN_SUCCESS            The information is stored successfully.
  @retval RETURN_INVALID_PARAMETER  No enough resource to save the information.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
SetUplPciRootBridges (
  IN UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE  *Data,
  IN UINTN                              Count
  );

/**
  Stores resource descriptor information into Universal Payload Data.

  @param[in] Data    Universal payload resource descriptor information to store.
  @param[in] Count   Count of resource data.

  @retval RETURN_SUCCESS            The information is stored successfully.
  @retval RETURN_INVALID_PARAMETER  No enough resource to save the information.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
SetUplResourceData (
  IN UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR    *Data,
  IN UINTN                                    Count
  );

/**
  Stores memory allocation information into Universal Payload Data.

  @param[in] Data    Universal payload memory allocation information to store.
  @param[in] Count   Count of memory allocation data.

  @retval RETURN_SUCCESS            The information is stored successfully.
  @retval RETURN_INVALID_PARAMETER  No enough resource to save the information.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
SetUplMemoryAllocationData (
  IN UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION      *Data,
  IN UINTN                                    Count
  );
#endif
