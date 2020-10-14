/** @file
  This library will parse the coreboot table in memory and extract those required
  information.

  Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Guid/GraphicsInfoHob.h>
#include <Guid/MemoryMapInfoGuid.h>
#include <Guid/SerialPortInfoGuid.h>
#include <Guid/AcpiBoardInfoGuid.h>

#ifndef __BOOTLOADER_PARSE_LIB__
#define __BOOTLOADER_PARSE_LIB__

#define GET_BOOTLOADER_PARAMETER()      (*(UINTN *)(UINTN)(PcdGet32(PcdPayloadStackTop) - sizeof(UINT64)))
#define SET_BOOTLOADER_PARAMETER(Value) GET_BOOTLOADER_PARAMETER()=Value

typedef RETURN_STATUS \
        (*BL_MEM_INFO_CALLBACK) (MEMROY_MAP_ENTRY *MemoryMapEntry, VOID *Param);

#endif
