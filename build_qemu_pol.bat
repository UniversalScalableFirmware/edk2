call build -p UefiPayloadPkg\UefiPayloadPkg.dsc -a IA32 -a X64 -D UNIVERSAL_PAYLOAD=TRUE -t VS2019
if not %ERRORLEVEL% == 0 exit /b 1

call py -3 ..\payload_poc\tools\pack_payload.py -i Build\UefiPayloadPkgX64\DEBUG_VS2019\FV\UEFIPAYLOAD.fd -a 4096 -ai -o UPL.bin
if not %ERRORLEVEL% == 0 exit /b 1

call build -p OvmfPkg\OvmfPkgPol.dsc -a IA32 -a X64 -D DEBUG_ON_SERIAL_PORT -t VS2019
