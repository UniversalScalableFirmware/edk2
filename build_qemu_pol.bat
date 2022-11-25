@echo off

@setlocal

if "%WORKSPACE%" == "" (
  call edksetup.bat
)

if not exist BaseTools\Bin\Win32\GenFw.exe (
  cd BaseTools
  call toolsetup.bat forcerebuild
  cd ..
)

if "%VS_VER%" == "" (
  set VS_VER=VS2019
)

@if "%1" == "entry" (
  goto entry
)
@if "%1" == "objcopy" (
  goto objcopy
)

@if "%1" == "ovmf" (
  goto ovmf
)

@if "%1" == "run" (
  goto run
)

echo Building Universal UEFI payload DXE FV ...
call build -p UefiPayloadPkg\UefiPayloadPkg.dsc -a X64 -D UNIVERSAL_PAYLOAD=TRUE -t %VS_VER% -y upllog.txt
if not %ERRORLEVEL% == 0 exit /b 1

:entry
echo Building Universal UEFI payload entry module ...
call build -p UefiPayloadPkg\UefiPayloadPkg.dsc -a X64 -D UNIVERSAL_PAYLOAD=TRUE -m UefiPayloadPkg\UefiPayloadEntry\UniversalPayloadEntry.inf -t CLANGDWARF
if not %ERRORLEVEL% == 0 exit /b 1


:objcopy
echo Generating Universal Payload binary ...
@set FV=Build\UefiPayloadPkgX64\DEBUG_%VS_VER%\FV\DXEFV.Fv
@set ENTRY=Build\UefiPayloadPkgX64\DEBUG_CLANGDWARF\X64\UefiPayloadPkg\UefiPayloadEntry\UniversalPayloadEntry\DEBUG\UniversalPayloadEntry.elf

@REM
@REM If dependent sources are not changed, entry.elf isn't updated by build tool.
@REM So, if we don't remove the uefi_fv section before adding, there will be 2 uefi_fv sections.
@REM
@REM Remove .upld.uefi_fv section because entry.elf isn't updated if dependent sources are not changed.\
python UefiPayloadPkg\Tools\GenUpldInfo.py Build\upld_info UEFI
@if not %ERRORLEVEL% == 0 exit /b 1
"C:\Program Files\LLVM\bin\llvm-objcopy" -I elf64-x86-64 -O elf64-x86-64 --remove-section .upld_info --remove-section .upld.uefi_fv %ENTRY%
@if not %ERRORLEVEL% == 0 exit /b 1
"C:\Program Files\LLVM\bin\llvm-objcopy" -I elf64-x86-64 -O elf64-x86-64 --add-section .upld_info=Build\upld_info --add-section .upld.uefi_fv=%FV% %ENTRY%
@if not %ERRORLEVEL% == 0 exit /b 1
"C:\Program Files\LLVM\bin\llvm-objcopy" -I elf64-x86-64 -O elf64-x86-64 --set-section-alignment .upld.upld_info=16 --set-section-alignment .upld.uefi_fv=16 %ENTRY%
@if not %ERRORLEVEL% == 0 exit /b 1

:ovmf
echo Building OVMF POL ...
call build -p OvmfPkg\OvmfPkgPol.dsc -a IA32 -a X64 -D DEBUG_ON_SERIAL_PORT  -D SMM_REQUIRE=TRUE  -t %VS_VER% -y ovmflog.txt
@if not %ERRORLEVEL% == 0 exit /b 1
goto :eof

:run
echo Running OVMF POL on QEMU ...
"C:\Program Files\qemu\qemu-system-x86_64.exe" -m 512M -cpu max -machine q35,accel=tcg -drive file=Build\OvmfPol\DEBUG_%VS_VER%\FV\OVMF.fd,if=pflash,format=raw -boot menu=on,splash-time=0 -net none --serial stdio




