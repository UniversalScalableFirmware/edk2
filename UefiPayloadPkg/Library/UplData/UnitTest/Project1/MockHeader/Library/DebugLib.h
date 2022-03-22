/** @file
  Mock the MdePkg\Include\Library\DebugLib.h

**/

#ifndef __DEBUG_MOCK_LIB_H__
#define __DEBUG_MOCK_LIB_H__
#undef _DEBUG
#include <MdePkg/Include/Library/DebugLib.h>
#include <assert.h>
#include <string.h>

#undef ASSERT
#define ASSERT(x)  assert(x)

#define PcdGet32(TokenName)  _PCD_GET_MODE_32_##TokenName
#define PcdGet64(TokenName)  _PCD_GET_MODE_64_##TokenName
#define PcdGetBool(TokenName)  _PCD_GET_MODE_BOOL_##TokenName

#define _PCD_GET_MODE_BOOL_PcdSerialUseMmio        FALSE
#define _PCD_GET_MODE_64_PcdSerialRegisterBase     0x03F8
#define _PCD_GET_MODE_32_PcdSerialBaudRate         115200
#define _PCD_GET_MODE_32_PcdSerialRegisterStride   1

#define AsciiStrCpyS strcpy_s
#define CopyMem  memcpy

#include <Uefi/UefiBaseType.h>
extern EFI_GUID  gUefiPayloadPkgTokenSpaceGuid;
extern EFI_GUID  gEfiHobMemoryAllocModuleGuid;
extern EFI_GUID  gEfiHobMemoryAllocStackGuid;
extern EFI_GUID  CborRootmapHobGuid;
extern EFI_GUID  gCborBufferGuid;

#endif
