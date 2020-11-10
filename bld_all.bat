call build -p UefiPayloadPkg\UefiPayloadPkg.dsc -a IA32 -a X64 -t VS2019 -y report.log
REM copy build\UefiPayloadPkgX64\DEBUG_VS2019\FV\UEFIPAYLOAD.fd build\UefiPayloadPkgX64\DEBUG_VS2019\FV\UEFIPAYLOAD_ORG.fd
REM fmmt -r build\UefiPayloadPkgX64\DEBUG_VS2019\FV\UEFIPAYLOAD.fd A7008BCD-7D3E-4692-A5C4-D31DF67E2994 PcdDxe Build\Ovmf3264\DEBUG_VS2019\FV\Ffs\80CF7257-87AB-47f9-A3FE-D50B76D89541PcdDxe\80CF7257-87AB-47f9-A3FE-D50B76D89541.ffs build\UefiPayloadPkgX64\DEBUG_VS2019\FV\UEFIPAYLOAD.fd
py -3 ..\..\pack_payload.py -i Build\UefiPayloadPkgX64\DEBUG_VS2019\FV\UEFIPAYLOAD.fd -a 4096 -ai -o ..\..\UPL.bin
call build -p ovmfpkg\OvmfPkgPol.dsc -a IA32 -a X64 -t VS2019 -D DEBUG_ON_SERIAL_PORT