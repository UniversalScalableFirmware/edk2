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
  ; Save the bootloader stack pointer
  ;
  mov     eax, esp

  mov     esp, FixedPcdGet32 (PcdPayloadStackTop)

  ;
  ; Prepare data strcuture at stack top
  ;
  push    0
  push    dword [eax + 8]
  push    0
  push    dword [eax + 4]

  ;
  ; Push arguments for entry point
  ;
  push    dword [eax + 8]
  push    dword [eax + 4]
  ;
  ; Call into C code
  ;
  call    ASM_PFX(PayloadEntry)
  jmp     $

