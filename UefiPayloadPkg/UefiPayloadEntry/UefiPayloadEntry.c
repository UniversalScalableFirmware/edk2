/** @file

  Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

/**
  Find the board related info from ACPI table

  @param  AcpiTableBase          ACPI table start address in memory
  @param  AcpiBoardInfo          Pointer to the acpi board info strucutre

  @retval RETURN_SUCCESS     Successfully find out all the required information.
  @retval RETURN_NOT_FOUND   Failed to find the required info.

**/
RETURN_STATUS
ParseAcpiInfo (
  IN   UINT64                                   AcpiTableBase,
  OUT  ACPI_BOARD_INFO                          *AcpiBoardInfo
  )
{
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

  AcpiBoardInfo->PmCtrlRegBase   = Fadt->Pm1aCntBlk;
  AcpiBoardInfo->PmTimerRegBase  = Fadt->PmTmrBlk;
  AcpiBoardInfo->ResetRegAddress = Fadt->ResetReg.Address;
  AcpiBoardInfo->ResetValue      = Fadt->ResetValue;
  AcpiBoardInfo->PmEvtBase       = Fadt->Pm1aEvtBlk;
  AcpiBoardInfo->PmGpeEnBase     = Fadt->Gpe0Blk + Fadt->Gpe0BlkLen / 2;

  if (MmCfgHdr != NULL) {
    MmCfgBase = (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *)((UINT8*) MmCfgHdr + sizeof (*MmCfgHdr));
    AcpiBoardInfo->PcieBaseAddress = MmCfgBase->BaseAddress;
    AcpiBoardInfo->PcieBaseSize = (MmCfgBase->EndBusNumber + 1 - MmCfgBase->StartBusNumber) * 4096 * 32 * 8;
  } else {
    AcpiBoardInfo->PcieBaseAddress = 0;
    AcpiBoardInfo->PcieBaseSize = 0;
  }
  DEBUG ((DEBUG_INFO, "PmCtrl  Reg 0x%lx\n",  AcpiBoardInfo->PmCtrlRegBase));
  DEBUG ((DEBUG_INFO, "PmTimer Reg 0x%lx\n",  AcpiBoardInfo->PmTimerRegBase));
  DEBUG ((DEBUG_INFO, "Reset   Reg 0x%lx\n",  AcpiBoardInfo->ResetRegAddress));
  DEBUG ((DEBUG_INFO, "Reset   Value 0x%x\n", AcpiBoardInfo->ResetValue));
  DEBUG ((DEBUG_INFO, "PmEvt   Reg 0x%lx\n",  AcpiBoardInfo->PmEvtBase));
  DEBUG ((DEBUG_INFO, "PmGpeEn Reg 0x%lx\n",  AcpiBoardInfo->PmGpeEnBase));
  DEBUG ((DEBUG_INFO, "PcieBaseAddr 0x%lx\n", AcpiBoardInfo->PcieBaseAddress));
  DEBUG ((DEBUG_INFO, "PcieBaseSize 0x%lx\n", AcpiBoardInfo->PcieBaseSize));

  //
  // Verify values for proper operation
  //
  ASSERT(Fadt->Pm1aCntBlk != 0);
  ASSERT(Fadt->PmTmrBlk != 0);
  ASSERT(Fadt->ResetReg.Address != 0);
  ASSERT(Fadt->Pm1aEvtBlk != 0);
  ASSERT(Fadt->Gpe0Blk != 0);

  DEBUG_CODE_BEGIN ();
    BOOLEAN    SciEnabled;

    //
    // Check the consistency of SCI enabling
    //

    //
    // Get SCI_EN value
    //
   if (Fadt->Pm1CntLen == 4) {
      SciEnabled = (IoRead32 (Fadt->Pm1aCntBlk) & BIT0)? TRUE : FALSE;
    } else {
      //
      // if (Pm1CntLen == 2), use 16 bit IO read;
      // if (Pm1CntLen != 2 && Pm1CntLen != 4), use 16 bit IO read as a fallback
      //
      SciEnabled = (IoRead16 (Fadt->Pm1aCntBlk) & BIT0)? TRUE : FALSE;
    }

    if (!(Fadt->Flags & EFI_ACPI_5_0_HW_REDUCED_ACPI) &&
        (Fadt->SmiCmd == 0) &&
       !SciEnabled) {
      //
      // The ACPI enabling status is inconsistent: SCI is not enabled but ACPI
      // table does not provide a means to enable it through FADT->SmiCmd
      //
      DEBUG ((DEBUG_ERROR, "ERROR: The ACPI enabling status is inconsistent: SCI is not"
        " enabled but the ACPI table does not provide a means to enable it through FADT->SmiCmd."
        " This may cause issues in OS.\n"));
    }
  DEBUG_CODE_END ();

  return RETURN_SUCCESS;
}


/**
  Entry point to the C language phase of UEFI payload.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
PayloadEntry (
  IN UINTN                     BootloaderParameter
  )
{
  EFI_STATUS                    Status;
  PHYSICAL_ADDRESS              DxeCoreEntryPoint;
  EFI_HOB_HANDOFF_INFO_TABLE    *HandoffHobTable;
  UINTN                         MemBase;
  UINTN                         MemSize;
  UINTN                         HobMemBase;
  UINTN                         HobMemTop;
  EFI_PEI_HOB_POINTERS          Hob;
  SERIAL_PORT_INFO              SerialPortInfo;



  Status = ParseSerialInfo (&SerialPortInfo);
  if (!EFI_ERROR (Status)) {
    CopyMem (GET_BOOTLOADER_PARAMETER_ADDR(3), &SerialPortInfo, sizeof(SERIAL_PORT_INFO));
  }

  // Call constructor for all libraries
  ProcessLibraryConstructorList ();
  DEBUG ((DEBUG_INFO, "GET_BOOTLOADER_PARAMETER() = 0x%lx\n", GET_BOOTLOADER_PARAMETER(1)));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof(UINTN)));


  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  // HOB region is used for HOB and memory allocation for this module
  MemBase    = PcdGet32 (PcdPayloadFdMemBase);
  HobMemBase = ALIGN_VALUE (MemBase + PcdGet32 (PcdPayloadFdMemSize), SIZE_1MB);
  HobMemTop  = HobMemBase + FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  // DXE core assumes the memory below HOB region could be used, so include the FV region memory into HOB range.
  MemSize    = HobMemTop - MemBase;
  HandoffHobTable = HobConstructor ((VOID *)MemBase, MemSize, (VOID *)HobMemBase, (VOID *)HobMemTop);

  // Build HOB based on information from Bootloader
  Status = BuildHobs ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BuildHobs Status = %r\n", Status));
    return Status;
  }

  // Load the DXE Core
  Status = LoadDxeCore (&DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "DxeCoreEntryPoint = 0x%lx\n", DxeCoreEntryPoint));

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE,  0xFF);

  Hob.HandoffInformationTable = HandoffHobTable;
  HandOffToDxeCore (DxeCoreEntryPoint, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
