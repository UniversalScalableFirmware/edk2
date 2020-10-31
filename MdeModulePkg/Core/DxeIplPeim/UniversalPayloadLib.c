/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <IndustryStandard/UniversalPayload.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/MemoryAllocationLib.h>

//#undef   DEBUG_VERBOSE
//#define  DEBUG_VERBOSE   DEBUG_INFO

/**
  Performs an specific relocation fpr PECOFF images. The caller needs to
  allocate enough buffer at the PreferedImageBase

  @param  ImageBase        Pointer to the current image base.

  @return Status code.

**/
RETURN_STATUS
EFIAPI
RelocateUniversalPayload (
  IN UPLD_RELOC_HEADER      *UpldRelocHdr,
  IN UINTN                   ActPldBase,
  IN UINTN                   FixupDelta
  )
{
  RETURN_STATUS                   Status;
  UINT32                          RelocSectionSize;
  UINT16                         *RelocDataPtr;
  UINT32                          PageRva;
  UINT32                          BlockSize;
  UINTN                           Index;
  UINT8                           Type;
  UINT32                         *DataPtr;
  UINT16                          Offset;
  UINT16                          TypeOffset;
  UINT32                          ImgOffset;
  UINT32                          Adjust;
  UINT64                          PeBase;
  PE_RELOC_BLOCK_HEADER          *RelocBlkHdr;

  Status = RETURN_SUCCESS;

  if (UpldRelocHdr->RelocFmt == UPLD_RELOC_FMT_RAW) {
    RelocDataPtr      =  (UINT16 *)(&UpldRelocHdr[1]);
    RelocSectionSize  = UpldRelocHdr->CommonHeader.HeaderLength - sizeof(UPLD_RELOC_HEADER);
  } else if (UpldRelocHdr->RelocFmt == UPLD_RELOC_FMT_PTR) {
    RelocBlkHdr       = (PE_RELOC_BLOCK_HEADER *)(&UpldRelocHdr[1]);
    RelocDataPtr      = (UINT16 *)(RelocBlkHdr->PageRva + ActPldBase);
    RelocSectionSize  = RelocBlkHdr->BlockSize;
  } else {
    // Not support yet
    DEBUG ((DEBUG_ERROR, "Relocation format is not supported yet !\n"));
    return EFI_UNSUPPORTED;
  }

  PeBase             = ActPldBase + UpldRelocHdr->RelocImgOffset;
  Adjust             = UpldRelocHdr->RelocImgStripped;
  DEBUG ((DEBUG_VERBOSE, "BaseGap: 0x%x  Adjust: 0x%x\n", FixupDelta, Adjust));

  // This seems to be a bug in the way MS generates the reloc fixup blocks.
  // After we have gone thru all the fixup blocks in the .reloc section, the
  // variable RelocSectionSize should ideally go to zero. But I have found some orphan
  // data after all the fixup blocks that don't quite fit anywhere. So, I have
  // changed the check to a greater-than-eight. It should be at least eight
  // because the PageRva and the BlockSize together take eight bytes. If less
  // than 8 are remaining, then those are the orphans and we need to disregard them.
  while (RelocSectionSize >= 8) {
    // Read the Page RVA and Block Size for the current fixup block.
    PageRva   = * (UINT32 *) (RelocDataPtr + 0);
    BlockSize = * (UINT32 *) (RelocDataPtr + 2);
    DEBUG ((DEBUG_VERBOSE, "PageRva = %04X  BlockSize = %04X\n", PageRva, BlockSize));

    RelocDataPtr += 4;
    if (BlockSize == 0) {
      break;
    }

    RelocSectionSize -= sizeof (UINT32) * 2;

    // Extract the correct number of Type/Offset entries. This is given by:
    // Loop count = Number of relocation items =
    // (Block Size - 4 bytes (Page RVA field) - 4 bytes (Block Size field)) divided
    // by 2 (each Type/Offset entry takes 2 bytes).
    DEBUG ((DEBUG_VERBOSE, "LoopCount = %04x\n", ((BlockSize - 2 * sizeof(UINT32)) / sizeof(UINT16))));
    for (Index = 0; Index < ((BlockSize - 2 * sizeof (UINT32)) / sizeof (UINT16)); Index++) {
      TypeOffset = *RelocDataPtr++;
      Type   = (UINT8) ((TypeOffset & 0xf000) >> 12);
      Offset = (UINT16) ((UINT16)TypeOffset & 0x0fff);
      RelocSectionSize -= sizeof (UINT16);
      ImgOffset = PageRva + Offset - Adjust;
      DEBUG ((DEBUG_VERBOSE, "%d: PageRva: %08x Offset: %04x Type: %x \n", Index, PageRva, ImgOffset, Type));
      DataPtr = (UINT32 *)(UINTN)(PeBase + ImgOffset);
      switch (Type) {
      case 0:
        break;
      case 1:
        *DataPtr += (((UINT32)FixupDelta >> 16) & 0x0000ffff);
        break;
      case 2:
        *DataPtr += ((UINT32)FixupDelta & 0x0000ffff);
        break;
      case 3:
        *DataPtr += (UINT32)FixupDelta;
        break;
      case 10:
        *(UINT64 *)DataPtr += FixupDelta;
        break;
      default:
        DEBUG ((DEBUG_ERROR, "Unknown RELOC type: %d\n", Type));
        break;
      }
    }
  }

  return Status;
}



/**
  Load universal payload image into memory.

  @param[in]  ImageBase    The universal payload image base
  @param[in]  PldEntry     The payload image entry point
  @param[in]  PldMachine   Indicate image machine type

  @retval     EFI_SUCCESS      The image was loaded successfully
              EFI_ABORTED      The image loading failed
              EFI_UNSUPPORTED  The relocation format is not supported

**/
EFI_STATUS
EFIAPI
LoadUniversalPayload (
  IN  UPLD_INFO_HEADER         *UpldInfoHdr,
  OUT UINT8                    **PldData,
  OUT UNIVERSAL_PAYLOAD_ENTRY  *PldEntry,
  OUT UINT32                   *PldMachine
  )
{
  EFI_STATUS              Status;
  UPLD_RELOC_HEADER       *UpldRelocHdr;
  CHAR8                   ImageId[9];
  UINT8                   *Data;

  if (UpldInfoHdr->CommonHeader.Identifier != UPLD_IMAGE_HEADER_ID) {
    DEBUG ((DEBUG_ERROR, "UPayload image format is invalid !\n"));
    return EFI_ABORTED;
  }

  if (PldMachine != NULL) {
    *PldMachine = UpldInfoHdr->Machine;
  }

  ImageId[8] = 0;
  *(UINT64 *)ImageId = *(UINT64 *)UpldInfoHdr->ImageId;
  DEBUG ((DEBUG_INFO,  "UPayload ImageId is %a, Machine: %04X\n", ImageId, UpldInfoHdr->Machine));

  Data = (UINT8 *)UpldInfoHdr + UpldInfoHdr->ImageOffset;
  if ((UpldInfoHdr->Capability & UPLD_IMAGE_CAP_RELOC) == 0) {
    DEBUG ((DEBUG_INFO,  "UPayload has no relocation info\n"));
    // If relocation is not provided,
    // it is required to load to the preferred base
    if (UpldInfoHdr->ImageBase != (UINTN) Data) {
      // TODO: Need to allocate memory in the desired address: UpldInfoHdr->ImageBase
      CopyMem ((VOID *)(UINTN) UpldInfoHdr->ImageBase, Data, UpldInfoHdr->ImageLength);
      Data = (UINT8 *) (UINTN) UpldInfoHdr->ImageBase;
    }

    if (PldEntry != NULL) {
      *PldEntry = (UNIVERSAL_PAYLOAD_ENTRY)(UINTN)((UINT8 *) Data + UpldInfoHdr->EntryPointOffset);
      DEBUG ((DEBUG_VERBOSE, "Image entry point is at %p\n", *PldEntry));
    }
    return EFI_SUCCESS;
  }


  UpldRelocHdr = (UPLD_RELOC_HEADER *)&UpldInfoHdr[1];
  if (UpldRelocHdr->CommonHeader.Identifier != UPLD_RELOC_ID) {
    DEBUG ((DEBUG_ERROR, "UPayload relocation table is invalid !\n"));
    return EFI_ABORTED;
  }

  if (ALIGN_POINTER (Data, SIZE_4KB) != Data) {
    Data = AllocatePages (EFI_SIZE_TO_PAGES (UpldInfoHdr->ImageLength));
    CopyMem (Data, (UINT8 *)UpldInfoHdr + UpldInfoHdr->ImageOffset, UpldInfoHdr->ImageLength);
  }
  
  Status = RelocateUniversalPayload (UpldRelocHdr, (UINTN) Data, (UINTN) Data - (UINT32)UpldInfoHdr->ImageBase);
  if (!EFI_ERROR(Status)) {
    if (PldEntry != NULL) {
      *PldEntry = (UNIVERSAL_PAYLOAD_ENTRY)(UINTN)((UINT8 *) Data + UpldInfoHdr->EntryPointOffset);
      DEBUG ((DEBUG_VERBOSE, "Image entry point is at %p\n", *PldEntry));
    }

    if (PldData != NULL) {
      *PldData = Data;
      DEBUG ((DEBUG_VERBOSE, "Data at %p\n", *PldData));
    }
  }

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "UPayload relocation failed - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO,  "UPayload was relocated successfully\n"));
  }

  return Status;
}


/**
  Authenticate a universal payload image.

  @param[in]  ImageBase    The universal payload image base

  @retval     EFI_SUCCESS      The image was authenticated successfully
              EFI_ABORTED      The image loading failed
              EFI_UNSUPPORTED  The relocation format is not supported
              EFI_SECURITY_VIOLATION  The image does not contain auth info


EFI_STATUS
EFIAPI
AuthenticateUniversalPayload (
  IN  UINT32                    ImageBase
)
{
  EFI_STATUS              Status;
  UPLD_INFO_HEADER       *UpldInfoHdr;
  UPLD_AUTH_HEADER       *UpldAuthHdr;
  UINT32                  Address;
  UPLD_SIGNATURE_HDR     *Signature;
  UPLD_PUB_KEY_HDR       *PubKey;

  UpldInfoHdr = (UPLD_INFO_HEADER *)(UINTN)ImageBase;
  if (UpldInfoHdr->CommonHeader.Identifier != UPLD_IMAGE_HEADER_ID) {
    DEBUG ((DEBUG_ERROR, "UPayload image format is invalid !\n"));
    return EFI_ABORTED;
  }

  if (!FeaturePcdGet (PcdVerifiedBootEnabled)) {
    DEBUG ((DEBUG_INFO,  "UPayload authentication skipped\n"));
    return EFI_SUCCESS;
  }

  if ((UpldInfoHdr->Capability & UPLD_IMAGE_CAP_AUTH) == 0) {
    DEBUG ((DEBUG_ERROR, "UPayload image does not support authentification !\n"));
    return EFI_SECURITY_VIOLATION;
  }

  Address = ImageBase + UpldInfoHdr->ImageOffset + UpldInfoHdr->ImageLength;
  Address = ALIGN_UP (Address, sizeof(UINT32));
  UpldAuthHdr = (UPLD_AUTH_HEADER *)(UINTN)Address;
  if (UpldAuthHdr->CommonHeader.Identifier != UPLD_AUTH_ID) {
    DEBUG ((DEBUG_ERROR, "UPayload auth table is invalid !\n"));
    return EFI_ABORTED;
  }

  Signature = (UPLD_SIGNATURE_HDR *)&UpldAuthHdr[1];
  if (Signature->Identifier != UPLD_AUTH_SIGNATURE_ID) {
    DEBUG ((DEBUG_ERROR, "UPayload signature header is invalid !\n"));
    return EFI_ABORTED;
  }

  PubKey = (UPLD_PUB_KEY_HDR *)((UINT8 *)&Signature[1] + Signature->SigSize);
  if (PubKey->Identifier != UPLD_AUTH_PUBKEY_ID) {
    DEBUG ((DEBUG_ERROR, "UPayload pubkey header is invalid !\n"));
    return EFI_ABORTED;
  }

  Status = DoRsaVerify (  (UINT8 *)(UINTN)ImageBase,
                          UpldInfoHdr->ImageOffset + UpldInfoHdr->ImageLength,
                          HASH_USAGE_PUBKEY_OS,
                          (SIGNATURE_HDR *)Signature,  (PUB_KEY_HDR *)PubKey,
                          HASH_TYPE_SHA384, NULL, NULL);

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "UPayload authentication failed - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO,  "UPayload authentication passed\n"));
  }

  return EFI_SUCCESS;
}
**/
