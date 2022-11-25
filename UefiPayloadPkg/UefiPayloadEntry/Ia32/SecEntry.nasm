;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2020, Intel Corporation. All rights reserved.<BR>
;*   SPDX-License-Identifier: BSD-2-Clause-Patent

;------------------------------------------------------------------------------

#include <Base.h>

SECTION .text

extern ASM_PFX(PayloadEntry)
extern  ASM_PFX(PcdGet32 (PcdPayloadStackTop))

;
; SecCore Entry Point
;
; Processor is in flat protected mode

global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):

  ;
  ; Disable all the interrupts
  ;
  cli

  ;
  ; Save the bootloader parameter base address
  ;
  mov   eax, [esp + 4]

  mov   esp, FixedPcdGet32 (PcdPayloadStackTop)

  ;
  ; Change code to use bootloader stack late.
  ; Push the bootloader parameter address onto new stack
  ;
  ; Save bootloader parameter
  push  0
  push  eax

  ; Save serial port info for platform hook library
  push  0
  push  0
  push  0
  push  0
  push  0
  push  eax
  ;
  ; Call into C code
  ;
  call  ASM_PFX(PayloadEntry)
  jmp   $
