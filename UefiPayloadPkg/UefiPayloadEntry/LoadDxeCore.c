/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

VOID*
AllocateCodePages (
  IN  UINTN     Pages
  )
{
  VOID                    *Alloc;
  EFI_PEI_HOB_POINTERS    Hob;

  Alloc = AllocatePages (Pages);
  if (Alloc == NULL) {
    return NULL;
  }

  DEBUG ((EFI_D_ERROR, "Alloc addr = 0x%p\n",  Alloc));
  DEBUG ((EFI_D_ERROR, "Alloc addr = 0x%llx\n",  (UINTN)Alloc));


  // find the HOB we just created, and change the type to EfiBootServicesCode
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (Hob.Raw != NULL) {
     DEBUG ((EFI_D_ERROR, "   MemoryBaseAddress = 0x%llx\n",  Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress));

    if (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress == (UINTN)Alloc) {
      Hob.MemoryAllocation->AllocDescriptor.MemoryType = EfiBootServicesCode;
      return Alloc;
    }
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (Hob));
  }

  DEBUG ((EFI_D_ERROR, " Not found\n"));

  ASSERT (FALSE);

  FreePages (Alloc, Pages);
  return NULL;
}


EFI_STATUS
LoadPeCoffImage (
  IN  VOID                          *PeCoffImage,
  OUT EFI_PHYSICAL_ADDRESS          *ImageAddress,
  OUT UINT64                        *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS          *EntryPoint
  )
{
  RETURN_STATUS                     Status;
  PE_COFF_LOADER_IMAGE_CONTEXT      ImageContext;
  VOID                              *Buffer;

  ZeroMem (&ImageContext, sizeof (ImageContext));

  ImageContext.Handle    = PeCoffImage;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate Memory for the image
  //
  Buffer = AllocateCodePages (EFI_SIZE_TO_PAGES((UINT32)ImageContext.ImageSize));
  ASSERT (Buffer != 0);

  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;

  //
  // Load the image to our new buffer
  //
  Status = PeCoffLoaderLoadImage (&ImageContext);
  ASSERT_EFI_ERROR (Status);

  //
  // Relocate the image in our new buffer
  //
  Status = PeCoffLoaderRelocateImage (&ImageContext);
  ASSERT_EFI_ERROR (Status);


  *ImageAddress = ImageContext.ImageAddress;
  *ImageSize    = ImageContext.ImageSize;
  *EntryPoint   = ImageContext.EntryPoint;

  return Status;
}

/**
  This function searchs a given file type within a valid FV.

  @param FvHeader        A pointer to firmware volume header that contains the set of files
                         to be searched.
  @param FileType        File type to be searched.
  @param FileHeader      A pointer to the discovered file, if successful.

  @retval EFI_SUCCESS    Successfully found FileType
  @retval EFI_NOT_FOUND  File type can't be found.
**/
EFI_STATUS
FvFindFile (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader,
  IN  EFI_FV_FILETYPE             FileType,
  OUT EFI_FFS_FILE_HEADER         **FileHeader
  )
{
  EFI_PHYSICAL_ADDRESS        CurrentAddress;
  EFI_PHYSICAL_ADDRESS        EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER         *File;
  UINT32                      Size;
  EFI_PHYSICAL_ADDRESS        EndOfFile;

  CurrentAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) FvHeader;
  EndOfFirmwareVolume = CurrentAddress + FvHeader->FvLength;

  //
  // Loop through the FFS files
  //
  for (EndOfFile = CurrentAddress + FvHeader->HeaderLength; ; ) {
    CurrentAddress = (EndOfFile + 7) & 0xfffffffffffffff8ULL;
    if (CurrentAddress > EndOfFirmwareVolume) {
      break;
    }

    File = (EFI_FFS_FILE_HEADER*)(UINTN) CurrentAddress;
    if (IS_FFS_FILE2 (File)) {
      Size = FFS_FILE2_SIZE (File);
      if (Size <= 0x00FFFFFF) {
        break;
      }
    } else {
      Size = FFS_FILE_SIZE (File);
      if (Size < sizeof (EFI_FFS_FILE_HEADER)) {
        break;
      }
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      break;
    }

    //
    // Look for file type
    //
    if (File->Type == FileType) {
      *FileHeader = File;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  This function searchs a given section type within a valid FFS file.

  @param  FileHeader            A pointer to the file header that contains the set of sections to
                                be searched.
  @param  SearchType            The value of the section type to search.
  @param  SectionData           A pointer to the discovered section, if successful.

  @retval EFI_SUCCESS           The section was found.
  @retval EFI_NOT_FOUND         The section was not found.

**/
EFI_STATUS
FileFindSection (
  IN EFI_FFS_FILE_HEADER        *FileHeader,
  IN EFI_SECTION_TYPE           SectionType,
  OUT VOID                      **SectionData
  )
{
  UINT32                        FileSize;
  EFI_COMMON_SECTION_HEADER     *Section;
  UINT32                        SectionSize;
  UINT32                        Index;

  if (IS_FFS_FILE2 (FileHeader)) {
    FileSize = FFS_FILE2_SIZE (FileHeader);
  } else {
    FileSize = FFS_FILE_SIZE (FileHeader);
  }
  FileSize  -= sizeof (EFI_FFS_FILE_HEADER);

  Section    = (EFI_COMMON_SECTION_HEADER *)(FileHeader + 1);
  Index      = 0;
  while (Index < FileSize) {
    if (Section->Type == SectionType) {
      if (IS_SECTION2 (Section)) {
        *SectionData = (VOID *)((UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER2));
      } else {
        *SectionData = (VOID *)((UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER));
      }
      return EFI_SUCCESS;
    }

    if (IS_SECTION2 (Section)) {
      SectionSize = SECTION2_SIZE (Section);
    } else {
      SectionSize = SECTION_SIZE (Section);
    }

    SectionSize = GET_OCCUPIED_SIZE (SectionSize, 4);
    ASSERT (SectionSize != 0);
    Index += SectionSize;

    Section = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)Section + SectionSize);
  }

  return EFI_NOT_FOUND;
}


/**
  Find DXE core from FV and build DXE core HOBs.

  @param[in]   FvBase                FV base to load DXE core from
  @param[out]  DxeCoreEntryPoint     DXE core entry point

  @retval EFI_SUCCESS        If it completed successfully.
  @retval EFI_NOT_FOUND      If it failed to load DXE FV.
**/
EFI_STATUS
LoadDxeCore (
  IN  UINTN                    FvBase,
  OUT PHYSICAL_ADDRESS        *DxeCoreEntryPoint
  )
{
  EFI_STATUS                  Status;
  EFI_FIRMWARE_VOLUME_HEADER  *PayloadFv;
  EFI_FIRMWARE_VOLUME_HEADER  *DxeCoreFv;
  EFI_FFS_FILE_HEADER         *FileHeader;
  VOID                        *PeCoffImage;
  EFI_PHYSICAL_ADDRESS        ImageAddress;
  UINT64                      ImageSize;

  PayloadFv = (EFI_FIRMWARE_VOLUME_HEADER *)FvBase;
  DxeCoreFv = PayloadFv;
  Status = FvFindFile (DxeCoreFv, EFI_FV_FILETYPE_DXE_CORE, &FileHeader);  
  if (EFI_ERROR (Status)) {
    //
    // DXE FV is inside Payload FV. Here find DXE FV from Payload FV
    //
    Status = FvFindFile (PayloadFv, EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, &FileHeader);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = FileFindSection (FileHeader, EFI_SECTION_FIRMWARE_VOLUME_IMAGE, (VOID **)&DxeCoreFv);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Find DXE core file from DXE FV
    //
    Status = FvFindFile (DxeCoreFv, EFI_FV_FILETYPE_DXE_CORE, &FileHeader);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Report DXE FV to DXE core
  //
  BuildFvHob ((EFI_PHYSICAL_ADDRESS) (UINTN) DxeCoreFv, DxeCoreFv->FvLength);


  Status = FileFindSection (FileHeader, EFI_SECTION_PE32, (VOID **)&PeCoffImage);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get DXE core info
  //
  Status = LoadPeCoffImage (PeCoffImage, &ImageAddress, &ImageSize, DxeCoreEntryPoint);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BuildModuleHob (&FileHeader->Name, ImageAddress, EFI_SIZE_TO_PAGES ((UINT32) ImageSize) * EFI_PAGE_SIZE, *DxeCoreEntryPoint);

  return EFI_SUCCESS;
}
