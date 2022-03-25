/** @file
  Get Universal Payload data library

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <UniversalPayload/SerialPortInfo.h>
#include <Library/GetUplDataLib.h>
#include "CborGetWrapper.h"
#include <Library/IoLib.h>

#define RETURN_ON_ERROR(Expression)          \
    do {                                 \
      Status = Expression;               \
      if (Status != RETURN_SUCCESS) {    \
        return Status;                   \
      }                                  \
    } while (FALSE)

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
  )
{
  RETURN_STATUS  Status;

  //
  // Assume the buffer is a well-formed and valid CBOR encoded item, so
  // parse process won't access out of the CBOR buffer range even if not
  // given the size to control it.
  //
  Status = CborDecoderGetRootMap (Buffer, (~(UINTN)0) - (UINTN)Buffer);
  return Status;
}

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
  )
{
  return CborDecoderGetUint64 (String, Result, NULL);
}

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
  )
{
  return CborDecoderGetBoolean (String, Result, NULL);
}

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
  )
{
  return CborDecoderGetBinary (String, Buffer, Size, NULL);
}

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
  )
{
  return CborDecoderGetTextString (String, Buffer, Size, NULL);
}

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
  )
{
  UINTN             Size, LocalIndex;
  UPL_DATA_DECODER  SubMap;
  UINTN             IdentifierSize;
  RETURN_STATUS     Status;

  RETURN_ON_ERROR (CborDecoderGetArrayLengthAndFirstElement (UPL_KEY_UPL_EXTRA_DATA, &Size, &SubMap));
  if (*Count == 0) {
    *Count = Size;
    return RETURN_BUFFER_TOO_SMALL;
  }

  if (Index >= Size) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Skip the first Index element.
  //
  for (LocalIndex = 0; LocalIndex < Index; LocalIndex++) {
    RETURN_ON_ERROR (CborDecoderGetArrayNextMap (&SubMap));
  }

  for (LocalIndex = 0; LocalIndex < *Count && LocalIndex < (Size -Index); LocalIndex++) {
    if (LocalIndex != 0) {
      RETURN_ON_ERROR (CborDecoderGetArrayNextMap (&SubMap));
    }

    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_BASE, &Data[LocalIndex].Base, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_SIZE, &Data[LocalIndex].Size, &SubMap));
    IdentifierSize = 16;
    RETURN_ON_ERROR (CborDecoderGetTextString (UPL_KEY_IDENTIFIER, (UINT8 *)&Data[LocalIndex].Identifier, &IdentifierSize, &SubMap));
  }

  *Count = LocalIndex;

  return EFI_SUCCESS;
}

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
  )
{
  UINTN             Size, LocalIndex;
  UPL_DATA_DECODER  SubMap;
  UINT64            Type;
  RETURN_STATUS     Status;

  RETURN_ON_ERROR (CborDecoderGetArrayLengthAndFirstElement (UPL_KEY_MEMORY_MAP, &Size, &SubMap));
  if (*Count == 0) {
    *Count = Size;
    return RETURN_BUFFER_TOO_SMALL;
  }

  if (Index >= Size) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Skip the first Index element.
  //
  for (LocalIndex = 0; LocalIndex < Index; LocalIndex++) {
    RETURN_ON_ERROR (CborDecoderGetArrayNextMap (&SubMap));
  }

  for (LocalIndex = 0; LocalIndex < *Count && LocalIndex < (Size -Index); LocalIndex++) {
    if (LocalIndex != 0) {
      RETURN_ON_ERROR (CborDecoderGetArrayNextMap (&SubMap));
    }

    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_BASE, &Data[LocalIndex].PhysicalStart, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_NUMBER_OF_PAGES, &Data[LocalIndex].NumberOfPages, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_TYPE, &Type, &SubMap));
    Data[LocalIndex].Type = (UINT32)Type;
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_ATTRIBUTE, &Data[LocalIndex].Attribute, &SubMap));
  }

  *Count = LocalIndex;

  return EFI_SUCCESS;
}

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
  )
{
  UINTN             Size, LocalIndex;
  UPL_DATA_DECODER  SubMap;
  RETURN_STATUS     Status;

  RETURN_ON_ERROR (CborDecoderGetArrayLengthAndFirstElement (UPL_KEY_ROOT_BRIDGE_INFO, &Size, &SubMap));
  if (*Count == 0) {
    *Count = Size;
    return RETURN_BUFFER_TOO_SMALL;
  }

  if (Index >= Size) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Skip the first Index element.
  //
  for (LocalIndex = 0; LocalIndex < Index; LocalIndex++) {
    RETURN_ON_ERROR (CborDecoderGetArrayNextMap (&SubMap));
  }

  for (LocalIndex = 0; LocalIndex < *Count && LocalIndex < (Size -Index); LocalIndex++) {
    if (LocalIndex != 0) {
      RETURN_ON_ERROR (CborDecoderGetArrayNextMap (&SubMap));
    }

    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_SEGMENT, (UINT64 *)&Data[LocalIndex].Segment, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_SUPPORTS, &Data[LocalIndex].Supports, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_ATTRIBUTE, &Data[LocalIndex].Attributes, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetBoolean (UPL_KEY_DMA_ABOVE_4G, &Data[LocalIndex].DmaAbove4G, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetBoolean (UPL_KEY_NO_EXTENDED_CONFIG_SPACE, &Data[LocalIndex].NoExtendedConfigSpace, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_ALLOCATION_ATTRIBUTES, &Data[LocalIndex].AllocationAttributes, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_BUS_BASE, &Data[LocalIndex].Bus.Base, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_BUS_LIMIT, &Data[LocalIndex].Bus.Limit, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_BUS_TRANSLATION, &Data[LocalIndex].Bus.Translation, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_IO_BASE, &Data[LocalIndex].Io.Base, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_IO_LIMIT, &Data[LocalIndex].Io.Limit, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_IO_TRANSLATION, &Data[LocalIndex].Io.Translation, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_MEM_BASE, &Data[LocalIndex].Mem.Base, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_MEM_LIMIT, &Data[LocalIndex].Mem.Limit, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_MEM_TRANSLATION, &Data[LocalIndex].Mem.Translation, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_MEM_ABOVE_4G_BASE, &Data[LocalIndex].MemAbove4G.Base, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_MEM_ABOVE_4G_LIMIT, &Data[LocalIndex].MemAbove4G.Limit, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_MEM_ABOVE_4G_TRANSLATION, &Data[LocalIndex].MemAbove4G.Translation, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_PMEM_BASE, &Data[LocalIndex].PMem.Base, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_PMEM_LIMIT, &Data[LocalIndex].PMem.Limit, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_PMEM_TRANSLATION, &Data[LocalIndex].PMem.Translation, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_PMEM_ABOVE_4G_BASE, &Data[LocalIndex].PMemAbove4G.Base, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_PMEM_ABOVE_4G_LIMIT, &Data[LocalIndex].PMemAbove4G.Limit, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_PMEM_ABOVE_4G_TRANSLATION, &Data[LocalIndex].PMemAbove4G.Translation, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_ROOT_BRIDGE_HID, (UINT64 *)&Data[LocalIndex].HID, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_ROOT_BRIDGE_UID, (UINT64 *)&Data[LocalIndex].UID, &SubMap));
  }

  *Count = LocalIndex;

  return EFI_SUCCESS;
}

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
  )
{
  UINTN             Size, BinarySize, LocalIndex;
  UPL_DATA_DECODER  SubMap;
  RETURN_STATUS     Status;

  RETURN_ON_ERROR (CborDecoderGetArrayLengthAndFirstElement (UPL_KEY_RESOURCE, &Size, &SubMap));
  if (*Count == 0) {
    *Count = Size;
    return RETURN_BUFFER_TOO_SMALL;
  }

  if (Index >= Size) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Skip the first Index element.
  //
  for (LocalIndex = 0; LocalIndex < Index; LocalIndex++) {
    RETURN_ON_ERROR (CborDecoderGetArrayNextMap (&SubMap));
  }

  for (LocalIndex = 0; LocalIndex < *Count && LocalIndex < (Size -Index); LocalIndex++) {
    if (LocalIndex != 0) {
      RETURN_ON_ERROR (CborDecoderGetArrayNextMap (&SubMap));
    }

    BinarySize = sizeof (Data[LocalIndex].Owner);
    RETURN_ON_ERROR (CborDecoderGetBinary (UPL_KEY_OWNER, &Data[LocalIndex].Owner, &BinarySize, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_TYPE, (UINT64 *)&Data[LocalIndex].ResourceType, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_ATTRIBUTE, (UINT64 *)&Data[LocalIndex].ResourceAttribute, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_BASE, (UINT64 *)&Data[LocalIndex].PhysicalStart, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_LENGTH, (UINT64 *)&Data[LocalIndex].ResourceLength, &SubMap));
  }

  *Count = LocalIndex;

  return EFI_SUCCESS;
}

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
  )
{
  UINTN             Size, BinarySize, LocalIndex;
  UPL_DATA_DECODER  SubMap;
  RETURN_STATUS     Status;

  RETURN_ON_ERROR (CborDecoderGetArrayLengthAndFirstElement (UPL_KEY_RESOURCE_ALLOCATION, &Size, &SubMap));
  if (*Count == 0) {
    *Count = Size;
    return RETURN_BUFFER_TOO_SMALL;
  }

  if (Index >= Size) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Skip the first Index element.
  //
  for (LocalIndex = 0; LocalIndex < Index; LocalIndex++) {
    RETURN_ON_ERROR (CborDecoderGetArrayNextMap (&SubMap));
  }

  for (LocalIndex = 0; LocalIndex < *Count && LocalIndex < (Size -Index); LocalIndex++) {
    if (LocalIndex != 0) {
      RETURN_ON_ERROR (CborDecoderGetArrayNextMap (&SubMap));
    }

    BinarySize = sizeof (Data[LocalIndex].Name);
    RETURN_ON_ERROR (CborDecoderGetBinary (UPL_KEY_NAME, &Data[LocalIndex].Name, &BinarySize, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_BASE, (UINT64 *)&Data[LocalIndex].MemoryBaseAddress, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_LENGTH, (UINT64 *)&Data[LocalIndex].MemoryLength, &SubMap));
    RETURN_ON_ERROR (CborDecoderGetUint64 (UPL_KEY_TYPE, (UINT64 *)&Data[LocalIndex].MemoryType, &SubMap));
  }

  *Count = LocalIndex;

  return EFI_SUCCESS;
}
