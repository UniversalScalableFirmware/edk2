/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __UNIVERSAL_PAYLOAD_H__
#define __UNIVERSAL_PAYLOAD_H__

typedef  VOID   (EFIAPI *UNIVERSAL_PAYLOAD_ENTRY) (VOID *HobList, VOID *ImageBase);

#define  UPLD_IMAGE_HEADER_ID    SIGNATURE_32('P','L','D', 'H')
#define  UPLD_RELOC_ID           SIGNATURE_32('P','L','D', 'R')
#define  UPLD_AUTH_ID            SIGNATURE_32('P','L','D', 'A')

#define  UPLD_AUTH_PUBKEY_ID     SIGNATURE_32('P','U','B', 'K')
#define  UPLD_AUTH_SIGNATURE_ID  SIGNATURE_32('S','I','G', 'N')

#define  UPLD_RELOC_FMT_RAW      0
#define  UPLD_RELOC_FMT_PTR      1

#define  UPLD_IMAGE_CAP_PIC      BIT0
#define  UPLD_IMAGE_CAP_RELOC    BIT1
#define  UPLD_IMAGE_CAP_AUTH     BIT2

typedef struct {
  UINT32                  Identifier;
  UINT32                  HeaderLength;
  UINT8                   HeaderRevision;
  UINT8                   Reserved1[3];
} UPLD_COMMON_HEADER;


typedef struct {
  UPLD_COMMON_HEADER      CommonHeader;
  CHAR8                   ProducerId[8];
  CHAR8                   ImageId[8];
  UINT32                  Revision;
  UINT32                  Length;
  UINT32                  Svn;
  UINT16                  Reserved2;
  UINT16                  Machine;
  UINT32                  Capability;
  UINT32                  ImageOffset;
  UINT32                  ImageLength;
  UINT64                  ImageBase;
  UINT32                  ImageAlignment;
  UINT32                  EntryPointOffset;
} UPLD_INFO_HEADER;


typedef struct {
  UPLD_COMMON_HEADER      CommonHeader;
  UINT8                   RelocFmt;
  UINT8                   Reserved;
  UINT16                  RelocImgStripped;
  UINT32                  RelocImgOffset;
} UPLD_RELOC_HEADER;


typedef struct {
  UINT32                  PageRva;
  UINT32                  BlockSize;
} PE_RELOC_BLOCK_HEADER;


typedef struct {
  UPLD_COMMON_HEADER      CommonHeader;
  UINT8                   AuthData[0];
} UPLD_AUTH_HEADER;


typedef struct {
  //signature ('P', 'U', 'B', 'K')
  UINT32                   Identifier;

  //Length of Public Key
  UINT16                   KeySize;

  //KeyType RSA or ECC
  UINT8                    KeyType;

  UINT8                    Rsvd;

  //Pubic key data with KeySize bytes
  //Contains Modulus(256/384 sized) and PubExp[4]
  UINT8                    KeyData[0];
} UPLD_PUB_KEY_HDR;

typedef struct {
  //signature Identifier('S', 'I', 'G', 'N')
  UINT32                   Identifier;

  //Length of signature 2K and 3K in bytes
  UINT16                   SigSize;

 //PKCSv1.5 or RSA-PSS or ECC
  UINT8                    SigType;

  //Hash Alg for signingh SHA256, SHA384
  UINT8                    HashAlg;

  //Signature length defined by SigSize bytes
  UINT8                    Signature[0];
} UPLD_SIGNATURE_HDR;

#endif
