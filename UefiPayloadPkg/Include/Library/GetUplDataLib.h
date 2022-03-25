/** @file
  Get Universal Payload data library

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __GET_UPL_DATA_LIB_H__
#define __GET_UPL_DATA_LIB_H__

#include <Base.h>
#include <UniversalPayload/UplDataDefinition.h>

/**
  Initialize the UPL buffer.

  @param[in] Buffer  Data buffer to be parsed.

  @retval RETURN_SUCCESS     Initialized successfully.
  @retval RETURN_ABORTED     Not a valid UPL buffer.
**/
RETURN_STATUS
EFIAPI
InitUplFromBuffer (
  IN VOID  *Buffer
  );

/**
  Get an Integer value with a specified key string from Universal Payload Data.

  @param[in]  String  The key string for which the value has to be found.
  @param[out] Result  Variable to store the found value.

  @retval RETURN_SUCCESS            The information is obtained successfully.
  @retval RETURN_NOT_FOUND          Key string is not found.
  @retval RETURN_INVALID_PARAMETER  Element with key string is not of type UINT64.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
GetUplInteger (
  IN  CHAR8   *String,
  OUT UINT64  *Result
  );

/**
  Get a BOOLEAN value with a specified key string from Universal Payload Data.

  @param[in]  String  The key string for which the value has to be found.
  @param[out] Result  Variable to store the found value.

  @retval RETURN_SUCCESS            The information is obtained successfully.
  @retval RETURN_NOT_FOUND          Key string is not found.
  @retval RETURN_INVALID_PARAMETER  Element with key string is not of type BOOLEAN.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
GetUplBoolean (
  IN  CHAR8    *String,
  OUT BOOLEAN  *Result
  );

/**
  Get an ASCII string with a specified key string from Universal Payload Data.

  @param[in]     String  The key string for which the value has to be found.
  @param[in out] Buffer  Buffer to store the found value.
  @param[in out] Size    Size of the buffer.

  @retval RETURN_SUCCESS            The information is obtained successfully.
  @retval RETURN_NOT_FOUND          Key string is not found.
  @retval RETURN_INVALID_PARAMETER  Element with key string is not of type String.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
GetUplAsciiString (
  IN     CHAR8  *String,
  IN OUT UINT8  *Buffer,
  IN OUT UINTN  *Size
  );

/**
  Get raw binary data with a specified key string from Universal Payload Data.

  @param[in]     String  The key string for which the value has to be found.
  @param[in out] Buffer  Buffer to store the found value.
  @param[in out] Size    Size of the buffer.

  @retval RETURN_SUCCESS            The information is obtained successfully.
  @retval RETURN_NOT_FOUND          Key string is not found.
  @retval RETURN_INVALID_PARAMETER  Element with key string is not of type byte String.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
GetUplBinary (
  IN     CHAR8  *String,
  IN OUT VOID   *Buffer,
  IN OUT UINTN  *Size
  );

/**
  Get Universal Payload extra data information from Universal Payload Data.

  @param[in out] Data   Universal payload extra data information to be found.
  @param[in out] Count  Count of extra data entries.
  @param[in]     Index  Start Index to store the data.

  @retval RETURN_SUCCESS            The information is obtained successfully.
  @retval RETURN_NOT_FOUND          Key string is not found.
  @retval RETURN_BUFFER_TOO_SMALL   Count is zero on output.
  @retval RETURN_INVALID_PARAMETER  Element with key string is not of expected type.
  @retval RETURN_UNSUPPORTED        Index is larger than or equal to total element count.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
GetUplExtraData (
  IN OUT UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY  *Data,
  IN OUT UINTN                               *Count,
  IN     UINTN                               Index
  );

/**
  Get Memory map information from Universal Payload Data.

  @param[in out] Data   Universal payload memory map information to be found.
  @param[in out] Count  Count of memory map descriptors.
  @param[in]     Index  Start Index to store the data.

  @retval RETURN_SUCCESS            The information is obtained successfully.
  @retval RETURN_NOT_FOUND          Key string is not found.
  @retval RETURN_BUFFER_TOO_SMALL   Count is zero on output.
  @retval RETURN_INVALID_PARAMETER  Element with key string is not of expected type.
  @retval RETURN_UNSUPPORTED        Index is larger than or equal to total element count.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
GetUplMemoryMap (
  IN OUT EFI_MEMORY_DESCRIPTOR  *Data,
  IN OUT UINTN                  *Count,
  IN     UINTN                  Index
  );

/**
  Get Universal Payload Pci root bridge information from Universal Payload Data.

  @param[in out] Data   Universal payload Pci root bridge information to be found.
  @param[in out] Count  Count of root bridges.
  @param[in]     Index  Start Index to store the data.

  @retval RETURN_SUCCESS            The information is obtained successfully.
  @retval RETURN_NOT_FOUND          Key string is not found.
  @retval RETURN_BUFFER_TOO_SMALL   Count is zero on output.
  @retval RETURN_INVALID_PARAMETER  Element with key string is not of expected type.
  @retval RETURN_UNSUPPORTED        Index is larger than or equal to total element count.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
GetUplPciRootBridges (
  IN OUT UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE  *Data,
  IN OUT UINTN                              *Count,
  IN     UINTN                              Index
  );

/**
  Get Universal Payload resource data information from Universal Payload Data.

  @param[in out] Data   Universal payload resource data to be found.
  @param[in out] Count  Count of resource data.
  @param[in]     Index  Start Index to store the data.

  @retval RETURN_SUCCESS            The information is obtained successfully.
  @retval RETURN_NOT_FOUND          Key string is not found.
  @retval RETURN_BUFFER_TOO_SMALL   Count is zero on output.
  @retval RETURN_INVALID_PARAMETER  Element with key string is not of expected type.
  @retval RETURN_UNSUPPORTED        Index is larger than or equal to total element count.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
GetUplResourceData (
  IN OUT UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR  *Data,
  IN OUT UINTN                                  *Count,
  IN     UINTN                                  Index
  );

/**
  Get Universal Payload memory allocation information from Universal Payload Data.

  @param[in out] Data   Universal payload memory allocation information to be found.
  @param[in out] Count  Count of memory allocation data.
  @param[in]     Index  Start Index to store the data.

  @retval RETURN_SUCCESS            The information is obtained successfully.
  @retval RETURN_NOT_FOUND          Key string is not found.
  @retval RETURN_BUFFER_TOO_SMALL   Count is zero on output.
  @retval RETURN_INVALID_PARAMETER  Element with key string is not of expected type.
  @retval RETURN_UNSUPPORTED        Index is larger than or equal to total element count.
  @retval RETURN_ABORTED            Unexpected failure.
**/
RETURN_STATUS
EFIAPI
GetUplMemoryAllocationData (
  IN OUT UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION  *Data,
  IN OUT UINTN                                *Count,
  IN     UINTN                                Index
  );

#endif
