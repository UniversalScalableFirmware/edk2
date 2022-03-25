/** @file
  Allows tiny cobr code to build under UEFI (edk2) build environment

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __PROTO11_H__
#define __PROTO11_H__

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

typedef INT8     int8_t;
typedef INT16    int16_t;
typedef INT32    int32_t;
typedef INT64    int64_t;
typedef UINT8    uint8_t;
typedef UINT16   uint16_t;
typedef UINT32   uint32_t;
typedef UINT64   uint64_t;
typedef UINTN    size_t;
typedef BOOLEAN  bool;
typedef UINTN    uintptr_t;
typedef INT8     int_least8_t;
typedef INT16    int_least16_t;
typedef INT32    int_least32_t;
typedef INT64    int_least64_t;
typedef UINT8    uint_least8_t;
typedef UINT16   uint_least16_t;
typedef UINT32   uint_least32_t;
typedef UINT64   uint_least64_t;
typedef UINT8    uint_fast8_t;
typedef INTN     ptrdiff_t;

#define MAX_STRING_SIZE            0x1000
#define memcpy                     CopyMem
#define memset(dest, ch, count)    SetMem(dest,(UINTN)(count),(UINT8)(ch))
#define memmove                    CopyMem
#define memcmp(buf1, buf2, count)  (int)(CompareMem(buf1,buf2,(UINTN)(count)))
#define strlen(str)                (size_t)(AsciiStrnLenS(str, MAX_STRING_SIZE))
#define static_assert(x, y)        _Static_assert(x,#y)
#define offsetof(type, member)     OFFSET_OF(type,member)
#define _byteswap_uint64(value)    SwapBytes64((UINT64)value)
#define _byteswap_ulong(value)     SwapBytes32((UINT32)value)
#define _byteswap_ushort(value)    SwapBytes16((UINT16)value)

#define FILE    VOID
#define false   FALSE
#define true    TRUE
#define assert  ASSERT
#ifndef SIZE_MAX
#define SIZE_MAX  MAX_ADDRESS
#endif

#define INT8_MAX    MAX_INT8
#define UINT8_MAX   MAX_UINT8
#define INT16_MAX   MAX_INT16
#define UINT16_MAX  MAX_UINT16
#define INT32_MAX   MAX_INT32
#define UINT32_MAX  0xFFFFFFFF
#define INT64_MAX   MAX_INT64
#define UINT64_MAX  MAX_UINT64
#define INT8_MIN  MIN_INT8
#define INT16_MIN  MIN_INT16
#define INT32_MIN  MIN_INT32
#define INT64_MIN  MIN_INT64

#endif
