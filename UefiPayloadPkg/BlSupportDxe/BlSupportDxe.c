/** @file
  This driver will set PCDs based on information from the bootloader.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "BlSupportDxe.h"

/**
  Reserve MMIO/IO resource in GCD

  @param  IsMMIO        Flag of whether it is mmio resource or io resource.
  @param  GcdType       Type of the space.
  @param  BaseAddress   Base address of the space.
  @param  Length        Length of the space.
  @param  Alignment     Align with 2^Alignment
  @param  ImageHandle   Handle for the image of this driver.

  @retval EFI_SUCCESS   Reserve successful
**/
EFI_STATUS
ReserveResourceInGcd (
  IN BOOLEAN               IsMMIO,
  IN UINTN                 GcdType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINTN                 Alignment,
  IN EFI_HANDLE            ImageHandle
  )
{
  EFI_STATUS               Status;

  if (IsMMIO) {
    Status = gDS->AddMemorySpace (
                    GcdType,
                    BaseAddress,
                    Length,
                    EFI_MEMORY_UC
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Failed to add memory space :0x%lx 0x%lx\n",
        BaseAddress,
        Length
        ));
    }
    ASSERT_EFI_ERROR (Status);
    Status = gDS->AllocateMemorySpace (
                    EfiGcdAllocateAddress,
                    GcdType,
                    Alignment,
                    Length,
                    &BaseAddress,
                    ImageHandle,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  } else {
    Status = gDS->AddIoSpace (
                    GcdType,
                    BaseAddress,
                    Length
                    );
    ASSERT_EFI_ERROR (Status);
    Status = gDS->AllocateIoSpace (
                    EfiGcdAllocateAddress,
                    GcdType,
                    Alignment,
                    Length,
                    &BaseAddress,
                    ImageHandle,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }
  return Status;
}


/**
  Set the platform related PCDs using ACPI table

  @param[in]  AcpiTableBase         ACPI table start address in memory

  @retval RETURN_SUCCESS            Successfully set PCDs based ACPI table.
  @retval RETURN_NOT_FOUND          Failed to find the required info

**/
RETURN_STATUS
SetPcdsUsingAcpiTable (
  IN   UINT64                                   AcpiTableBase
  )
{
  EFI_STATUS                                    Status;
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;
  UINT32                                        *Entry32;
  UINTN                                         Entry32Num;
  EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE     *Fadt;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;
  UINT64                                        *Entry64;
  UINTN                                         Entry64Num;
  UINTN                                         Idx;
  UINT32                                        *Signature;
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *MmCfgHdr;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *MmCfgBase;
  UINT64                                        PcieBaseAddress;
  UINT64                                        PcieBaseSize;

  Rsdp = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)(UINTN)AcpiTableBase;
  DEBUG ((DEBUG_INFO, "Rsdp at 0x%p\n", Rsdp));
  DEBUG ((DEBUG_INFO, "Rsdt at 0x%x, Xsdt at 0x%lx\n", Rsdp->RsdtAddress, Rsdp->XsdtAddress));

  //
  // Search Rsdt First
  //
  Fadt     = NULL;
  MmCfgHdr = NULL;
  Rsdt     = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Rsdp->RsdtAddress);
  if (Rsdt != NULL) {
    Entry32  = (UINT32 *)(Rsdt + 1);
    Entry32Num = (Rsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 2;
    for (Idx = 0; Idx < Entry32Num; Idx++) {
      Signature = (UINT32 *)(UINTN)Entry32[Idx];
      if (*Signature == EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
        Fadt = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)Signature;
        DEBUG ((DEBUG_INFO, "Found Fadt in Rsdt\n"));
      }

      if (*Signature == EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
        MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)Signature;
        DEBUG ((DEBUG_INFO, "Found MM config address in Rsdt\n"));
      }

      if ((Fadt != NULL) && (MmCfgHdr != NULL)) {
        goto Done;
      }
    }
  }

  //
  // Search Xsdt Second
  //
  Xsdt     = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Rsdp->XsdtAddress);
  if (Xsdt != NULL) {
    Entry64  = (UINT64 *)(Xsdt + 1);
    Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
    for (Idx = 0; Idx < Entry64Num; Idx++) {
      Signature = (UINT32 *)(UINTN)Entry64[Idx];
      if (*Signature == EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
        Fadt = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)Signature;
        DEBUG ((DEBUG_INFO, "Found Fadt in Xsdt\n"));
      }

      if (*Signature == EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
        MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)Signature;
        DEBUG ((DEBUG_INFO, "Found MM config address in Xsdt\n"));
      }

      if ((Fadt != NULL) && (MmCfgHdr != NULL)) {
        goto Done;
      }
    }
  }

  if (Fadt == NULL) {
    return RETURN_NOT_FOUND;
  }

Done:

  if (MmCfgHdr != NULL) {
    MmCfgBase = (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *)((UINT8*) MmCfgHdr + sizeof (*MmCfgHdr));
    PcieBaseAddress = MmCfgBase->BaseAddress;
    PcieBaseSize    = (MmCfgBase->EndBusNumber + 1 - MmCfgBase->StartBusNumber) * 4096 * 32 * 8;
  } else {
    PcieBaseAddress = 0;
    PcieBaseSize    = 0;
  }
  DEBUG ((DEBUG_INFO, "PmCtrl  Reg 0x%lx\n",  Fadt->Pm1aCntBlk));
  DEBUG ((DEBUG_INFO, "PmTimer Reg 0x%lx\n",  Fadt->PmTmrBlk));
  DEBUG ((DEBUG_INFO, "Reset   Reg 0x%lx\n",  Fadt->ResetReg.Address));
  DEBUG ((DEBUG_INFO, "Reset   Value 0x%x\n", Fadt->ResetValue));
  DEBUG ((DEBUG_INFO, "PmEvt   Reg 0x%lx\n",  Fadt->Pm1aEvtBlk));
  DEBUG ((DEBUG_INFO, "PmGpeEn Reg 0x%lx\n",  Fadt->Gpe0Blk + Fadt->Gpe0BlkLen / 2));
  DEBUG ((DEBUG_INFO, "PcieBaseAddr 0x%lx\n", PcieBaseAddress));
  DEBUG ((DEBUG_INFO, "PcieBaseSize 0x%lx\n", PcieBaseSize));

  //
  // Verify values for proper operation
  //
  ASSERT(Fadt->Pm1aCntBlk       != 0);
  ASSERT(Fadt->PmTmrBlk         != 0);
  ASSERT(Fadt->ResetReg.Address != 0);
  ASSERT(Fadt->Pm1aEvtBlk       != 0);
  ASSERT(Fadt->Gpe0Blk          != 0);

  Status = PcdSet32S (PcdAcpiPm1aControlAddress, Fadt->Pm1aCntBlk);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet32S (PcdAcpiPm1aEventAddress, Fadt->Pm1aEvtBlk);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet32S (PcdAcpiGpe0EnableAddress, Fadt->Gpe0Blk + Fadt->Gpe0BlkLen / 2);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet64S (PcdAcpiResetRegister, Fadt->ResetReg.Address);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet8S (PcdAcpiResetValue, Fadt->ResetValue);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet32S (PcdAcpiPm1TimerRegister, Fadt->PmTmrBlk);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet64S (PcdPciExpressBaseAddress, PcieBaseAddress);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet64S (PcdPciExpressBaseSize, PcieBaseSize);
  ASSERT_EFI_ERROR (Status);

  return RETURN_SUCCESS;
}


/**
  Main entry for the bootloader support DXE module.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
BlDxeEntryPoint (
  IN EFI_HANDLE              ImageHandle,
  IN EFI_SYSTEM_TABLE        *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_HOB_GUID_TYPE          *GuidHob;
  EFI_PEI_GRAPHICS_INFO_HOB  *GfxInfo;
  ACPI_TABLE_HOB             *AcpiTableHob;
  SMBIOS_TABLE_HOB           *SmbiosTable;

  Status = EFI_SUCCESS;
  //
  // Report MMIO/IO Resources
  //
  Status = ReserveResourceInGcd (TRUE, EfiGcdMemoryTypeMemoryMappedIo, 0xFEC00000, SIZE_4KB, 0, ImageHandle); // IOAPIC
  ASSERT_EFI_ERROR (Status);

  Status = ReserveResourceInGcd (TRUE, EfiGcdMemoryTypeMemoryMappedIo, 0xFED00000, SIZE_1KB, 0, ImageHandle); // HPET
  ASSERT_EFI_ERROR (Status);

  //
  // Install Acpi Table
  //
  GuidHob = GetFirstGuidHob (&gEfiAcpiTableGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    AcpiTableHob = (ACPI_TABLE_HOB *)GET_GUID_HOB_DATA (GuidHob);
    DEBUG ((DEBUG_ERROR, "Install Acpi Table at 0x%lx \n", AcpiTableHob->TableAddress));
    Status = gBS->InstallConfigurationTable (&gEfiAcpiTableGuid, (VOID *)(UINTN)AcpiTableHob->TableAddress);
    ASSERT_EFI_ERROR (Status);

    Status = SetPcdsUsingAcpiTable (AcpiTableHob->TableAddress);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Install Smbios Table
  //
  GuidHob = GetFirstGuidHob (&gEfiSmbiosTableGuid);
  if (GuidHob != NULL) {
    SmbiosTable = (SMBIOS_TABLE_HOB *)GET_GUID_HOB_DATA (GuidHob);
    DEBUG ((DEBUG_ERROR, "Install SMBIOS Table at 0x%lx \n", SmbiosTable->TableAddress));
    Status = gBS->InstallConfigurationTable (&gEfiSmbiosTableGuid, (VOID *)(UINTN)SmbiosTable->TableAddress);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Find the frame buffer information and update PCDs
  //
  GuidHob = GetFirstGuidHob (&gEfiGraphicsInfoHobGuid);
  if (GuidHob != NULL) {
    GfxInfo = (EFI_PEI_GRAPHICS_INFO_HOB *)GET_GUID_HOB_DATA (GuidHob);
    Status = PcdSet32S (PcdVideoHorizontalResolution, GfxInfo->GraphicsMode.HorizontalResolution);
    ASSERT_EFI_ERROR (Status);
    Status = PcdSet32S (PcdVideoVerticalResolution, GfxInfo->GraphicsMode.VerticalResolution);
    ASSERT_EFI_ERROR (Status);
    Status = PcdSet32S (PcdSetupVideoHorizontalResolution, GfxInfo->GraphicsMode.HorizontalResolution);
    ASSERT_EFI_ERROR (Status);
    Status = PcdSet32S (PcdSetupVideoVerticalResolution, GfxInfo->GraphicsMode.VerticalResolution);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

