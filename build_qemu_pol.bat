@setlocal

@if "%1" == "entry" (
  goto entry
)
@if "%1" == "objcopy" (
  goto objcopy
)

@if "%1" == "ovmf" (
  goto ovmf
)

call build -p UefiPayloadPkg\UefiPayloadPkg.dsc -a X64 -D UNIVERSAL_PAYLOAD=TRUE -t VS2019 -y upllog.txt
if not %ERRORLEVEL% == 0 exit /b 1

:entry
call build -p UefiPayloadPkg\UefiPayloadPkg.dsc -a X64 -D UNIVERSAL_PAYLOAD=TRUE -m UefiPayloadPkg\UefiPayloadEntry\UniversalPayloadEntry.inf -t CLANGDWARF 
if not %ERRORLEVEL% == 0 exit /b 1


:objcopy
@set FV=Build\UefiPayloadPkgX64\DEBUG_VS2019\FV\DXEFV.Fv
@set ENTRY=Build\UefiPayloadPkgX64\DEBUG_CLANGDWARF\X64\UefiPayloadPkg\UefiPayloadEntry\UniversalPayloadEntry\DEBUG\UniversalPayloadEntry.elf

@REM 
@REM If dependent sources are not changed, entry.elf isn't updated by build tool.
@REM So, if we don't remove the uefi_fv section before adding, there will be 2 uefi_fv sections.
@REM
@REM Remove .upld.uefi_fv section because entry.elf isn't updated if dependent sources are not changed.
"C:\Program Files\LLVM\bin\llvm-objcopy" -I elf64-x86-64 -O elf64-x86-64 --remove-section .upld.uefi_fv %ENTRY%
@if not %ERRORLEVEL% == 0 exit /b 1
"C:\Program Files\LLVM\bin\llvm-objcopy" -I elf64-x86-64 -O elf64-x86-64 --add-section .upld.uefi_fv=%FV% %ENTRY%
@if not %ERRORLEVEL% == 0 exit /b 1
"C:\Program Files\LLVM\bin\llvm-objcopy" -I elf64-x86-64 -O elf64-x86-64 --set-section-alignment .upld.uefi_fv=16 %ENTRY%
@if not %ERRORLEVEL% == 0 exit /b 1

:ovmf
call build -p OvmfPkg\OvmfPkgPol.dsc -a IA32 -a X64 -D DEBUG_ON_SERIAL_PORT -t VS2019 -y ovmflog.txt
