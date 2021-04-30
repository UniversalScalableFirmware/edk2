/** @file
  ELF Load Image Support

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <UniversalPayload/UniversalPayload.h>
#include <UniversalPayload/ExtraData.h>

#include <Ppi/LoadFile.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include "ElfLib.h"

/**
  The wrapper function of PeiLoadImageLoadImage().

  @param This            - Pointer to EFI_PEI_LOAD_FILE_PPI.
  @param FileHandle      - Pointer to the FFS file header of the image.
  @param ImageAddressArg - Pointer to PE/TE image.
  @param ImageSizeArg    - Size of PE/TE image.
  @param EntryPoint      - Pointer to entry point of specified image file for output.
  @param AuthenticationState - Pointer to attestation authentication state of image.

  @return Status of PeiLoadImageLoadImage().

**/
EFI_STATUS
EFIAPI
PeiLoadFileLoadElf (
  IN     CONST EFI_PEI_LOAD_FILE_PPI  *This,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *ImageAddressArg,  OPTIONAL
  OUT    UINT64                       *ImageSizeArg,     OPTIONAL
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  )
{
  EFI_STATUS         Status;
  VOID               *Elf;
  PLD_EXTRA_DATA     *ExtraData;
  ELF_IMAGE_CONTEXT  Context;
  PLD_INFO_HEADER    *PldInfo;
  UINT32             Index;
  UINT16             ExtraDataIndex;
  CHAR8              *SectionName;
  UINTN              Offset;
  UINTN              Size;
  UINTN              ExtraDataCount;
  UINTN              Instance;

  //
  // ELF is added to file as RAW section for EDKII bootloader.
  // But RAW section might be added by build tool before the ELF RAW section when alignment is specified for ELF RAW section.
  // Below loop skips the RAW section that doesn't contain valid ELF image.
  //
  Instance = 0;
  do {
    Status = PeiServicesFfsFindSectionData3 (EFI_SECTION_RAW, Instance++, FileHandle, &Elf, AuthenticationState);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    ZeroMem (&Context, sizeof (Context));
    Status = ParseElfImage (Elf, &Context);
  } while (EFI_ERROR (Status));

  DEBUG ((
    DEBUG_INFO, "ELF File Size: 0x%08X, Mem Size: 0x%08x, Reload: %d\n",
    Context.FileSize, Context.ImageSize, Context.ReloadRequired
    ));

  //
  // Get PLD_INFO and number of additional PLD sections.
  //
  PldInfo        = NULL;
  ExtraDataCount = 0;
  for (Index = 0; Index < Context.ShNum; Index++) {
    Status = GetElfSectionName (&Context, Index, &SectionName);
    if (EFI_ERROR(Status)) {
      continue;
    }
    DEBUG ((DEBUG_ERROR, "ELF Section[%d]: %a\n", Index, SectionName));
    if (AsciiStrCmp(SectionName, PLD_INFO_SEC_NAME) == 0) {
      Status = GetElfSectionPos (&Context, Index, &Offset, &Size);
      if (!EFI_ERROR(Status)) {
        PldInfo = (PLD_INFO_HEADER *)(Context.FileBase + Offset);
      }
    } else if (AsciiStrnCmp(SectionName, PLD_EXTRA_SEC_NAME_PREFIX, PLD_EXTRA_SEC_NAME_PREFIX_LENGTH) == 0) {
      Status = GetElfSectionPos (&Context, Index, &Offset, &Size);
      if (!EFI_ERROR (Status)) {
        ExtraDataCount++;
      }
    }
  }

  //
  // Report the additional PLD sections through HOB.
  //
  ExtraData = BuildGuidHob (
               &gPldExtraDataGuid,
               sizeof (PLD_EXTRA_DATA) + ExtraDataCount * sizeof (PLD_EXTRA_DATA_ENTRY)
               );
  ExtraData->Count = ExtraDataCount;
  if (ExtraDataCount != 0) {
    for (ExtraDataIndex = 0, Index = 0; Index < Context.ShNum; Index++) {
      Status = GetElfSectionName (&Context, Index, &SectionName);
      if (EFI_ERROR(Status)) {
        continue;
      }
      if (AsciiStrnCmp(SectionName, PLD_EXTRA_SEC_NAME_PREFIX, PLD_EXTRA_SEC_NAME_PREFIX_LENGTH) == 0) {
        Status = GetElfSectionPos (&Context, Index, &Offset, &Size);
        if (!EFI_ERROR (Status)) {
          ASSERT (ExtraDataIndex < ExtraDataCount);
          AsciiStrCpyS (
            ExtraData->Entry[ExtraDataIndex].Identifier,
            sizeof(ExtraData->Entry[ExtraDataIndex].Identifier),
            SectionName + PLD_EXTRA_SEC_NAME_PREFIX_LENGTH
            );
          ExtraData->Entry[ExtraDataIndex].Base = (UINTN)(Context.FileBase + Offset);
          ExtraData->Entry[ExtraDataIndex].Size = Size;
          ExtraDataIndex++;
        }
      }
    }
  }

  if (Context.ReloadRequired) {
    Context.ImageAddress = AllocatePages (EFI_SIZE_TO_PAGES (Context.ImageSize));
  } else {
    Context.ImageAddress = Context.FileBase;
  }

  //
  // Load ELF into the required base
  //
  Status = LoadElfImage (&Context);
  if (!EFI_ERROR(Status)) {
    *ImageAddressArg = (UINTN) Context.ImageAddress;
    *EntryPoint      = Context.EntryPoint;
    *ImageSizeArg    = Context.ImageSize;
  }
  return Status;
}


EFI_PEI_LOAD_FILE_PPI   mPeiLoadFilePpi = {
  PeiLoadFileLoadElf
};


EFI_PEI_PPI_DESCRIPTOR     gPpiLoadFilePpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiLoadFilePpiGuid,
  &mPeiLoadFilePpi
};
/**

  Install Pei Load File PPI.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point executes successfully.
  @retval Others      Some error occurs during the execution of this function.

**/
EFI_STATUS
EFIAPI
InitializeElfLoaderPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;
  Status = PeiServicesInstallPpi (&gPpiLoadFilePpiList);

  return Status;
}
