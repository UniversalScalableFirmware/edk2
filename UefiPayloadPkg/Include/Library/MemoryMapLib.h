/** @file
  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MEMORY_MAP_LIB_H__
#define __MEMORY_MAP_LIB_H__

/**
  Create a Guid Hob containing the memory map descriptor table.
  After calling the function, PEI should not do any memory allocation operation.

  @retval EFI_SUCCESS           The Memory Map is created successfully.
**/
RETURN_STATUS
EFIAPI
BuildMemoryMapHob (
  VOID
  );

#endif
