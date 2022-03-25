/** @file
  Get Universal Payload data library

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <PiPei.h>
#include <Library/IoLib.h>
#include <Library/GetUplDataLib.h>
#include "CborGetWrapper.h"

UPL_DATA_DECODER  mRootValue;
CborParser        mParser;

#define RETURN_ON_ERROR_CBOR_DECODE(Expression)      \
    do {                                         \
      if (Expression != CborNoError) {           \
        return RETURN_ABORTED;                   \
      }                                          \
    } while (FALSE)

RETURN_STATUS
CborDecoderGetRootMap (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
{
  RETURN_ON_ERROR_CBOR_DECODE (cbor_parser_init (Buffer, Size, 0, &mParser, &mRootValue));
  if (cbor_value_is_map (&mRootValue) == FALSE) {
    return RETURN_ABORTED;
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
CborDecoderGetUint64 (
  IN  CHAR8   *String,
  OUT UINT64  *Result,
  IN  VOID    *Map
  )
{
  CborValue  Element;

  if (Map == NULL) {
    Map = &mRootValue;
  }

  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_map_find_value (Map, String, &Element));
  if (cbor_value_is_valid (&Element) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_integer (&Element) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }

  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_get_uint64 (&Element, Result));

  return RETURN_SUCCESS;
}

RETURN_STATUS
CborDecoderGetBoolean (
  IN  CHAR8    *String,
  OUT BOOLEAN  *Result,
  IN  VOID     *Map
  )
{
  CborValue  Element;

  if (Map == NULL) {
    Map = &mRootValue;
  }

  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_map_find_value (Map, String, &Element));
  if (cbor_value_is_valid (&Element) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_boolean (&Element) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }

  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_get_boolean (&Element, (bool *)Result));

  return RETURN_SUCCESS;
}

RETURN_STATUS
CborDecoderGetBinary (
  IN     VOID   *Value,
  OUT    VOID   *Result,
  IN OUT UINTN  *Size,
  IN     VOID   *Map
  )
{
  CborValue  Element;

  if (Map == NULL) {
    Map = &mRootValue;
  }

  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_map_find_value (Map, Value, &Element));
  if (cbor_value_is_valid (&Element) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_byte_string (&Element) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }

  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_copy_byte_string (&Element, (uint8_t *)Result, Size, NULL));

  return RETURN_SUCCESS;
}

RETURN_STATUS
CborDecoderGetTextString (
  IN     VOID   *Value,
  OUT    UINT8  *Result,
  IN OUT UINTN  *Size,
  IN     VOID   *Map
  )
{
  CborValue  Element;

  if (Map == NULL) {
    Map = &mRootValue;
  }

  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_map_find_value (Map, Value, &Element));
  if (cbor_value_is_valid (&Element) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_text_string (&Element) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }

  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_copy_text_string (&Element, (char *)Result, Size, NULL));

  return RETURN_SUCCESS;
}

RETURN_STATUS
CborDecoderGetArrayNextMap (
  IN OUT UPL_DATA_DECODER  *NextMap
  )
{
  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_advance (NextMap));

  return RETURN_SUCCESS;
}

RETURN_STATUS
CborDecoderGetArrayLengthAndFirstElement (
  IN     CHAR8             *String,
  OUT    UINTN             *Size,
  OUT    UPL_DATA_DECODER  *Map
  )
{
  CborValue  Array;

  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_map_find_value (&mRootValue, String, &Array));

  if (cbor_value_is_valid (&Array) == FALSE) {
    return RETURN_NOT_FOUND;
  }

  if (cbor_value_is_array (&Array) == FALSE) {
    return RETURN_INVALID_PARAMETER;
  }

  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_get_array_length (&Array, Size));
  RETURN_ON_ERROR_CBOR_DECODE (cbor_value_enter_container (&Array, Map));
  return RETURN_SUCCESS;
}
