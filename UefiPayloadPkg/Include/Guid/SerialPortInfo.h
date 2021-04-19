/** @file
  This file defines the hob structure for serial port.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLD_SERIAL_PORT_INFO_H__
#define __PLD_SERIAL_PORT_INFO_H__

///
/// Serial Port Information GUID
///
extern EFI_GUID gPldSerialPortInfoGuid;

#pragma pack(1)
typedef struct {
  UINT16        Revision;
  BOOLEAN       UseMmio;
  UINT8         RegisterWidth;
  UINT32        BaudRate;
  UINT64        RegisterBase;
} PLD_SERIAL_PORT_INFO;
#pragma pack()

#endif
