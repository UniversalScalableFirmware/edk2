@if "%1" == "ovmf" (
  goto ovmf
)

call py UefiPayloadPkg\UniversalPayloadBuild.py -t CLANGPDB -D CPU_TIMER_LIB_ENABLE=FALSE
if not %ERRORLEVEL% == 0 exit /b 1


:ovmf
call build -p OvmfPkg\QemuUniversalPayload\OvmfPkgPol.dsc -a IA32 -a X64 -D DEBUG_ON_SERIAL_PORT -D UNIVERSAL_PAYLOAD=TRUE -t CLANGPDB -y ovmflog.txt

if not %ERRORLEVEL% == 0 exit /b 1

call C:\qemu\qemu-system-x86_64.exe -m 256M -machine q35 -drive file=Build\OvmfPol\DEBUG_CLANGPDB\FV\OVMF.fd,if=pflash,format=raw -boot menu=on,splash-time=0 -usb -device nec-usb-xhci,id=xhci -device usb-kbd   -device usb-mouse -net none -serial mon:stdio