@if "%1" == "ovmf" (
  goto ovmf
)

call py UefiPayloadPkg\UniversalPayloadBuild.py -t CLANGPDB 
if not %ERRORLEVEL% == 0 exit /b 1


:ovmf
call build -p OvmfPkg\QemuUniversalPayload\OvmfPkgPol.dsc -a IA32 -a X64 -D DEBUG_ON_SERIAL_PORT -D UNIVERSAL_PAYLOAD=TRUE -t CLANGPDB -y ovmflog.txt
