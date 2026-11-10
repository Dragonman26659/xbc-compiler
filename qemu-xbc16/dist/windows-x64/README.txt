16BC native Windows x64 QEMU 9.2.0

Run the default example: run.cmd
Run your program: run.cmd "C:\path with spaces\program.bin"
Character input: run.cmd -Headless -Keyboard "examples\keyboard.bin"
Monitor: run.cmd -Monitor "examples\boot.bin"

Keep the EXE and DLL files together. MSYS2 is not needed to run this folder.
SDL window: 80x25 VGA-style text, keyboard and relative mouse input.
Graphics/input demo: run.cmd examples\graphics.bin
Headless: run.cmd -Headless examples\boot.bin

Upstream source: https://gitlab.com/qemu-project/qemu.git
QEMU v9.2.0 commit: ae35f033b874c627d81d51070187fbf55f0bf1a7
Extension sources are in source/xbc16. The full configured upstream
source checkout is retained in the build workspace by build.sh.
