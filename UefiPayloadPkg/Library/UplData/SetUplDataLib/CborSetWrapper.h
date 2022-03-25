/** @file
  Set Universal Payload data library

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CBOR_SET_WRAPPER_H__
#define __CBOR_SET_WRAPPER_H__

RETURN_STATUS
CborEncoderInit (
  OUT VOID  **RootEncoderPointer,
  OUT VOID  **RootMapEncoderPointer,
  OUT VOID  **Buffer,
  IN UINTN  size
  );

RETURN_STATUS
CborEncoderCreateSubMap (
  VOID   *ParentEncoder,
  VOID   *Child
  );

RETURN_STATUS
CborEncoderCreateArray (
  VOID   *ParentEncoder,
  VOID   *ArrayEncoder,
  UINTN  Length
  );

RETURN_STATUS
CborEncodeTextString (
  VOID         *MapEncoder,
  const CHAR8  *TextString
  );

RETURN_STATUS
CborEncodeTextStringS (
  VOID         *MapEncoder,
  CHAR8        *TextString,
  UINTN        Size
  );

RETURN_STATUS
CborEncodeByteString (
  VOID         *MapEncoder,
  const UINT8  *ByteString,
  UINTN        size
  );

RETURN_STATUS
CborEncodeUint64 (
  VOID    *MapEncoder,
  UINT64  Value
  );

RETURN_STATUS
CborEncodeUint8 (
  VOID   *MapEncoder,
  UINT8  Value
  );

RETURN_STATUS
CborEncodeBoolean (
  VOID     *MapEncoder,
  BOOLEAN  Value
  );

RETURN_STATUS
CborEncoderCloseContainer (
  VOID  *ParentEncoder,
  VOID  *ContainerEncoder
  );

UINTN
CborGetBuffer (
  VOID  *RootEncoder,
  VOID         *Buffer
  );

#endif
