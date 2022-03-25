/** @file
  OVMF ACPI support using QEMU's fw-cfg interface

  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2012-2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "QemuLoader.h"

#define EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('F', 'A', 'C', 'P')
#define INSTALLED_TABLES_MAX                                 128

#pragma pack(1)
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  EFI_PHYSICAL_ADDRESS           Tables[4];
} XSDT_TABLE_STRUCT;
#pragma pack()

//
// Condensed structure for capturing the fw_cfg operations -- select, skip,
// write -- inherent in executing a QEMU_LOADER_WRITE_POINTER command.
//
typedef struct {
  UINT16    PointerItem;   // resolved from QEMU_LOADER_WRITE_POINTER.PointerFile
  UINT8     PointerSize;   // copied as-is from QEMU_LOADER_WRITE_POINTER
  UINT32    PointerOffset; // copied as-is from QEMU_LOADER_WRITE_POINTER
  UINT64    PointerValue;  // resolved from QEMU_LOADER_WRITE_POINTER.PointeeFile
                           //   and QEMU_LOADER_WRITE_POINTER.PointeeOffset
} CONDENSED_WRITE_POINTER;

//
// Context structure to accumulate CONDENSED_WRITE_POINTER objects from
// QEMU_LOADER_WRITE_POINTER commands.
//
// Any pointers in this structure own the pointed-to objects; that is, when the
// context structure is released, all pointed-to objects must be released too.
//
struct S3_CONTEXT {
  CONDENSED_WRITE_POINTER    *WritePointers; // one array element per processed
                                             //   QEMU_LOADER_WRITE_POINTER
                                             //   command
  UINTN                      Allocated;      // number of elements allocated for
                                             //   WritePointers
  UINTN                      Used;           // number of elements populated in
                                             //   WritePointers
};

typedef struct {
  UINT8      File[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated name of the fw_cfg
                                           // blob. This is the ordering / search
                                           // key.
  UINTN      Size;                         // The number of bytes in this blob.
  UINT8      *Base;                        // Pointer to the blob data.
  BOOLEAN    HostsOnlyTableData;           // TRUE iff the blob has been found to
                                           // only contain data that is directly
                                           // part of ACPI tables.
} BLOB;

STATIC
INTN
EFIAPI
AsciiStringCompare (
  IN CONST VOID  *AsciiString1,
  IN CONST VOID  *AsciiString2
  )
{
  return AsciiStrCmp (AsciiString1, AsciiString2);
}

STATIC
EFI_STATUS
CollectAllocationsRestrictedTo32Bit (
  OUT ORDERED_COLLECTION      **AllocationsRestrictedTo32Bit,
  IN CONST QEMU_LOADER_ENTRY  *LoaderStart,
  IN CONST QEMU_LOADER_ENTRY  *LoaderEnd
  )
{
  ORDERED_COLLECTION       *Collection;
  CONST QEMU_LOADER_ENTRY  *LoaderEntry;
  EFI_STATUS               Status;

  Collection = OrderedCollectionInit (AsciiStringCompare, AsciiStringCompare);
  if (Collection == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (LoaderEntry = LoaderStart; LoaderEntry < LoaderEnd; ++LoaderEntry) {
    CONST QEMU_LOADER_ADD_POINTER  *AddPointer;

    if (LoaderEntry->Type != QemuLoaderCmdAddPointer) {
      continue;
    }

    AddPointer = &LoaderEntry->Command.AddPointer;

    if (AddPointer->PointerSize >= 8) {
      continue;
    }

    if (AddPointer->PointeeFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0') {
      DEBUG ((DEBUG_ERROR, "%a: malformed file name\n", __FUNCTION__));
      Status = EFI_PROTOCOL_ERROR;
    }

    Status = OrderedCollectionInsert (
               Collection,
               NULL,                           // Entry
               (VOID *)AddPointer->PointeeFile
               );
    switch (Status) {
      case EFI_SUCCESS:
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: restricting blob \"%a\" from 64-bit allocation\n",
          __FUNCTION__,
          AddPointer->PointeeFile
          ));
        break;
      case EFI_ALREADY_STARTED:
        //
        // The restriction has been recorded already.
        //
        break;
      case EFI_OUT_OF_RESOURCES:
      default:
        ASSERT (FALSE);
    }
  }

  *AllocationsRestrictedTo32Bit = Collection;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ProcessCmdAllocate (
  IN CONST QEMU_LOADER_ALLOCATE  *Allocate,
  IN OUT ORDERED_COLLECTION      *Tracker,
  IN ORDERED_COLLECTION          *AllocationsRestrictedTo32Bit
  )
{
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  EFI_STATUS            Status;
  UINTN                 NumPages;
  EFI_PHYSICAL_ADDRESS  Address;
  BLOB                  *Blob;

  DEBUG ((DEBUG_ERROR, "%a:%d\n", __FILE__, __LINE__));
  if (Allocate->File[QEMU_LOADER_FNAME_SIZE - 1] != '\0') {
    DEBUG ((DEBUG_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  if (Allocate->Alignment > EFI_PAGE_SIZE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: unsupported alignment 0x%x\n",
      __FUNCTION__,
      Allocate->Alignment
      ));
    return EFI_UNSUPPORTED;
  }

  Status = QemuFwCfgFindFile ((CHAR8 *)Allocate->File, &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: QemuFwCfgFindFile(\"%a\"): %r\n",
      __FUNCTION__,
      Allocate->File,
      Status
      ));
    return Status;
  }

  NumPages = EFI_SIZE_TO_PAGES (FwCfgSize);
  Address  = MAX_UINT64;
  if (OrderedCollectionFind (
        AllocationsRestrictedTo32Bit,
        Allocate->File
        ) != NULL)
  {
    Address = MAX_UINT32;
  }

  Status = PeiServicesAllocatePages (
             EfiACPIMemoryNVS,
             NumPages,
             &Address
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PeiServicesAllocatePool (sizeof *Blob, (void **)&Blob);
  if (Blob == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreePages;
  }

  CopyMem (Blob->File, Allocate->File, QEMU_LOADER_FNAME_SIZE);
  Blob->Size               = FwCfgSize;
  Blob->Base               = (VOID *)(UINTN)Address;
  Blob->HostsOnlyTableData = TRUE;

  Status = OrderedCollectionInsert (Tracker, NULL, Blob);
  if (Status == RETURN_ALREADY_STARTED) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: duplicated file \"%a\"\n",
      __FUNCTION__,
      Allocate->File
      ));
    Status = EFI_PROTOCOL_ERROR;
  }

  if (EFI_ERROR (Status)) {
    goto FreeBlob;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, Blob->Base);
  ZeroMem (Blob->Base + Blob->Size, EFI_PAGES_TO_SIZE (NumPages) - Blob->Size);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: File=\"%a\" Alignment=0x%x Zone=%d Size=0x%Lx "
    "Address=0x%Lx\n",
    __FUNCTION__,
    Allocate->File,
    Allocate->Alignment,
    Allocate->Zone,
    (UINT64)Blob->Size,
    (UINT64)(UINTN)Blob->Base
    ));
  return EFI_SUCCESS;

FreeBlob:
  FreePool (Blob);

FreePages:
  PeiServicesFreePages (Address, NumPages);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
ProcessCmdAddPointer (
  IN CONST QEMU_LOADER_ADD_POINTER  *AddPointer,
  IN CONST ORDERED_COLLECTION       *Tracker
  )
{
  ORDERED_COLLECTION_ENTRY  *TrackerEntry, *TrackerEntry2;
  BLOB                      *Blob, *Blob2;
  UINT8                     *PointerField;
  UINT64                    PointerValue;

  if ((AddPointer->PointerFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0') ||
      (AddPointer->PointeeFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0'))
  {
    DEBUG ((DEBUG_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  TrackerEntry  = OrderedCollectionFind (Tracker, AddPointer->PointerFile);
  TrackerEntry2 = OrderedCollectionFind (Tracker, AddPointer->PointeeFile);
  if ((TrackerEntry == NULL) || (TrackerEntry2 == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid blob reference(s) \"%a\" / \"%a\"\n",
      __FUNCTION__,
      AddPointer->PointerFile,
      AddPointer->PointeeFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  Blob  = OrderedCollectionUserStruct (TrackerEntry);
  Blob2 = OrderedCollectionUserStruct (TrackerEntry2);
  if (((AddPointer->PointerSize != 1) && (AddPointer->PointerSize != 2) &&
       (AddPointer->PointerSize != 4) && (AddPointer->PointerSize != 8)) ||
      (Blob->Size < AddPointer->PointerSize) ||
      (Blob->Size - AddPointer->PointerSize < AddPointer->PointerOffset))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid pointer location or size in \"%a\"\n",
      __FUNCTION__,
      AddPointer->PointerFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  PointerField = Blob->Base + AddPointer->PointerOffset;
  PointerValue = 0;
  CopyMem (&PointerValue, PointerField, AddPointer->PointerSize);
  if (PointerValue >= Blob2->Size) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid pointer value in \"%a\"\n",
      __FUNCTION__,
      AddPointer->PointerFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ProcessCmdAddChecksum (
  IN CONST QEMU_LOADER_ADD_CHECKSUM  *AddChecksum,
  IN CONST ORDERED_COLLECTION        *Tracker
  )
{
  ORDERED_COLLECTION_ENTRY  *TrackerEntry;
  BLOB                      *Blob;

  if (AddChecksum->File[QEMU_LOADER_FNAME_SIZE - 1] != '\0') {
    DEBUG ((DEBUG_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  TrackerEntry = OrderedCollectionFind (Tracker, AddChecksum->File);
  if (TrackerEntry == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid blob reference \"%a\"\n",
      __FUNCTION__,
      AddChecksum->File
      ));
    return EFI_PROTOCOL_ERROR;
  }

  Blob = OrderedCollectionUserStruct (TrackerEntry);
  if ((Blob->Size <= AddChecksum->ResultOffset) ||
      (Blob->Size < AddChecksum->Length) ||
      (Blob->Size - AddChecksum->Length < AddChecksum->Start))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid checksum range in \"%a\"\n",
      __FUNCTION__,
      AddChecksum->File
      ));
    return EFI_PROTOCOL_ERROR;
  }

  Blob->Base[AddChecksum->ResultOffset] = CalculateCheckSum8 (
                                            Blob->Base + AddChecksum->Start,
                                            AddChecksum->Length
                                            );
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: File=\"%a\" ResultOffset=0x%x Start=0x%x "
    "Length=0x%x\n",
    __FUNCTION__,
    AddChecksum->File,
    AddChecksum->ResultOffset,
    AddChecksum->Start,
    AddChecksum->Length
    ));
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ProcessCmdWritePointer (
  IN     CONST QEMU_LOADER_WRITE_POINTER  *WritePointer,
  IN     CONST ORDERED_COLLECTION         *Tracker
  )
{
  RETURN_STATUS             Status;
  FIRMWARE_CONFIG_ITEM      PointerItem;
  UINTN                     PointerItemSize;
  ORDERED_COLLECTION_ENTRY  *PointeeEntry;
  BLOB                      *PointeeBlob;
  UINT64                    PointerValue;

  DEBUG ((DEBUG_ERROR, "%a:%d\n", __FILE__, __LINE__));
  if ((WritePointer->PointerFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0') ||
      (WritePointer->PointeeFile[QEMU_LOADER_FNAME_SIZE - 1] != '\0'))
  {
    DEBUG ((DEBUG_ERROR, "%a: malformed file name\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  Status = QemuFwCfgFindFile (
             (CONST CHAR8 *)WritePointer->PointerFile,
             &PointerItem,
             &PointerItemSize
             );
  PointeeEntry = OrderedCollectionFind (Tracker, WritePointer->PointeeFile);
  if (RETURN_ERROR (Status) || (PointeeEntry == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid fw_cfg file or blob reference \"%a\" / \"%a\"\n",
      __FUNCTION__,
      WritePointer->PointerFile,
      WritePointer->PointeeFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  if (((WritePointer->PointerSize != 1) && (WritePointer->PointerSize != 2) &&
       (WritePointer->PointerSize != 4) && (WritePointer->PointerSize != 8)) ||
      (PointerItemSize < WritePointer->PointerSize) ||
      (PointerItemSize - WritePointer->PointerSize <
       WritePointer->PointerOffset))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid pointer location or size in \"%a\"\n",
      __FUNCTION__,
      WritePointer->PointerFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  PointeeBlob  = OrderedCollectionUserStruct (PointeeEntry);
  PointerValue = WritePointer->PointeeOffset;
  if (PointerValue >= PointeeBlob->Size) {
    DEBUG ((DEBUG_ERROR, "%a: invalid PointeeOffset\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  //
  // The memory allocation system ensures that the address of the byte past the
  // last byte of any allocated object is expressible (no wraparound).
  //
  ASSERT ((UINTN)PointeeBlob->Base <= MAX_ADDRESS - PointeeBlob->Size);

  PointerValue += (UINT64)(UINTN)PointeeBlob->Base;
  if ((WritePointer->PointerSize < 8) &&
      (RShiftU64 (PointerValue, WritePointer->PointerSize * 8) != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: pointer value unrepresentable in \"%a\"\n",
      __FUNCTION__,
      WritePointer->PointerFile
      ));
    return EFI_PROTOCOL_ERROR;
  }

  QemuFwCfgSelectItem (PointerItem);
  QemuFwCfgSkipBytes (WritePointer->PointerOffset);
  QemuFwCfgWriteBytes (WritePointer->PointerSize, &PointerValue);

  //
  // Because QEMU has now learned PointeeBlob->Base, we must mark PointeeBlob
  // as unreleasable, for the case when the whole linker/loader script is
  // handled successfully.
  //
  PointeeBlob->HostsOnlyTableData = FALSE;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: PointerFile=\"%a\" PointeeFile=\"%a\" "
    "PointerOffset=0x%x PointeeOffset=0x%x PointerSize=%d\n",
    __FUNCTION__,
    WritePointer->PointerFile,
    WritePointer->PointeeFile,
    WritePointer->PointerOffset,
    WritePointer->PointeeOffset,
    WritePointer->PointerSize
    ));
  return EFI_SUCCESS;
}

VOID
ParseAcpi (
  IN UINT8  *Acpibase
  )
{
  EFI_ACPI_DESCRIPTION_HEADER                                                            *Header;
  EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE                                              *Fadt;
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER                         *MmCfgHdr;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE  *MmCfgBase;
  EFI_ACPI_DESCRIPTION_HEADER                                                            *Dsdt;
  EFI_ACPI_DESCRIPTION_HEADER                                                            *Rsdt;
  EFI_ACPI_6_3_FIRMWARE_ACPI_CONTROL_STRUCTURE                                           *Facs;
  EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER                                    *Apic;
  EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER                                       *Hpet;
  EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER                                           *Rsdp;
  XSDT_TABLE_STRUCT                                                                      *Xsdt;
  UNIVERSAL_PAYLOAD_ACPI_TABLE                                                           *AcpiTableHob;
  UINT32                                                                                 NodeCount;
  EFI_PHYSICAL_ADDRESS                                                                   PhyAddr;
  EFI_STATUS                                                                             Status;

  Status = PeiServicesAllocatePages (
             EfiACPIReclaimMemory,
             EFI_SIZE_TO_PAGES (
               sizeof (EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE) +
               sizeof (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER) +
               sizeof (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE)+
               sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
               sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
               sizeof (EFI_ACPI_6_3_FIRMWARE_ACPI_CONTROL_STRUCTURE)+
               sizeof (EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER)+
               sizeof (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER)
               ) +
             sizeof (XSDT_TABLE_STRUCT) +
             sizeof (EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER),
             &PhyAddr
             );
  ASSERT_EFI_ERROR (Status);

  Fadt     = (EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE *)(UINTN)PhyAddr;
  Dsdt     = (EFI_ACPI_DESCRIPTION_HEADER *)(Fadt + 1);
  Facs     = (EFI_ACPI_6_3_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(Dsdt + 1);
  Apic     = (EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)(Facs + 1);
  Hpet     = (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER *)(Apic + 1);
  Xsdt     = (XSDT_TABLE_STRUCT *)(Hpet + 1);
  Rsdp     = (EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER *)(Xsdt + 1);
  Rsdt     = (EFI_ACPI_DESCRIPTION_HEADER *)(Rsdp + 1);
  MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)(Rsdt + 1);

  Header = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Acpibase;

  for (int i = 0; i < 7; i++) {
    DEBUG ((DEBUG_ERROR, "Acpi table Signature:\"%-4.4a\" \n", &Header->Signature));
    if (Header->Signature == EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
      CopyMem (Fadt, Header, sizeof (*Fadt));
    }

    if (Header->Signature == EFI_ACPI_6_3_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
      CopyMem (MmCfgHdr, Header, (sizeof (*MmCfgHdr)+sizeof (*MmCfgBase)));
      MmCfgBase = (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *)((UINT8 *)MmCfgHdr + sizeof (*MmCfgHdr));
      NodeCount = (MmCfgHdr->Header.Length - sizeof (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER)) /
                  sizeof (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE);
    }

    if (Header->Signature == EFI_ACPI_6_3_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) {
      CopyMem (Facs, Header, sizeof (*Facs));
    }

    if (Header->Signature == EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
      CopyMem (Dsdt, Header, sizeof (*Dsdt));
    }

    if (Header->Signature == EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE) {
      CopyMem (Apic, Header, sizeof (*Apic));
    }

    if (Header->Signature == EFI_ACPI_6_3_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE) {
      CopyMem (Hpet, Header, sizeof (*Hpet));
    }

    if (Header->Signature == EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
      CopyMem (Dsdt, Header, sizeof (*Dsdt));
    }

    if (Header->Signature == EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
      CopyMem (Rsdt, Header, sizeof (*Rsdt));
    }

    Header = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)Header + Header->Length);
  }

  Rsdp->Signature = EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE;
  CopyMem (&Rsdp->OemId, &Fadt->Header.OemId, sizeof (Fadt->Header.OemId));
  Rsdp->Revision    = 0x02;
  Rsdp->Length      = sizeof (EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER);
  Rsdp->RsdtAddress = (UINT32)Rsdt;
  Rsdp->XsdtAddress = (UINT64)Xsdt;
  SetMem (Rsdp->Reserved, sizeof (Rsdp->Reserved), EFI_ACPI_RESERVED_BYTE);
  Rsdp->Checksum         = 0;
  Rsdp->Checksum         = CalculateCheckSum8 ((UINT8 *)Rsdp, 20);
  Rsdp->ExtendedChecksum = CalculateCheckSum8 ((UINT8 *)Rsdp, Rsdp->Length);

  Xsdt->Header.Signature = EFI_ACPI_6_3_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE;
  Xsdt->Header.Length    = sizeof (XSDT_TABLE_STRUCT);
  CopyMem (&Xsdt->Header.OemId, &Fadt->Header.OemId, sizeof (Fadt->Header.OemId));
  CopyMem (&Xsdt->Header.OemTableId, &Fadt->Header.OemTableId, sizeof (Fadt->Header.OemTableId));
  Xsdt->Header.Revision        = 0x01;
  Xsdt->Header.OemRevision     = 0x1;
  Xsdt->Header.Length          = sizeof (XSDT_TABLE_STRUCT);
  Xsdt->Header.CreatorRevision = 0x1000013;
  Xsdt->Header.Checksum        = 0;
  Xsdt->Header.Checksum        = CalculateCheckSum8 ((UINT8 *)Xsdt, Xsdt->Header.Length);
  Xsdt->Tables[0]              = (EFI_PHYSICAL_ADDRESS)Fadt;
  Xsdt->Tables[1]              = (EFI_PHYSICAL_ADDRESS)Apic;
  Xsdt->Tables[2]              = (EFI_PHYSICAL_ADDRESS)Hpet;
  Xsdt->Tables[3]              = (EFI_PHYSICAL_ADDRESS)MmCfgHdr;

  Fadt->Dsdt                    = (UINT32)Dsdt;
  Fadt->XDsdt                   = (UINT64)Dsdt;
  Fadt->FirmwareCtrl            = (UINT32)Facs;
  AcpiTableHob                  = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)BuildGuidHob (&gUniversalPayloadAcpiTableGuid, sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE));
  AcpiTableHob->Rsdp            = (EFI_PHYSICAL_ADDRESS)Rsdp;
  AcpiTableHob->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE);
  AcpiTableHob->Header.Revision = UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION;
  //
  // Printing FADT table Values.
  //
  DEBUG ((DEBUG_INFO, "FADT \n"));
  DEBUG ((DEBUG_INFO, "      Signature             = %c%c%c%c\n", ((CHAR8 *)&Fadt->Header.Signature)[0], ((CHAR8 *)&Fadt->Header.Signature)[1], ((CHAR8 *)&Fadt->Header.Signature)[2], ((CHAR8 *)&Fadt->Header.Signature)[3]));
  DEBUG ((DEBUG_INFO, "      Length                = %d\n", Fadt->Header.Length));
  DEBUG ((DEBUG_INFO, "      Revision              = 0x%x\n", Fadt->Header.Revision));
  DEBUG ((DEBUG_INFO, "      Checksum              = 0x%x\n", Fadt->Header.Checksum));
  DEBUG ((DEBUG_INFO, "      OemId                 = %c%c%c%c%c\n", Fadt->Header.OemId[0], Fadt->Header.OemId[1], Fadt->Header.OemId[2], Fadt->Header.OemId[3], Fadt->Header.OemId[4], Fadt->Header.OemId[5]));
  DEBUG ((
    DEBUG_INFO,
    "      OemTableId            = %c%c%c%c%c%c%c%c\n",
    ((CHAR8 *)&Fadt->Header.OemTableId)[0],
    ((CHAR8 *)&Fadt->Header.OemTableId)[1],
    ((CHAR8 *)&Fadt->Header.OemTableId)[2],
    ((CHAR8 *)&Fadt->Header.OemTableId)[3],
    ((CHAR8 *)&Fadt->Header.OemTableId)[4],
    ((CHAR8 *)&Fadt->Header.OemTableId)[5],
    ((CHAR8 *)&Fadt->Header.OemTableId)[6],
    ((CHAR8 *)&Fadt->Header.OemTableId)[7]
    ));
  DEBUG ((DEBUG_INFO, "      OemRevision           = 0x%x\n", Fadt->Header.OemRevision));
  DEBUG ((DEBUG_INFO, "      CreatorId             = %c%c%c%c\n", ((CHAR8 *)&Fadt->Header.CreatorId)[0], ((CHAR8 *)&Fadt->Header.CreatorId)[1], ((CHAR8 *)&Fadt->Header.CreatorId)[2], ((CHAR8 *)&Fadt->Header.CreatorId)[3]));
  DEBUG ((DEBUG_INFO, "      CreatorRevision       = 0x%x\n", Fadt->Header.CreatorRevision));
  DEBUG ((DEBUG_INFO, "      FirmwareCtrl          = 0x%x\n", Fadt->FirmwareCtrl));
  DEBUG ((DEBUG_INFO, "      Dsdt                  = 0x%x\n", Fadt->Dsdt));
  DEBUG ((DEBUG_INFO, "      Reserved0             = 0x%x\n", Fadt->Reserved0));
  DEBUG ((DEBUG_INFO, "      PreferredPmProfile    = 0x%x\n", Fadt->PreferredPmProfile));
  DEBUG ((DEBUG_INFO, "      SciInt                = 0x%x\n", Fadt->SciInt));
  DEBUG ((DEBUG_INFO, "      SmiCmd                = 0x%x\n", Fadt->SmiCmd));
  DEBUG ((DEBUG_INFO, "      AcpiEnable            = 0x%x\n", Fadt->AcpiEnable));
  DEBUG ((DEBUG_INFO, "      AcpiDisable           = 0x%x\n", Fadt->AcpiDisable));
  DEBUG ((DEBUG_INFO, "      S4BiosReq             = 0x%x\n", Fadt->S4BiosReq));
  DEBUG ((DEBUG_INFO, "      PstateCnt             = 0x%x\n", Fadt->PstateCnt));
  DEBUG ((DEBUG_INFO, "      Pm1aEvtBlk            = 0x%x\n", Fadt->Pm1aEvtBlk));
  DEBUG ((DEBUG_INFO, "      Pm1bEvtBlk            = 0x%x\n", Fadt->Pm1bEvtBlk));
  DEBUG ((DEBUG_INFO, "      Pm1aCntBlk            = 0x%x\n", Fadt->Pm1aCntBlk));
  DEBUG ((DEBUG_INFO, "      Pm1bCntBlk            = 0x%x\n", Fadt->Pm1bCntBlk));
  DEBUG ((DEBUG_INFO, "      Pm2CntBlk             = 0x%x\n", Fadt->Pm2CntBlk));
  DEBUG ((DEBUG_INFO, "      PmTmrBlk              = 0x%x\n", Fadt->PmTmrBlk));
  DEBUG ((DEBUG_INFO, "      Gpe0Blk               = 0x%x\n", Fadt->Gpe0Blk));
  DEBUG ((DEBUG_INFO, "      Gpe1Blk               = 0x%x\n", Fadt->Gpe1Blk));
  DEBUG ((DEBUG_INFO, "      Pm1EvtLen             = 0x%x\n", Fadt->Pm1EvtLen));
  DEBUG ((DEBUG_INFO, "      Pm1CntLen             = 0x%x\n", Fadt->Pm1CntLen));
  DEBUG ((DEBUG_INFO, "      Pm2CntLen             = 0x%x\n", Fadt->Pm2CntLen));
  DEBUG ((DEBUG_INFO, "      PmTmrLen              = 0x%x\n", Fadt->PmTmrLen));
  DEBUG ((DEBUG_INFO, "      Gpe0BlkLen            = 0x%x\n", Fadt->Gpe0BlkLen));
  DEBUG ((DEBUG_INFO, "      Gpe1BlkLen            = 0x%x\n", Fadt->Gpe1BlkLen));
  DEBUG ((DEBUG_INFO, "      Gpe1Base              = 0x%x\n", Fadt->Gpe1Base));
  DEBUG ((DEBUG_INFO, "      CstCnt                = 0x%x\n", Fadt->CstCnt));
  DEBUG ((DEBUG_INFO, "      PLvl2Lat              = 0x%x\n", Fadt->PLvl2Lat));
  DEBUG ((DEBUG_INFO, "      PLvl3Lat              = 0x%x\n", Fadt->PLvl3Lat));
  DEBUG ((DEBUG_INFO, "      FlushSize             = 0x%x\n", Fadt->FlushSize));
  DEBUG ((DEBUG_INFO, "      FlushStride           = 0x%x\n", Fadt->FlushStride));
  DEBUG ((DEBUG_INFO, "      DutyOffset            = 0x%x\n", Fadt->DutyOffset));
  DEBUG ((DEBUG_INFO, "      DutyWidth             = 0x%x\n", Fadt->DutyWidth));
  DEBUG ((DEBUG_INFO, "      DayAlrm               = 0x%x\n", Fadt->DayAlrm));
  DEBUG ((DEBUG_INFO, "      MonAlrm               = 0x%x\n", Fadt->MonAlrm));
  DEBUG ((DEBUG_INFO, "      Century               = 0x%x\n", Fadt->Century));
  DEBUG ((DEBUG_INFO, "      IaPcBootArch          = 0x%x\n", Fadt->IaPcBootArch));
  DEBUG ((DEBUG_INFO, "      Reserved1             = 0x%x\n", Fadt->Reserved1));
  DEBUG ((DEBUG_INFO, "      Flags                 = 0x%x\n", Fadt->Flags));
  DEBUG ((DEBUG_INFO, "Fadt  ResetReg \n"));
  DEBUG ((DEBUG_INFO, "      AddressSpaceId        = 0x%x\n", Fadt->ResetReg.AddressSpaceId));
  DEBUG ((DEBUG_INFO, "      RegisterBitWidth      = 0x%x\n", Fadt->ResetReg.RegisterBitWidth));
  DEBUG ((DEBUG_INFO, "      RegisterBitOffset     = 0x%x\n", Fadt->ResetReg.RegisterBitOffset));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->ResetReg.AccessSize));
  DEBUG ((DEBUG_INFO, "      Address               = 0x%x\n", Fadt->ResetReg.Address));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->ResetReg.AccessSize));
  DEBUG ((DEBUG_INFO, "      ResetValue            = 0x%x\n", Fadt->ResetValue));
  DEBUG ((DEBUG_INFO, "      ArmBootArch           = 0x%x\n", Fadt->ArmBootArch));
  DEBUG ((DEBUG_INFO, "      MinorVersion          = 0x%x\n", Fadt->MinorVersion));
  DEBUG ((DEBUG_INFO, "      XFirmwareCtrl         = 0x%lx\n", Fadt->XFirmwareCtrl));
  DEBUG ((DEBUG_INFO, "      XDsdt                 = 0x%lx\n", Fadt->XDsdt));
  DEBUG ((DEBUG_INFO, "      XPm1aEvtBlk \n"));
  DEBUG ((DEBUG_INFO, "      AddressSpaceId        = 0x%x\n", Fadt->XPm1aEvtBlk.AddressSpaceId));
  DEBUG ((DEBUG_INFO, "      RegisterBitWidth      = 0x%x\n", Fadt->XPm1aEvtBlk.RegisterBitWidth));
  DEBUG ((DEBUG_INFO, "      RegisterBitOffset     = 0x%x\n", Fadt->XPm1aEvtBlk.RegisterBitOffset));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPm1aEvtBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "      Address               = 0x%x\n", Fadt->XPm1aEvtBlk.Address));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPm1aEvtBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "Fadt  XPm1bEvtBlk \n"));
  DEBUG ((DEBUG_INFO, "      AddressSpaceId        = 0x%x\n", Fadt->XPm1bEvtBlk.AddressSpaceId));
  DEBUG ((DEBUG_INFO, "      RegisterBitWidth      = 0x%x\n", Fadt->XPm1bEvtBlk.RegisterBitWidth));
  DEBUG ((DEBUG_INFO, "      RegisterBitOffset     = 0x%x\n", Fadt->XPm1bEvtBlk.RegisterBitOffset));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPm1bEvtBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "      Address               = 0x%x\n", Fadt->XPm1bEvtBlk.Address));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPm1bEvtBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "Fadt  XPm1aCntBlk \n"));
  DEBUG ((DEBUG_INFO, "      AddressSpaceId        = 0x%x\n", Fadt->XPm1aCntBlk.AddressSpaceId));
  DEBUG ((DEBUG_INFO, "      RegisterBitWidth      = 0x%x\n", Fadt->XPm1aCntBlk.RegisterBitWidth));
  DEBUG ((DEBUG_INFO, "      RegisterBitOffset     = 0x%x\n", Fadt->XPm1aCntBlk.RegisterBitOffset));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPm1aCntBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "      Address               = 0x%x\n", Fadt->XPm1aCntBlk.Address));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPm1aCntBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "Fadt  XPm1bCntBlk \n"));
  DEBUG ((DEBUG_INFO, "      AddressSpaceId        = 0x%x\n", Fadt->XPm1bCntBlk.AddressSpaceId));
  DEBUG ((DEBUG_INFO, "      RegisterBitWidth      = 0x%x\n", Fadt->XPm1bCntBlk.RegisterBitWidth));
  DEBUG ((DEBUG_INFO, "      RegisterBitOffset     = 0x%x\n", Fadt->XPm1bCntBlk.RegisterBitOffset));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPm1bCntBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "      Address               = 0x%x\n", Fadt->XPm1bCntBlk.Address));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPm1bCntBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "Fadt  XPm2CntBlk \n"));
  DEBUG ((DEBUG_INFO, "      AddressSpaceId        = 0x%x\n", Fadt->XPm2CntBlk.AddressSpaceId));
  DEBUG ((DEBUG_INFO, "      RegisterBitWidth      = 0x%x\n", Fadt->XPm2CntBlk.RegisterBitWidth));
  DEBUG ((DEBUG_INFO, "      RegisterBitOffset     = 0x%x\n", Fadt->XPm2CntBlk.RegisterBitOffset));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPm2CntBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "      Address               = 0x%x\n", Fadt->XPm2CntBlk.Address));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPm2CntBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "Fadt  XPmTmrBlk \n"));
  DEBUG ((DEBUG_INFO, "      AddressSpaceId        = 0x%x\n", Fadt->XPmTmrBlk.AddressSpaceId));
  DEBUG ((DEBUG_INFO, "      RegisterBitWidth      = 0x%x\n", Fadt->XPmTmrBlk.RegisterBitWidth));
  DEBUG ((DEBUG_INFO, "      RegisterBitOffset     = 0x%x\n", Fadt->XPmTmrBlk.RegisterBitOffset));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPmTmrBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "      Address               = 0x%x\n", Fadt->XPmTmrBlk.Address));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XPmTmrBlk.AccessSize));
  DEBUG ((DEBUG_INFO, "Fadt  XGpe0Blk \n"));
  DEBUG ((DEBUG_INFO, "      AddressSpaceId        = 0x%x\n", Fadt->XGpe0Blk.AddressSpaceId));
  DEBUG ((DEBUG_INFO, "      RegisterBitWidth      = 0x%x\n", Fadt->XGpe0Blk.RegisterBitWidth));
  DEBUG ((DEBUG_INFO, "      RegisterBitOffset     = 0x%x\n", Fadt->XGpe0Blk.RegisterBitOffset));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XGpe0Blk.AccessSize));
  DEBUG ((DEBUG_INFO, "      Address               = 0x%x\n", Fadt->XGpe0Blk.Address));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XGpe0Blk.AccessSize));
  DEBUG ((DEBUG_INFO, "Fadt  XGpe1Blk \n"));
  DEBUG ((DEBUG_INFO, "      AddressSpaceId        = 0x%x\n", Fadt->XGpe1Blk.AddressSpaceId));
  DEBUG ((DEBUG_INFO, "      RegisterBitWidth      = 0x%x\n", Fadt->XGpe1Blk.RegisterBitWidth));
  DEBUG ((DEBUG_INFO, "      RegisterBitOffset     = 0x%x\n", Fadt->XGpe1Blk.RegisterBitOffset));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XGpe1Blk.AccessSize));
  DEBUG ((DEBUG_INFO, "      Address               = 0x%x\n", Fadt->XGpe1Blk.Address));
  DEBUG ((DEBUG_INFO, "      AccessSize            = 0x%x\n", Fadt->XGpe1Blk.AccessSize));

  //
  // Printing MMCFG table Values.
  //
  DEBUG ((DEBUG_INFO, "MMCFG \n"));
  DEBUG ((DEBUG_INFO, "     Signature             = %c%c%c%c\n", ((CHAR8 *)&MmCfgHdr->Header.Signature)[0], ((CHAR8 *)&MmCfgHdr->Header.Signature)[1], ((CHAR8 *)&MmCfgHdr->Header.Signature)[2], ((CHAR8 *)&MmCfgHdr->Header.Signature)[3]));
  DEBUG ((DEBUG_INFO, "     Length                = 0x%x\n", MmCfgHdr->Header.Length));
  DEBUG ((DEBUG_INFO, "     Revision              = 0x%x\n", MmCfgHdr->Header.Revision));
  DEBUG ((DEBUG_INFO, "     Checksum              = 0x%x\n", MmCfgHdr->Header.Checksum));
  DEBUG ((DEBUG_INFO, "     OemId                 = %c%c%c%c%c\n", MmCfgHdr->Header.OemId[0], MmCfgHdr->Header.OemId[1], MmCfgHdr->Header.OemId[2], MmCfgHdr->Header.OemId[3], MmCfgHdr->Header.OemId[4], MmCfgHdr->Header.OemId[5]));
  DEBUG ((
    DEBUG_INFO,
    "     OemTableId            = %c%c%c%c%c%c%c%c\n",
    ((CHAR8 *)&MmCfgHdr->Header.OemTableId)[0],
    ((CHAR8 *)&MmCfgHdr->Header.OemTableId)[1],
    ((CHAR8 *)&MmCfgHdr->Header.OemTableId)[2],
    ((CHAR8 *)&MmCfgHdr->Header.OemTableId)[3],
    ((CHAR8 *)&MmCfgHdr->Header.OemTableId)[4],
    ((CHAR8 *)&MmCfgHdr->Header.OemTableId)[5],
    ((CHAR8 *)&MmCfgHdr->Header.OemTableId)[6],
    ((CHAR8 *)&MmCfgHdr->Header.OemTableId)[7]
    ));
  DEBUG ((DEBUG_INFO, "     OemRevision           = 0x%x\n", MmCfgHdr->Header.OemRevision));
  DEBUG ((DEBUG_INFO, "     CreatorId             = %c%c%c%c\n", ((CHAR8 *)&MmCfgHdr->Header.CreatorId)[0], ((CHAR8 *)&MmCfgHdr->Header.CreatorId)[1], ((CHAR8 *)&MmCfgHdr->Header.CreatorId)[2], ((CHAR8 *)&MmCfgHdr->Header.CreatorId)[3]));
  DEBUG ((DEBUG_INFO, "     CreatorRevision       = 0x%x\n", MmCfgHdr->Header.CreatorRevision));
  DEBUG ((DEBUG_INFO, "PCI Config Space      \n"));

  for (UINT16 Index = 0; Index < NodeCount; Index++) {
    DEBUG ((DEBUG_INFO, "PCI Segment %d \n", Index));
    DEBUG ((DEBUG_INFO, "   Segment[%2d].BaseAddress           = 0x%x\n", Index, MmCfgBase->BaseAddress));
    DEBUG ((DEBUG_INFO, "   Segment[%2d].PciSegmentGroupNumber = 0x%x\n", Index, MmCfgBase->PciSegmentGroupNumber));
    DEBUG ((DEBUG_INFO, "   Segment[%2d].StartBusNumber        = 0x%x\n", Index, MmCfgBase->StartBusNumber));
    DEBUG ((DEBUG_INFO, "   Segment[%2d].EndBusNumber          = 0x%x\n\n", Index, MmCfgBase->EndBusNumber));
  }

  //
  // Printing RSDP table Values.
  //
  DEBUG ((DEBUG_INFO, "RSDP \n"));
  DEBUG ((DEBUG_INFO, "     Signature            = %c%c%c%c%c%c%c\n", ((CHAR8 *)&Rsdp->Signature)[0], ((CHAR8 *)&Rsdp->Signature)[1], ((CHAR8 *)&Rsdp->Signature)[2], ((CHAR8 *)&Rsdp->Signature)[3], ((CHAR8 *)&Rsdp->Signature)[4], ((CHAR8 *)&Rsdp->Signature)[5], ((CHAR8 *)&Rsdp->Signature)[6]));
  DEBUG ((DEBUG_INFO, "     Length               = 0x%x\n", Rsdp->Length));
  DEBUG ((DEBUG_INFO, "     Revision             = 0x%x\n", Rsdp->Revision));
  DEBUG ((DEBUG_INFO, "     Checksum             = 0x%x\n", Rsdp->Checksum));
  DEBUG ((DEBUG_INFO, "     OemId                = %c%c%c%c%c\n", Rsdp->OemId[0], Rsdp->OemId[1], Rsdp->OemId[2], Rsdp->OemId[3], Rsdp->OemId[4], Rsdp->OemId[5]));
  DEBUG ((DEBUG_INFO, "     ExtendedChecksum     = 0x%x\n", Rsdp->ExtendedChecksum));
  DEBUG ((DEBUG_INFO, "     XSDT                 = 0x%lx\n", Rsdp->XsdtAddress));
  DEBUG ((DEBUG_INFO, "     Reserved             = %x %x %x\n", Rsdp->Reserved[0], Rsdp->Reserved[1], Rsdp->Reserved[2]));

  //
  // Printing XSDT table Values.
  //
  DEBUG ((DEBUG_INFO, "XSDT \n"));
  DEBUG ((DEBUG_INFO, "     Signature            = %c%c%c%c\n", ((CHAR8 *)&Xsdt->Header.Signature)[0], ((CHAR8 *)&Xsdt->Header.Signature)[1], ((CHAR8 *)&Xsdt->Header.Signature)[2], ((CHAR8 *)&Xsdt->Header.Signature)[3]));
  DEBUG ((DEBUG_INFO, "     Length               = 0x%x\n", Xsdt->Header.Length));
  DEBUG ((DEBUG_INFO, "     Revision             = 0x%x\n", Xsdt->Header.Revision));
  DEBUG ((DEBUG_INFO, "     Checksum             = 0x%x\n", Xsdt->Header.Checksum));
  DEBUG ((DEBUG_INFO, "     OemId                = %c%c%c%c%c\n", Xsdt->Header.OemId[0], Xsdt->Header.OemId[1], Xsdt->Header.OemId[2], Xsdt->Header.OemId[3], Xsdt->Header.OemId[4], Xsdt->Header.OemId[5]));
  DEBUG ((
    DEBUG_INFO,
    "     OemTableId           = %c%c%c%c%c%c%c%c\n",
    ((CHAR8 *)&Xsdt->Header.OemTableId)[0],
    ((CHAR8 *)&Xsdt->Header.OemTableId)[1],
    ((CHAR8 *)&Xsdt->Header.OemTableId)[2],
    ((CHAR8 *)&Xsdt->Header.OemTableId)[3],
    ((CHAR8 *)&Xsdt->Header.OemTableId)[4],
    ((CHAR8 *)&Xsdt->Header.OemTableId)[5],
    ((CHAR8 *)&Xsdt->Header.OemTableId)[6],
    ((CHAR8 *)&Xsdt->Header.OemTableId)[7]
    ));
  DEBUG ((DEBUG_INFO, "     OemRevision          = 0x%x\n", Xsdt->Header.OemRevision));
  DEBUG ((DEBUG_INFO, "     CreatorId            = %c%c%c%c\n", ((CHAR8 *)&Xsdt->Header.CreatorId)[0], ((CHAR8 *)&Xsdt->Header.CreatorId)[1], ((CHAR8 *)&Xsdt->Header.CreatorId)[2], ((CHAR8 *)&Xsdt->Header.CreatorId)[3]));
  DEBUG ((DEBUG_INFO, "     CreatorRevision      = 0x%x\n", Xsdt->Header.CreatorRevision));
  DEBUG ((DEBUG_INFO, "     XSDT[0]              = 0x%lx\n", Xsdt->Tables[0]));
  DEBUG ((DEBUG_INFO, "     XSDT[1]              = 0x%lx\n", Xsdt->Tables[1]));
  DEBUG ((DEBUG_INFO, "     XSDT[2]              = 0x%lx\n", Xsdt->Tables[2]));
  DEBUG ((DEBUG_INFO, "     XSDT[3]              = 0x%lx\n", Xsdt->Tables[3]));
}

STATIC
INTN
EFIAPI
BlobKeyCompare (
  IN CONST VOID  *StandaloneKey,
  IN CONST VOID  *UserStruct
  )
{
  CONST BLOB  *Blob;

  Blob = UserStruct;
  return AsciiStrCmp (StandaloneKey, (CONST CHAR8 *)Blob->File);
}

STATIC
INTN
EFIAPI
BlobCompare (
  IN CONST VOID  *UserStruct1,
  IN CONST VOID  *UserStruct2
  )
{
  CONST BLOB  *Blob1;

  Blob1 = UserStruct1;
  return BlobKeyCompare (Blob1->File, UserStruct2);
}

STATIC
INTN
EFIAPI
PointerCompare (
  IN CONST VOID  *Pointer1,
  IN CONST VOID  *Pointer2
  )
{
  if (Pointer1 == Pointer2) {
    return 0;
  }

  if ((UINTN)Pointer1 < (UINTN)Pointer2) {
    return -1;
  }

  return 1;
}

EFI_STATUS
EFIAPI
OvmfAcpiEntrypoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                      Status;
  FIRMWARE_CONFIG_ITEM            FwCfgItem;
  UINTN                           FwCfgSize;
  QEMU_LOADER_ENTRY               *LoaderStart;
  CONST QEMU_LOADER_ENTRY         *LoaderEntry, *LoaderEnd;
  CONST QEMU_LOADER_ENTRY         *WritePointerSubsetEnd;
  ORDERED_COLLECTION              *Tracker;
  ORDERED_COLLECTION              *AllocationsRestrictedTo32Bit;
  UINTN                           *InstalledKey;
  ORDERED_COLLECTION              *SeenPointers;
  INT32                           Installed;
  CONST ORDERED_COLLECTION_ENTRY  *TrackerEntry;
  CONST BLOB                      *Blob;

  Status = QemuFwCfgFindFile ("etc/table-loader", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FwCfgSize % sizeof *LoaderEntry != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: \"etc/table-loader\" has invalid size 0x%Lx\n",
      __FUNCTION__,
      (UINT64)FwCfgSize
      ));
    return EFI_PROTOCOL_ERROR;
  }

  PeiServicesAllocatePool (FwCfgSize, (void **)&LoaderStart);
  if (LoaderStart == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, LoaderStart);
  LoaderEnd                    = LoaderStart + FwCfgSize / sizeof *LoaderEntry;
  AllocationsRestrictedTo32Bit = NULL;
  Status                       = CollectAllocationsRestrictedTo32Bit (
                                   &AllocationsRestrictedTo32Bit,
                                   LoaderStart,
                                   LoaderEnd
                                   );

  Tracker               = OrderedCollectionInit (BlobCompare, BlobKeyCompare);
  WritePointerSubsetEnd = LoaderStart;
  for (LoaderEntry = LoaderStart; LoaderEntry < LoaderEnd; ++LoaderEntry) {
    switch (LoaderEntry->Type) {
      case QemuLoaderCmdAllocate:
        Status = ProcessCmdAllocate (
                   &LoaderEntry->Command.Allocate,
                   Tracker,
                   AllocationsRestrictedTo32Bit
                   );
        break;

      case QemuLoaderCmdAddPointer:
        Status = ProcessCmdAddPointer (
                   &LoaderEntry->Command.AddPointer,
                   Tracker
                   );
        break;

      case QemuLoaderCmdAddChecksum:
        Status = ProcessCmdAddChecksum (
                   &LoaderEntry->Command.AddChecksum,
                   Tracker
                   );
        break;

      case QemuLoaderCmdWritePointer:
        Status = ProcessCmdWritePointer (
                   &LoaderEntry->Command.WritePointer,
                   Tracker
                   );
        if (!EFI_ERROR (Status)) {
          WritePointerSubsetEnd = LoaderEntry + 1;
        }

        break;

      default:
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: unknown loader command: 0x%x\n",
          __FUNCTION__,
          LoaderEntry->Type
          ));
        break;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:Error", __FUNCTION__));
    }
  }

  PeiServicesAllocatePool (INSTALLED_TABLES_MAX * sizeof *InstalledKey, (void **)&InstalledKey);
  if (InstalledKey == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  }

  SeenPointers = OrderedCollectionInit (PointerCompare, PointerCompare);
  if (SeenPointers == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  }

  //
  // identify and install ACPI tables
  //
  Installed = 0;
  for (LoaderEntry = LoaderStart; LoaderEntry < LoaderEnd; ++LoaderEntry) {
    if (LoaderEntry->Type == QemuLoaderCmdAddPointer) {
      if (!AsciiStringCompare (LoaderEntry->Command.AddPointer.PointeeFile, "etc/acpi/tables")) {
        TrackerEntry = OrderedCollectionFind (Tracker, LoaderEntry->Command.AddPointer.PointerFile);
        Blob         = OrderedCollectionUserStruct (TrackerEntry);
        ParseAcpi (Blob->Base);
        break;
      }
    }
  }

  return Status;
}
