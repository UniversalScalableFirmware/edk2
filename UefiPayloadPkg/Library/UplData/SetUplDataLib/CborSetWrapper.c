/** @file
  Set Universal Payload data library

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "..\tinycbor\src\cbor.h"
#include <Base.h>
#include <Library/DebugLib.h>
#include <PiPei.h>
#include <Library/MemoryAllocationLib.h>

#define RETURN_ON_ERROR_CBOR_ENCODE(Expression)    \
    do {                                       \
      switch (Expression)                      \
      {                                        \
      case CborNoError:                        \
        break;                                 \
      case CborErrorOutOfMemory:               \
        return RETURN_OUT_OF_RESOURCES;        \
      default:                                 \
        return RETURN_ABORTED;                \
      }                                        \
    } while (FALSE)

CborEncoder  mSubMapEncoder;
CborEncoder  mArrayEncoder;

RETURN_STATUS
CborEncoderInit (
  OUT VOID  **RootEncoderPointer,
  OUT VOID  **RootMapEncoderPointer,
  OUT VOID  **Buffer,
  IN UINTN  BufferSize
  )
{
  *RootEncoderPointer    = AllocatePages (EFI_SIZE_TO_PAGES (BufferSize+ 2*sizeof (CborEncoder)));
  *RootMapEncoderPointer = (UINT8 *)*RootEncoderPointer + sizeof (CborEncoder);
  *Buffer                = (UINT8 *)*RootEncoderPointer + 2*sizeof (CborEncoder);

  cbor_encoder_init (*RootEncoderPointer, *Buffer, BufferSize, 0);
  RETURN_ON_ERROR_CBOR_ENCODE (cbor_encoder_create_map (*RootEncoderPointer, *RootMapEncoderPointer, CborIndefiniteLength));
  return RETURN_SUCCESS;
}

RETURN_STATUS
CborEncoderCreateSubMap (
  VOID  *ParentEncoder,
  VOID  **Child
  )
{
  RETURN_ON_ERROR_CBOR_ENCODE (cbor_encoder_create_map (ParentEncoder, &mSubMapEncoder, CborIndefiniteLength));
  *Child = &mSubMapEncoder;
  return RETURN_SUCCESS;
}

RETURN_STATUS
CborEncoderCreateArray (
  VOID   *ParentEncoder,
  VOID   **ArrayEncoder,
  UINTN  Length
  )
{
  RETURN_ON_ERROR_CBOR_ENCODE (cbor_encoder_create_array (ParentEncoder, &mArrayEncoder, Length));
  *ArrayEncoder = &mArrayEncoder;
  return RETURN_SUCCESS;
}

RETURN_STATUS
CborEncodeTextString (
  VOID         *MapEncoder,
  const CHAR8  *TextString
  )
{
  RETURN_ON_ERROR_CBOR_ENCODE (cbor_encode_text_stringz (MapEncoder, TextString));
  return RETURN_SUCCESS;
}

RETURN_STATUS
CborEncodeTextStringS (
  VOID         *MapEncoder,
  CHAR8        *TextString,
  UINTN        Size
  )
{
  RETURN_ON_ERROR_CBOR_ENCODE (cbor_encode_text_string (MapEncoder, TextString, Size));
  return RETURN_SUCCESS;
}

RETURN_STATUS
CborEncodeByteString (
  VOID         *MapEncoder,
  const UINT8  *ByteString,
  UINTN        Size
  )
{
  RETURN_ON_ERROR_CBOR_ENCODE (cbor_encode_byte_string (MapEncoder, ByteString, Size));

  return RETURN_SUCCESS;
}

RETURN_STATUS
CborEncodeUint64 (
  VOID    *MapEncoder,
  UINT64  Value
  )
{
  RETURN_ON_ERROR_CBOR_ENCODE (cbor_encode_uint (MapEncoder, Value));

  return RETURN_SUCCESS;
}

RETURN_STATUS
CborEncodeUint8 (
  VOID   *MapEncoder,
  UINT8  Value
  )
{
  RETURN_ON_ERROR_CBOR_ENCODE (cbor_encode_simple_value (MapEncoder, Value));

  return RETURN_SUCCESS;
}

RETURN_STATUS
CborEncodeBoolean (
  VOID     *MapEncoder,
  BOOLEAN  Value
  )
{
  RETURN_ON_ERROR_CBOR_ENCODE (cbor_encode_boolean (MapEncoder, Value));

  return RETURN_SUCCESS;
}

RETURN_STATUS
CborEncoderCloseContainer (
  VOID  *ParentEncoder,
  VOID  *ContainerEncoder
  )
{
  RETURN_ON_ERROR_CBOR_ENCODE (cbor_encoder_close_container (ParentEncoder, ContainerEncoder));

  return RETURN_SUCCESS;
}

UINTN
CborGetBuffer (
  VOID  *ParentEncoder,
  VOID  *Buffer
  )
{
  UINTN  UsedSize;

  UsedSize = cbor_encoder_get_buffer_size (ParentEncoder, (const uint8_t *)Buffer);
  DEBUG ((EFI_D_ERROR, "used_size: 0x%x \n", UsedSize));

  return UsedSize;
}
