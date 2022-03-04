/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EDKII_PREPARE_FOR_UNIVERSAL_PAYLOAD_PPI_H_
#define _EDKII_PREPARE_FOR_UNIVERSAL_PAYLOAD_PPI_H_

typedef struct _EDKII_PREPARE_FOR_UNIVERSAL_PAYLOAD_PPI EDKII_PREPARE_FOR_UNIVERSAL_PAYLOAD_PPI;

/**
  Do preparation work for Universal Payload.

  @param[in]  HobStart             The start address of Hob.

  @retval The interface for universal payload.
          If return value is NULL, then the preparation work fails
**/
typedef
VOID *
(EFIAPI *EDKII_PREPARE_FOR_UNIVERSAL_PAYLOAD)(
  IN VOID *HobStart
  );

struct _EDKII_PREPARE_FOR_UNIVERSAL_PAYLOAD_PPI {
  EDKII_PREPARE_FOR_UNIVERSAL_PAYLOAD    PrepareForUniversalPayload;
};

extern EFI_GUID  gEdkiiPrepareForUniversalPayloadPpiGuid;

#endif
