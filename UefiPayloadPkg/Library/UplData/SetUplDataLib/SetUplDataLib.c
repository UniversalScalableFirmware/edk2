/** @file
  Set Universal Payload data library

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <CborRootmapHobGuid.h>
#include "CborSetWrapper.h"
#include <Library/SetUplDataLib.h>

#define RETURN_ON_ERROR(Expression)          \
    do {                                 \
      Status = Expression;               \
      if (Status != RETURN_SUCCESS) {    \
        return Status;                   \
      }                                  \
    } while (FALSE)

//
// Assume 64K bytes for Universal payload interface is enough.
//
#define UNIVERSAL_PAYLOAD_INTERFACE_SIZE 64*1024

VOID  *gRootEncoderPointer, *gRootMapEncoderPointer;
VOID  *gBuffer;

RETURN_STATUS
EFIAPI
SetUplDataLibConstructor (
  VOID
  )
{
  VOID               *Hob;
  UINTN              Size;
  CBOR_ROOTMAP_INFO  *Data;
  RETURN_STATUS      Status;

  Size = UNIVERSAL_PAYLOAD_INTERFACE_SIZE;
  Hob = (VOID *)GetFirstGuidHob (&CborRootmapHobGuid);
  if (Hob == NULL) {
    RETURN_ON_ERROR (CborEncoderInit (&gRootEncoderPointer, &gRootMapEncoderPointer, &gBuffer, Size));
    Data                 = (CBOR_ROOTMAP_INFO *)BuildGuidHob (&CborRootmapHobGuid, sizeof (CBOR_ROOTMAP_INFO));
    Data->RootEncoder    = gRootEncoderPointer;
    Data->RootMapEncoder = gRootMapEncoderPointer;
    Data->Buffer         = gBuffer;
  } else {
    Data                  = GET_GUID_HOB_DATA (Hob);
    gRootEncoderPointer   = Data->RootEncoder;
    gRootMapEncoderPointer = Data->RootMapEncoder;
    gBuffer               = Data->Buffer;
  }

  return RETURN_SUCCESS;
}

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
  )
{
  RETURN_STATUS  Status;

  RETURN_ON_ERROR (CborEncodeTextString (gRootMapEncoderPointer, Key));
  RETURN_ON_ERROR (CborEncodeUint64 (gRootMapEncoderPointer, Value));
  return RETURN_SUCCESS;
}

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
  )
{
  RETURN_STATUS  Status;

  RETURN_ON_ERROR (CborEncodeTextString (gRootMapEncoderPointer, Key));
  RETURN_ON_ERROR (CborEncodeByteString (gRootMapEncoderPointer, Value, Size));
  return RETURN_SUCCESS;
}

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
  )
{
  RETURN_STATUS  Status;

  RETURN_ON_ERROR (CborEncodeTextString (gRootMapEncoderPointer, Key));
  RETURN_ON_ERROR (CborEncodeTextStringS (gRootMapEncoderPointer, (CHAR8 *)Value, Size));
  return RETURN_SUCCESS;
}

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
  )
{
  RETURN_STATUS  Status;

  RETURN_ON_ERROR (CborEncodeTextString (gRootMapEncoderPointer, Key));
  RETURN_ON_ERROR (CborEncodeBoolean (gRootMapEncoderPointer, Value));
  return RETURN_SUCCESS;
}

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
  )
{
  RETURN_STATUS  Status;

  RETURN_ON_ERROR (CborEncoderCloseContainer (gRootEncoderPointer, gRootMapEncoderPointer));
  *Size = CborGetBuffer (gRootEncoderPointer, gBuffer);
  *Buff = gBuffer;
  return RETURN_SUCCESS;
}

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
  IN UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY  *Data,
  IN UINTN                               Count
  )
{
  VOID           *ArrayEncoder;
  VOID           *SubMapEncoder;
  UINTN          Index;
  RETURN_STATUS  Status;

  RETURN_ON_ERROR (CborEncodeTextString (gRootMapEncoderPointer, UPL_KEY_UPL_EXTRA_DATA));
  RETURN_ON_ERROR (CborEncoderCreateArray (gRootMapEncoderPointer, &ArrayEncoder, Count));
  for (Index = 0; Index < Count; Index++) {
    RETURN_ON_ERROR (CborEncoderCreateSubMap (ArrayEncoder, &SubMapEncoder));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_IDENTIFIER));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, Data[Index].Identifier));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_BASE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Base));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_SIZE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Size));
    RETURN_ON_ERROR (CborEncoderCloseContainer (ArrayEncoder, SubMapEncoder));
  }

  RETURN_ON_ERROR (CborEncoderCloseContainer (gRootMapEncoderPointer, ArrayEncoder));
  return EFI_SUCCESS;
}

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
  )
{
  VOID           *ArrayEncoder;
  VOID           *SubMapEncoder;
  UINTN          Index;
  RETURN_STATUS  Status;

  RETURN_ON_ERROR (CborEncodeTextString (gRootMapEncoderPointer, UPL_KEY_MEMORY_MAP));
  RETURN_ON_ERROR (CborEncoderCreateArray (gRootMapEncoderPointer, &ArrayEncoder, Count));
  for (Index = 0; Index < Count; Index++) {
    RETURN_ON_ERROR (CborEncoderCreateSubMap (ArrayEncoder, &SubMapEncoder));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_BASE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PhysicalStart));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_NUMBER_OF_PAGES));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].NumberOfPages));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_TYPE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Type));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_ATTRIBUTE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Attribute));
    RETURN_ON_ERROR (CborEncoderCloseContainer (ArrayEncoder, SubMapEncoder));
  }

  RETURN_ON_ERROR (CborEncoderCloseContainer (gRootMapEncoderPointer, ArrayEncoder));
  return EFI_SUCCESS;
}

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
  )
{
  VOID           *ArrayEncoder;
  VOID           *SubMapEncoder;
  UINTN          Index;
  RETURN_STATUS  Status;

  RETURN_ON_ERROR (CborEncodeTextString (gRootMapEncoderPointer, UPL_KEY_ROOT_BRIDGE_INFO));
  RETURN_ON_ERROR (CborEncoderCreateArray (gRootMapEncoderPointer, &ArrayEncoder, Count));

  for (Index = 0; Index < Count; Index++) {
    RETURN_ON_ERROR (CborEncoderCreateSubMap (ArrayEncoder, &SubMapEncoder));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_SEGMENT));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Segment));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_SUPPORTS));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Supports));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_ATTRIBUTE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Attributes));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_DMA_ABOVE_4G));
    RETURN_ON_ERROR (CborEncodeBoolean (SubMapEncoder, Data[Index].DmaAbove4G));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_NO_EXTENDED_CONFIG_SPACE));
    RETURN_ON_ERROR (CborEncodeBoolean (SubMapEncoder, Data[Index].NoExtendedConfigSpace));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_ALLOCATION_ATTRIBUTES));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].AllocationAttributes));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_BUS_BASE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Bus.Base));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_BUS_LIMIT));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Bus.Limit));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_BUS_TRANSLATION));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Bus.Translation));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_IO_BASE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Io.Base));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_IO_LIMIT));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Io.Limit));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_IO_TRANSLATION));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Io.Translation));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_MEM_BASE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Mem.Base));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_MEM_LIMIT));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Mem.Limit));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_MEM_TRANSLATION));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].Mem.Translation));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_MEM_ABOVE_4G_BASE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemAbove4G.Base));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_MEM_ABOVE_4G_LIMIT));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemAbove4G.Limit));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_MEM_ABOVE_4G_TRANSLATION));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemAbove4G.Translation));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_PMEM_BASE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMem.Base));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_PMEM_LIMIT));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMem.Limit));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_PMEM_TRANSLATION));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMem.Translation));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_PMEM_ABOVE_4G_BASE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMemAbove4G.Base));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_PMEM_ABOVE_4G_LIMIT));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMemAbove4G.Limit));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_PMEM_ABOVE_4G_TRANSLATION));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PMemAbove4G.Translation));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_ROOT_BRIDGE_HID));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].HID));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_ROOT_BRIDGE_UID));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].UID));
    RETURN_ON_ERROR (CborEncoderCloseContainer (ArrayEncoder, SubMapEncoder));
  }

  RETURN_ON_ERROR (CborEncoderCloseContainer (gRootMapEncoderPointer, ArrayEncoder));
  return EFI_SUCCESS;
}

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
  IN UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR  *Data,
  IN UINTN                                  Count
  )
{
  VOID           *ArrayEncoder;
  VOID           *SubMapEncoder;
  UINTN          Index;
  RETURN_STATUS  Status;

  DEBUG ((EFI_D_ERROR, "KBT Data->Owner: %g \n", &Data->Owner));
  DEBUG ((EFI_D_ERROR, "KBT sizeof(Data[Index].Owner: %x \n", sizeof (Data[0].Owner)));
  RETURN_ON_ERROR (CborEncodeTextString (gRootMapEncoderPointer, UPL_KEY_RESOURCE));
  RETURN_ON_ERROR (CborEncoderCreateArray (gRootMapEncoderPointer, &ArrayEncoder, Count));
  for (Index = 0; Index < Count; Index++) {
    RETURN_ON_ERROR (CborEncoderCreateSubMap (ArrayEncoder, &SubMapEncoder));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_OWNER));
    RETURN_ON_ERROR (CborEncodeByteString (SubMapEncoder, (UINT8 *)&Data[Index].Owner, sizeof (Data[Index].Owner)));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_TYPE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].ResourceType));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_ATTRIBUTE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].ResourceAttribute));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_BASE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].PhysicalStart));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_LENGTH));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].ResourceLength));
    RETURN_ON_ERROR (CborEncoderCloseContainer (ArrayEncoder, SubMapEncoder));
  }

  RETURN_ON_ERROR (CborEncoderCloseContainer (gRootMapEncoderPointer, ArrayEncoder));
  return EFI_SUCCESS;
}

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
  IN UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION  *Data,
  IN UINTN                                Count
  )
{
  VOID           *ArrayEncoder;
  VOID           *SubMapEncoder;
  UINTN          Index;
  RETURN_STATUS  Status;

  RETURN_ON_ERROR (CborEncodeTextString (gRootMapEncoderPointer, UPL_KEY_RESOURCE_ALLOCATION));
  RETURN_ON_ERROR (CborEncoderCreateArray (gRootMapEncoderPointer, &ArrayEncoder, Count));
  for (Index = 0; Index < Count; Index++) {
    RETURN_ON_ERROR (CborEncoderCreateSubMap (ArrayEncoder, &SubMapEncoder));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_NAME));
    RETURN_ON_ERROR (CborEncodeByteString (SubMapEncoder, (UINT8 *)&Data[Index].Name, sizeof (Data[Index].Name)));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_BASE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemoryBaseAddress));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_LENGTH));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemoryLength));
    RETURN_ON_ERROR (CborEncodeTextString (SubMapEncoder, UPL_KEY_TYPE));
    RETURN_ON_ERROR (CborEncodeUint64 (SubMapEncoder, Data[Index].MemoryType));
    RETURN_ON_ERROR (CborEncoderCloseContainer (ArrayEncoder, SubMapEncoder));
  }

  RETURN_ON_ERROR (CborEncoderCloseContainer (gRootMapEncoderPointer, ArrayEncoder));
  return EFI_SUCCESS;
}
