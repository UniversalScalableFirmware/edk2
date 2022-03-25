/** @file
  Get Universal Payload data library

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CBOR_GET_WRAPPER_H__
#define __CBOR_GET_WRAPPER_H__

#include "..\tinycbor\src\cbor.h"
#include  <Base.h>
typedef CborValue UPL_DATA_DECODER;

RETURN_STATUS
CborDecoderGetRootMap (
  VOID   *Buffer,
  UINTN  Size
  );

RETURN_STATUS
CborDecoderGetUint64 (
  IN  CHAR8   *String,
  OUT UINT64  *Result,
  IN  VOID    *Map
  );

RETURN_STATUS
CborDecoderGetTextString (
  IN     VOID   *Value,
  OUT    UINT8  *Result,
  IN OUT UINTN  *Size,
  IN     VOID   *Map
  );

RETURN_STATUS
CborDecoderGetArrayNextMap (
  IN OUT UPL_DATA_DECODER  *NextMap
  );


RETURN_STATUS
CborDecoderGetArrayLengthAndFirstElement (
  IN     CHAR8             *String,
  OUT    UINTN             *Size,
  OUT    UPL_DATA_DECODER  *Map
  );

RETURN_STATUS
CborDecoderGetBoolean (
  IN  CHAR8    *String,
  OUT BOOLEAN  *Result,
  IN  VOID     *Map
  );

RETURN_STATUS
CborDecoderGetBinary (
  IN     VOID   *Value,
  OUT    VOID   *Result,
  IN OUT UINTN  *Size,
  IN     VOID   *Map
  );
#endif
