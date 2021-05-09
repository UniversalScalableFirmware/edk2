/** @file
  This file defines the SMM info hob structure.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PAYLOAD_SMM_INFO_GUID_H__
#define __PAYLOAD_SMM_INFO_GUID_H__

///
/// SMM Information GUID
///
extern EFI_GUID gSmmInformationGuid;

//
// Set this flag if there are 4KB SMRAM at SmmBase used for communication
// between bootloader and UEFI payload.
//
#define   SMM_FLAGS_4KB_COMMUNICATION  BIT0

typedef enum {
  REG_TYPE_MEM,
  REG_TYPE_IO,
  REG_TYPE_PCICFG,
  REG_TYPE_MMIO,
} REG_TYPE;

typedef enum {
  WIDE8   = 1,
  WIDE16  = 2,
  WIDE32  = 4
} REG_WIDTH;

#pragma pack(1)

///
/// SMI control register
///
typedef struct {
  // Refer REG_TYPE
  UINT8   RegType;

  // REG_WIDTH
  UINT8   RegWidth;

  // The bit index for APMC Enable (APMC_EN).
  UINT8   SmiApmPos;

  // The bit index for Global SMI Enable (GBL_SMI_EN)
  UINT8   SmiGblPos;

  /// The bit index for End of SMI (EOS)
  UINT8   SmiEosPos;
  UINT8   Rsvd[3];

  /// Address for SMM control and enable register
  UINT32  Address;
} SMI_CTRL_REG;

///
/// SMI status register
///
typedef struct {
  // Refer REG_TYPE
  UINT8   RegType;

  // REG_WIDTH
  UINT8   RegWidth;

  /// The bit index for APM Status (APM_STS).
  UINT8   SmiApmPos;
  UINT8   Rsvd[5];

  /// Address for SMM status register
  UINT32  Address;
} SMI_STS_REG;

///
/// SMI lock register
///
typedef struct {
  // Refer REG_TYPE
  UINT8   RegType;

  // REG_WIDTH
  UINT8   RegWidth;

  // The bit index for SMI Lock (SMI_LOCK)
  UINT8   SmiLockPos;
  UINT8   Rsvd;

  // Register address for SMM SMI lock
  UINT32  Address;
} SMI_LOCK_REG;

typedef struct {
  UINT8                       Revision;
  UINT8                       Flags;
  UINT8                       Reserved[2];
  //
  // Tseg base address
  //
  UINT32                      SmmBase;
  UINT32                      SmmSize;
  SMI_CTRL_REG                SmiCtrlReg;
  SMI_STS_REG                 SmiStsReg;
  SMI_LOCK_REG                SmiLockReg;
} LOADER_SMM_INFO;


///
/// The information below is used for communication between bootloader and payload.
/// It is used to save/store some registers in S3 path when the bootloader doesn't
/// support SMM.
///
#define BL_PLD_COMM_SIG       SIGNATURE_32('B', 'P', 'C', 'O')

//
// Payload and bootloader communication region
//
// This region exists only when SMM_FLAGS_4KB_COMMUNICATION is set in HOB flag.
// Communication area starting from TSEG.Data structures can be present in any 
// order. They are identified by their Id.
//
//  --------------------------
//  |     BL_PLD_COMM_HDR    |
//  |         (Id:*)         |
//  --------------------------
//  |       DATA             |
//  --------------------------
//  --------------------------
//  |     BL_PLD_COMM_HDR    |
//  |         (Id:*)         |
//  --------------------------
//  |      DATA              |
//  --------------------------<----TSEG_BASE
//

#define SMMBASE_INFO_COMM_ID  1
#define BL_SW_SMI_COMM_ID     3

#define COMM_ID_SMMBASE_INFO  1
#define COMM_ID_BL_SW_SMI     3

#pragma pack(1)

typedef struct {
  UINT32          Signature;
  UINT8           Id;
  UINT8           Count;
  UINT16          TotalSize;
} BL_PLD_COMM_HDR;


//
// COMM_ID_SMMBASE_INFO
// This data structure is required when bootloader doesn't support SMM.
// Payload would save smmrebase info in normal boot.
// In S3 path, bootloader need restore SMM base.
//
typedef struct {
  UINT32          ApicId;
  UINT32          SmmBase;
} CPU_SMM_BASE;

typedef struct {
  BL_PLD_COMM_HDR SmmBaseHdr;
  CPU_SMM_BASE    SmmBase[0];
} SMMBASE_INFO;

//
// COMM_ID_BL_SW_SMI
// This data structure is required when bootloader doesn't support SMM.
// Payload would save SW SMI value in normal boot.
// In S3 path, bootloader need writie IO port 0xB2 with BlSwSmiHandlerInput
// to trigger SMI.
//
typedef struct {
  BL_PLD_COMM_HDR BlSwSmiHdr;
  UINT8           BlSwSmiHandlerInput;
} BL_SW_SMI_INFO;

#pragma pack()

#endif
