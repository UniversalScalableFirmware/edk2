/** @file
  Mock the MdePkg\Include\Library\DebugLib.h

**/

#ifndef __DEBUG_MOCK_LIB_H__
#define __DEBUG_MOCK_LIB_H__
#undef _DEBUG
#include <MdePkg/Include/Library/DebugLib.h>
#include <assert.h>
#include <string.h>

void
myassert(
  IN CONST CHAR8* FileName,
  IN UINTN        LineNumber,
  BOOLEAN         x
);

#define CopyMem  memcpy
#undef ASSERT
#define ASSERT(x)  do{ myassert(__FILE__, __LINE__, x); }while(0)

#define FixedPcdGet32(TokenName)  _PCD_VALUE_##TokenName
#define _PCD_VALUE_PcdSystemMemoryUefiRegionSize  0x04000000U

#include <Uefi/UefiBaseType.h>
extern EFI_GUID  gEfiHobMemoryAllocModuleGuid;
extern EFI_GUID  gEfiHobMemoryAllocStackGuid;
extern EFI_GUID  gUniversalPayloadMemoryMapGuid;
extern EFI_GUID  gUefiAcpiBoardInfoGuid;
extern EFI_GUID  gUniversalPayloadAcpiTableGuid;
extern EFI_GUID  gUniversalPayloadPciRootBridgeInfoGuid;
extern EFI_GUID  gUniversalPayloadSmbios3TableGuid;
extern EFI_GUID  gUniversalPayloadSmbiosTableGuid;
extern EFI_GUID  gUniversalPayloadExtraDataGuid;
extern EFI_GUID  gUniversalPayloadSerialPortInfoGuid;
extern EFI_GUID  gEfiMemoryTypeInformationGuid;
extern EFI_GUID  gEdkiiBootManagerMenuFileGuid;
extern EFI_GUID  gEfiHobMemoryAllocBspStoreGuid;
#endif
