/** @file

  Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

/**
   Callback function to build resource descriptor HOB

   This function build a HOB based on the memory map entry info.

   @param MemoryMapEntry         Memory map entry info got from bootloader.
   @param Params                 Not used for now.

  @retval RETURN_SUCCESS        Successfully build a HOB.
**/
EFI_STATUS
MemInfoCallback (
  IN MEMROY_MAP_ENTRY          *MemoryMapEntry,
  IN VOID                      *Params
  )
{
  EFI_PHYSICAL_ADDRESS         Base;
  EFI_RESOURCE_TYPE            Type;
  UINT64                       Size;
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribue;

  Type    = (MemoryMapEntry->Type == 1) ? EFI_RESOURCE_SYSTEM_MEMORY : EFI_RESOURCE_MEMORY_RESERVED;
  Base    = MemoryMapEntry->Base;
  Size    = MemoryMapEntry->Size;

  Attribue = EFI_RESOURCE_ATTRIBUTE_PRESENT |
             EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
             EFI_RESOURCE_ATTRIBUTE_TESTED |
             EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE;

  if (Base >= BASE_4GB ) {
    // Remove tested attribute to avoid DXE core to dispatch driver to memory above 4GB
    Attribue &= ~EFI_RESOURCE_ATTRIBUTE_TESTED;
  }

  BuildResourceDescriptorHob (Type, Attribue, (EFI_PHYSICAL_ADDRESS)Base, Size);
  DEBUG ((DEBUG_INFO , "buildhob: base = 0x%lx, size = 0x%lx, type = 0x%x\n", Base, Size, Type));

  return RETURN_SUCCESS;
}


/**
  It will build HOBs based on information from bootloaders.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildHobFromBl (
  VOID
  )
{
  EFI_STATUS                       Status;
  ACPI_TABLE_HOB                   AcpiTableHob;
  ACPI_TABLE_HOB                   *NewAcpiTableHob;
  SMBIOS_TABLE_HOB                 SmbiosTable;
  SMBIOS_TABLE_HOB                 *NewSmbiosTable;
  EFI_PEI_GRAPHICS_INFO_HOB        GfxInfo;
  EFI_PEI_GRAPHICS_INFO_HOB        *NewGfxInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB GfxDeviceInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB *NewGfxDeviceInfo;

  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK   *SmramHob;
  UINT32                           Size;
  UINT8                            Buffer[200];
  PLD_SMM_REGISTERS                *SmmRegisterHob;
  VOID                             *NewHob;
  UINT32                           Index;
  PLD_S3_COMMUNICATION             *PldS3Info;
  SPI_FLASH_INFO                   SpiFlashInfo;
  SPI_FLASH_INFO                   *NewSpiFlashInfo;
  NV_VARIABLE_INFO                 NvVariableInfo;
  NV_VARIABLE_INFO                 *NewNvVariableInfo;

  //
  // Parse memory info and build memory HOBs
  //
  Status = ParseMemoryInfo (MemInfoCallback, NULL);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Create guid hob for frame buffer information
  //
  Status = ParseGfxInfo (&GfxInfo);
  if (!EFI_ERROR (Status)) {
    NewGfxInfo = BuildGuidHob (&gEfiGraphicsInfoHobGuid, sizeof (GfxInfo));
    ASSERT (NewGfxInfo != NULL);
    CopyMem (NewGfxInfo, &GfxInfo, sizeof (GfxInfo));
    DEBUG ((DEBUG_INFO, "Created graphics info hob\n"));
  }


  Status = ParseGfxDeviceInfo (&GfxDeviceInfo);
  if (!EFI_ERROR (Status)) {
    NewGfxDeviceInfo = BuildGuidHob (&gEfiGraphicsDeviceInfoHobGuid, sizeof (GfxDeviceInfo));
    ASSERT (NewGfxDeviceInfo != NULL);
    CopyMem (NewGfxDeviceInfo, &GfxDeviceInfo, sizeof (GfxDeviceInfo));
    DEBUG ((DEBUG_INFO, "Created graphics device info hob\n"));
  }


  //
  // Create smbios table guid hob
  //
  Status = ParseSmbiosTable(&SmbiosTable);
  if (!EFI_ERROR (Status)) {
    NewSmbiosTable = BuildGuidHob (&gEfiSmbiosTableGuid, sizeof (SmbiosTable));
    ASSERT (NewSmbiosTable != NULL);
    CopyMem (NewSmbiosTable, &SmbiosTable, sizeof (SmbiosTable));
    DEBUG ((DEBUG_INFO, "Detected Smbios Table at 0x%lx\n", SmbiosTable.TableAddress));
  }

  //
  // Create acpi table guid hob
  //
  Status = ParseAcpiTableInfo(&AcpiTableHob);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    NewAcpiTableHob = BuildGuidHob (&gEfiAcpiTableGuid, sizeof (AcpiTableHob));
    ASSERT (NewAcpiTableHob != NULL);
    CopyMem (NewAcpiTableHob, &AcpiTableHob, sizeof (AcpiTableHob));
    DEBUG ((DEBUG_INFO, "Detected ACPI Table at 0x%lx\n", AcpiTableHob.TableAddress));
  }

  //
  // Create SMRAM information HOB
  //
  SmramHob = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)Buffer;
  Size     = sizeof (Buffer);
  Status = GetSmramInfo (SmramHob, &Size);
  DEBUG((DEBUG_INFO, "GetSmramInfo = %r, data Size = 0x%x\n", Status, Size));
  if (!EFI_ERROR (Status)) {
    NewHob = BuildGuidHob (&gEfiSmmSmramMemoryGuid, Size);
    ASSERT (NewHob != NULL);
    CopyMem (NewHob, SmramHob, Size);
    DEBUG((DEBUG_INFO, "Region count = 0x%x\n", SmramHob->NumberOfSmmReservedRegions));
    for (Index = 0; Index < SmramHob->NumberOfSmmReservedRegions; Index++ ) {
      DEBUG((DEBUG_INFO, "CpuStart[%d] = 0x%lx\n", Index, SmramHob->Descriptor[Index].CpuStart));
      DEBUG((DEBUG_INFO, "base[%d]     = 0x%lx\n", Index, SmramHob->Descriptor[Index].PhysicalStart));
      DEBUG((DEBUG_INFO, "size[%d]     = 0x%lx\n", Index, SmramHob->Descriptor[Index].PhysicalSize));
      DEBUG((DEBUG_INFO, "State[%d]    = 0x%lx\n", Index, SmramHob->Descriptor[Index].RegionState));
    }
  }

  //
  // Create SMM register information HOB
  //
  SmmRegisterHob = (PLD_SMM_REGISTERS *)Buffer;
  Size           = sizeof (Buffer);
  Status = GetSmmRegisterInfo (SmmRegisterHob, &Size);
  DEBUG((DEBUG_INFO, "GetSmmRegisterInfo = %r, data Size = 0x%x\n", Status, Size));
  if (!EFI_ERROR (Status)) {
    NewHob = BuildGuidHob (&gPldSmmRegisterInfoGuid, Size);
    ASSERT (NewHob != NULL);
    CopyMem (NewHob, SmramHob, Size);
    DEBUG((DEBUG_INFO, "SMM register count = 0x%x\n", SmmRegisterHob->Count));
    for (Index = 0; Index < SmmRegisterHob->Count; Index++ ) {
      DEBUG((DEBUG_INFO, "ID[%d]            = 0x%lx\n", Index, SmmRegisterHob->Registers[Index].Id));
      DEBUG((DEBUG_INFO, "Value[%d]         = 0x%lx\n", Index, SmmRegisterHob->Registers[Index].Value));
      DEBUG((DEBUG_INFO, "AddressSpaceId    = 0x%x\n",  SmmRegisterHob->Registers[Index].Address.AddressSpaceId));
      DEBUG((DEBUG_INFO, "RegisterBitWidth  = 0x%x\n",  SmmRegisterHob->Registers[Index].Address.RegisterBitWidth));
      DEBUG((DEBUG_INFO, "RegisterBitOffset = 0x%x\n",  SmmRegisterHob->Registers[Index].Address.RegisterBitOffset));
      DEBUG((DEBUG_INFO, "AccessSize        = 0x%x\n",  SmmRegisterHob->Registers[Index].Address.AccessSize));
      DEBUG((DEBUG_INFO, "Address           = 0x%lx\n", SmmRegisterHob->Registers[Index].Address.Address));
    }
  }

  //
  // Create SMM S3 information HOB
  //
  PldS3Info = (PLD_S3_COMMUNICATION *)Buffer;
  Size      = sizeof (Buffer);
  Status = GetPldS3CommunicationInfo (PldS3Info, &Size);
  DEBUG((DEBUG_INFO, "GetPldS3Info = %r, data Size = 0x%x\n", Status, Size));
  if (!EFI_ERROR (Status)) {
    NewHob = BuildGuidHob (&gPldS3CommunicationGuid, Size);
    ASSERT (NewHob != NULL);
    CopyMem (NewHob, SmramHob, Size);
    PldS3Info = (PLD_S3_COMMUNICATION *)NewHob;
  }

  //
  // Create a HOB for SPI flash info
  //
  Status = ParseSpiFlashInfo(&SpiFlashInfo);
  if (!EFI_ERROR (Status)) {
    NewSpiFlashInfo = BuildGuidHob (&gSpiFlashInfoGuid, sizeof (SPI_FLASH_INFO));
    ASSERT (NewSpiFlashInfo != NULL);
    CopyMem (NewSpiFlashInfo, &SpiFlashInfo, sizeof (SPI_FLASH_INFO));
    DEBUG ((DEBUG_INFO, "Flags=0x%x, SPI PCI Base=0x%lx\n", SpiFlashInfo.Flags, SpiFlashInfo.SpiAddress.Address));
  }

  //
  // Create a hob for NV variable
  //
  Status = ParseNvVariableInfo(&NvVariableInfo);
  if (!EFI_ERROR (Status)) {
    NewNvVariableInfo = BuildGuidHob (&gNvVariableInfoGuid, sizeof (NV_VARIABLE_INFO));
    ASSERT (NewNvVariableInfo != NULL);
    CopyMem (NewNvVariableInfo, &NvVariableInfo, sizeof (NV_VARIABLE_INFO));
    DEBUG ((DEBUG_INFO, "VarStoreBase=0x%x, length=0x%x\n", NvVariableInfo.VariableStoreBase, NvVariableInfo.VariableStoreSize));
  }

  //
  // Parse platform specific information.
  //
  Status = ParsePlatformInfo ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error when parsing platform info, Status = %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  This function will build some generic HOBs that doesn't depend on information from bootloaders.

**/
VOID
BuildGenericHob (
  VOID
  )
{
  UINT32                           RegEax;
  UINT8                            PhysicalAddressBits;
  EFI_RESOURCE_ATTRIBUTE_TYPE      ResourceAttribute;

  // The UEFI payload FV
  BuildMemoryAllocationHob (PcdGet32 (PcdPayloadFdMemBase), PcdGet32 (PcdPayloadFdMemSize), EfiBootServicesData);

  //
  // Build CPU memory space and IO space hob
  //
  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    PhysicalAddressBits = (UINT8) RegEax;
  } else {
    PhysicalAddressBits  = 36;
  }

  BuildCpuHob (PhysicalAddressBits, 16);

  //
  // Report Local APIC range, cause sbl HOB to be NULL, comment now
  //
  ResourceAttribute = (
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED
  );
  BuildResourceDescriptorHob (EFI_RESOURCE_MEMORY_MAPPED_IO, ResourceAttribute, 0xFEC80000, SIZE_512KB);
  BuildMemoryAllocationHob ( 0xFEC80000, SIZE_512KB, EfiMemoryMappedIO);

}


/**
  This function will build HOBs.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildHobs (
  VOID
  )
{
  EFI_STATUS Status;

  Status = BuildHobFromBl ();
  if (EFI_ERROR (Status)) {
    return Status;
  }
  BuildGenericHob ();
  return EFI_SUCCESS;
}

