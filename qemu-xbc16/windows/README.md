# Native Windows x64 configuration

This configuration builds a native `qemu-system-xbc16.exe` with MSYS2 MINGW64.
It runs the 16BC binary format directly on Windows; WSL is not required.
Windows x64 is the configured host. The guest architecture remains 16BC.

## First build

1. Install [MSYS2](https://www.msys2.org/) at its default `C:\msys64` location.
2. Open **MSYS2 MINGW64** and run `pacman -Syu`. If it asks you to close the
   terminal, reopen MINGW64 and run `pacman -Syu` again to complete the update.
3. Extract this extension. Open PowerShell in its `qemu-xbc16` folder and run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\windows\build.ps1
```

This invocation permits this script for that PowerShell process; it does not
change the machine's execution-policy setting. The build runs under MSYS2's
MINGW64 shell, installs the required build packages, clones the exact QEMU
9.2.0 commit, applies the overlay, configures only `xbc16-softmmu`, builds and
runs boot/input tests. Internet access and several GB of working disk space
are needed for the first build.

Defaults:

| Setting | Value |
|---|---|
| MSYS2 location | `C:\msys64` |
| Source/build workspace | `C:\xbc16-build` |
| Parallel compiler jobs | 4 |
| Executable output | `dist\windows-x64\qemu-system-xbc16.exe` |
| Portable ZIP output | `dist\qemu-xbc16-windows-x64.zip` |

The `dist` paths are relative to the extension folder. QEMU source/build paths
must not contain spaces. The build workspace is deliberately separate from
the extension folder, which may have spaces in its path. To customize:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\windows\build.ps1 `
    -Msys2Root D:\msys64 -WorkDir D:\xbc16-build -Jobs 8
```

You can also run directly inside MSYS2 **MINGW64**:

```sh
bash windows/build.sh /c/xbc16-build 4
```

The script selects MinGW GCC, Python, pkg-config and Ninja explicitly. Your
Visual Studio compiler/assembler projects remain separate; QEMU uses its own
Meson/MinGW build. No `.vcxproj` is needed for QEMU.

## Run a binary

After the build, from the extension folder:

```powershell
.\windows\run.cmd "C:\path with spaces\program.bin"
```

Without arguments, `run.cmd` boots the included arithmetic example and prints
42 as `R1=0x002a` / `R3=0x002a` before exiting. A binary must be a headerless
little-endian 16-bit word sequence, with an even length between 2 and 131072
bytes. The launcher validates the length; QEMU loads it at guest word address 0.

Optional character input and the built-in monitor:

```powershell
.\windows\run.cmd -Keyboard ".\examples\keyboard.bin"
.\windows\run.cmd -Monitor ".\examples\boot.bin"
```

Keyboard mode attaches console bytes to keyboard interrupt vector 0. Monitor
mode keeps QEMU open after HLT; enter `info registers`, `xp /16hx 0`, or `quit`.
Choose one mode, since both use the console.

From PowerShell, the launcher can also use a custom EXE location:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\windows\run.ps1 `
    -QemuExe "D:\tools\qemu-system-xbc16.exe" -Bin "D:\program.bin"
```

## Portable output

The packaging step inspects the PE imports with MinGW `objdump` and copies
recursive non-system DLL dependencies beside the EXE. It fails if a DLL cannot
be resolved, and never copies Windows system DLLs. Keep the entire output folder
together. The portable folder has its own `run.cmd` and examples; after packaging
it should run without MSYS2 on the destination machine.

The build tests the packaged EXE again with a PATH containing only its own
folder and Windows System32, so DLLs cannot be supplied by the MSYS2 PATH.
A file hash manifest, available runtime license files and extension sources
are included. The original QEMU source checkout remains in the build workspace.

## Validation and limits

The original Linux QEMU build/boot/input tests pass. PowerShell and Bash syntax and the DLL
resolver's recursion, case-insensitive lookup, cycles, missing-DLL rejection
and unsafe-import rejection were checked here. Windows compilation and execution
have **not** been run in this environment, so this download contains build
configuration and source, not a prebuilt Windows EXE. The script runs the actual
Windows smoke tests before reporting success.

The target remains pinned to QEMU 9.2.0. Existing instruction, memory-layout,
interrupt, snapshot and display limitations are documented in the root README.

References:

- [QEMU Windows build environment](https://www.qemu.org/docs/master/devel/build-environment.html#windows)
- [MSYS2 environments](https://www.msys2.org/docs/environments/)

## Python bootstrap dependency

The build installs pycotap 1.3.1 into the workspace's python-deps directory,
then exposes that private directory to QEMU through PYTHONPATH. This handles
QEMU 9.2's initial offline check without modifying the MSYS2 Python install.
It uses the checkout's bundled wheel by filesystem path when available,
and falls back to PyPI if that wheel is missing.

Compiler and pkg-config paths are converted to native C:/ paths before
configure writes Meson's configuration. MSYS /mingw64 paths cannot be launched
by native Windows Python. Re-running setup regenerates that configuration;
there is no need to delete the checkout.

For Windows builds containing only xbc16-softmmu, the installer skips QEMU's
optional development symlink bundle. The raw-binary board uses no assets from
that bundle. Other target configurations and non-Windows builds keep QEMU's
original post-configuration step.

## Graphics and input update

Run `run.bat graphics.asm` from the project root. The first graphical run rebuilds
old bundled QEMU automatically using MSYS2. To force rebuilding, use
`run.bat graphics.asm -BuildQemu`. This update changes source code; the old bundled
EXE is not a rebuilt graphics executable. SDL2 and Pixman are installed by the
Windows build script and their runtime DLLs are included when packaging.

The default launch opens an SDL window and keeps it open after HLT. Use
`-Headless` for the previous no-window, exit-on-halt behavior. `-Keyboard` still
provides serial character input; window keys provide QEMU numeric keycodes.

Text mode uses word addresses 63534–65533, 80 columns × 25 rows, row-major.
Each word contains a CP437 character in bits 0–7, foreground color in bits 8–11,
and background color in bits 12–15. For example, `0x1F41` is a bright-white A
on blue. All 16 VGA colors are supported; bit 15 selects bright background.
The screen is 640×400 pixels with QEMU's 8×16 VGA font. This is a text framebuffer,
not an x86 VGA port/register device; graphics modes and hardware cursor/blink
are not implemented.

Keyboard press/release events enter vector 0: EAX contains the QEMU numeric
keycode, with bit 15 set on release. Mouse events enter vector 1: EAX is the
button mask (left=1, right=2, middle=4), EBX/ECX are signed relative X/Y deltas.
Click the SDL window to focus it; use Ctrl+Alt+G to grab/release relative mouse
input. The demo updates event counters and shows the last keycode glyph/buttons.
Guest handlers must install their IDT entries and enable interrupts with SETIRR.
HLT permanently stops this ISA's CPU, so interactive programs use a wait loop.
