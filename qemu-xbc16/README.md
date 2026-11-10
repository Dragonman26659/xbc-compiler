# Revision 2 interrupt ABI

Keyboard, mouse, and timer now push PC and R14–R1: 15 words total. Rebuild QEMU and update handlers together. See [the current specification](../XBC-SPEC-v2.md).

# Native QEMU target for 16BC

This extension adds a real `xbc16-softmmu` CPU target and an `xbc16` machine to
QEMU 9.2.0. It produces `qemu-system-xbc16`, which boots raw binaries from the
updated assembler directly. No RISC-V/x86 CPU or guest interpreter executable
is involved. QEMU's normal `-plugin` interface instruments existing targets;
this extension is compiled into QEMU as a new architecture.

## Native Windows x64 build

A Windows configuration is included under `windows/`. Install and update
[MSYS2](https://www.msys2.org/), then run from this extension folder:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\windows\build.ps1
.\windows\run.cmd "C:\path\program.bin"
```

The build script installs the MINGW64 dependencies, applies this target to the
pinned QEMU source, builds `qemu-system-xbc16.exe`, runs the boot tests and
packages its runtime DLLs. The resulting EXE and DLLs are native Windows files.
See [windows/README.md](windows/README.md) for full setup, options and output paths.

The Windows scripts and DLL dependency resolver were checked here; an actual
Windows build/runtime was not available to verify. Linux QEMU tests still pass.

## Build (Linux or Windows via WSL)

The source overlay is pinned to **QEMU v9.2.0**, commit
`ae35f033b874c627d81d51070187fbf55f0bf1a7`. The installer rejects other versions.
On Ubuntu/WSL install ordinary QEMU build dependencies:

```sh
sudo apt install git build-essential python3 python3-venv python3-pip \
    pkg-config libglib2.0-dev libpixman-1-dev libsdl2-dev ninja-build

git clone --depth 1 --branch v9.2.0 https://gitlab.com/qemu-project/qemu.git
python3 qemu-xbc16/apply.py qemu
mkdir qemu/build
cd qemu/build
../configure --target-list=xbc16-softmmu --disable-docs --disable-tools \
    --disable-guest-agent --disable-slirp --disable-gtk --enable-sdl --enable-pixman \
    --disable-vnc --disable-curses --disable-opengl --disable-virglrenderer \
    --disable-capstone --disable-plugins --disable-rust
ninja qemu-system-xbc16
```

Keep the downloaded extension folder named `qemu-xbc16`, next to the QEMU source
folder for these example paths. Configure may download Python build dependencies
and QEMU subprojects. There is no separate binary plugin to load into a stock
QEMU installation. Native Windows uses the MSYS2/MinGW configuration above. MSVC projects are not
provided for QEMU; WSL remains an alternative using the tested Linux toolchain. The full upstream QEMU source is not duplicated in this ZIP.

## Boot your binary

```sh
./qemu-system-xbc16 -M xbc16,exit-on-halt=on \
    -display none -serial none -monitor none -kernel /path/to/program.bin
```

Or boot the included example from the build directory:

```sh
./qemu-system-xbc16 -M xbc16,exit-on-halt=on \
    -display none -serial none -monitor none \
    -kernel ../../qemu-xbc16/examples/boot.bin
```

The example adds 40 + 2, pushes/pops the result and halts. Its final dump includes
`R1=0x002a`, `R3=0x002a`, `R16=0xf72c`. `exit-on-halt=on` prints 16BC state and
exits successfully on HLT. It defaults to off, keeping QEMU open with the CPU
halted. HLT is permanent until reset, matching your simulator.

Input is a headerless sequence of little-endian 16-bit words, not an x86 boot
sector, ELF, disk image or operating-system container. QEMU loads it at physical
byte address 0, corresponding to **word address 0**, and starts PC there.
Binaries must contain 1–65536 complete words; empty, odd-length and oversized
files are rejected. The loader can initialize the whole RAM image, including
IDT entries. Memory is zero-initialized on initial boot; QEMU system reset
restores the original boot image and resets CPU/device state. RAM outside the
boot image retains its contents on reset, following QEMU's ordinary RAM behavior.

To regenerate the supplied examples with your compiler:

```sh
xbc examples/boot.asm asm examples/boot.bin
xbc examples/keyboard.asm asm examples/keyboard.bin
```

## Keyboard and mouse

For character input, attach QEMU's first serial frontend:

```sh
./qemu-system-xbc16 -M xbc16,exit-on-halt=on \
    -display none -monitor none -serial stdio \
    -kernel ../../qemu-xbc16/examples/keyboard.bin
```

The keyboard example installs vector 0, enables interrupts and waits. A byte
from stdin becomes a keyboard interrupt with that byte in EAX; its handler
halts. Serial input supplies bytes, so UTF-8 characters may produce multiple
interrupts. Display RAM is rendered in the graphical console.

The board also registers QEMU keyboard and relative-mouse input handlers. QMP
`input-send-event` works without a display. QEMU numeric keyboard codes are
passed in EAX; bit 15 indicates a key release. The serial path emits press bytes
only. Mouse EAX has button bits left=1, right=2, middle=4; EBX/ECX contain signed
horizontal/vertical deltas. Movements in one frontend sync batch coalesce and
clamp to signed 16-bit range. Wheel and absolute-pointer events are not handled.
The guest must initialize its IDT handlers and enable device interrupts.

Device events use a 64-entry FIFO. Timer interrupts coalesce while pending.
The serial frontend applies backpressure when the queue is full. QMP input
reports a dropped-input diagnostic rather than overwriting earlier events;
timer enqueue failure stops the CPU with a diagnostic. CPU execution and host
input access are serialized using QEMU's Big QEMU Lock.

## Architectural behavior

The ISA, flags, binary16 behavior and memory conventions match the updated
simulator supplied earlier. Registers and ACC are 16-bit; PC/SP are word
addresses. QEMU internally uses 32-bit byte addresses for its TCG/MMU machinery;
this does not extend the guest address space. For QEMU monitor memory commands,
guest word address N is byte address **2*N**.

| Region | Inclusive word addresses |
|---|---:|
| Code/data/heap | 0–59179 |
| Stack, 4096 words | 59180–63275 |
| Reserved gap | 63276–63277 |
| IDT, 256 words | 63278–63533 |
| Display RAM, 2000 words | 63534–65533 |
| Reserved | 65534 |
| Timer period | 65535 |

These boundaries retain the earlier documented choices for the contradictory
endpoints in your original specification. R16 starts at 63276. PUSH pre-decrements;
POP reads and post-increments. Normal stack-region accesses/fetches fault.
Memory/IDT/display/timer writes are otherwise allowed. R15 latches RRAM results.

Legacy opcode words decode from their low eight bits; 16BC extensions use the
`0xF` prefix. HLT is exactly `0xFFFF`. ALU operations update ACC/flags; conversions
preserve them. Shifts update the operand register plus ACC/flags. Binary16 uses
round-to-nearest, ties-to-even. Float-to-integer conversion truncates and rejects
non-finite/out-of-range values. Integer and float division by zero dispatch vector 6.

| Interrupt | Vector |
|---|---:|
| Keyboard | 0 |
| Mouse | 1 |
| Timer | 2 |
| SegFault | 3 |
| Stack overflow | 4 |
| Stack underflow | 5 |
| Divide by zero | 6 |

Device entry saves next PC, then R14–R1, and clears device-interrupt enable.
The saved R16 is SP immediately after pushing PC, matching the updated simulator.
Error entry saves faulting PC, then old EAX, loads EAX with the vector/error code,
and enters the handler even if device interrupts are masked. Frame validation
is atomic. A full stack cannot fit a fault frame; execution stops with an error
instead of corrupting memory. Invalid opcodes/conversions/IDT targets likewise
stop QEMU with exit code 1. Handler register/stack restoration remains the guest's
responsibility; there is no IRET opcode in the specified ISA.

The timer counts guest instructions, not host wall-clock milliseconds. Zero
at word 65535 disables it. ACC/flags are not saved in hardware interrupt frames.
The assembler's R14/R15 scratch conventions and interrupt-return limitations
are unchanged; see the updated project's README.

## Implementation and debugging

`target/xbc16/translate.c` emits one TCG helper call per instruction and returns
control to QEMU after each step. The helper fetches live words, uses QEMU's memory
APIs, executes the shared C core and applies interrupt/timer effects. This is a
correctness-focused native CPU target, with interpreter-style instruction
execution inside a TCG helper. It does not translate each ALU operation into
separate optimized TCG operations. Self-modifying code is supported through
QEMU's TB invalidation.

The board loads `-kernel`, provides fixed RAM and input frontends, and supports
QEMU pause/resume/reset and HMP `info registers`. Example interactive monitor:

```sh
./qemu-system-xbc16 -display none -serial none -monitor stdio \
    -kernel /path/to/program.bin
# At the monitor: info registers
#                xp /16hx 0
```

The register dump reports both PC word and byte addresses. Raw GDB remote hooks
expose R1–R16 (2 bytes each), ACC (2), byte PC (4) and flags (2), little endian.
Stock GDB has no XBC16 architecture definition, so a normal typed GDB session
is not provided. Migration/snapshots are explicitly blocked until CPU state
serialization is implemented. There is one CPU, no user-mode ABI, an 80×25 text display and no KVM acceleration for this custom ISA. `-icount` and deterministic
record/replay were not validated.

## Tests and verification

From the extension folder:

```sh
make test
python3 tests/smoke.py /path/to/qemu/build/qemu-system-xbc16
# Optional: compare against your updated standalone simulator
python3 tests/differential.py /path/to/xbc-sim
```

Validated here against a compiled QEMU 9.2.0 target with GCC 13.3 on Linux:

- Binary boot, final registers, halt/exit and malformed-file rejection.
- Integer/binary16 operations, RAM load/store, and self-modifying code.
- Fault frames, timer delivery and QMP keyboard/mouse delivery.
- Standalone core tests under undefined-behavior sanitization.
- Encode/decode round trips for every non-NaN binary16 bit pattern.
- 100 randomized programs compared with the updated C++ simulator.

`apply.py` copies the new files and updates six upstream registration/build
files. It validates its anchors and is idempotent. Install it into a clean,
isolated QEMU checkout before building.

Primary QEMU references:

- https://www.qemu.org/docs/master/devel/build-system.html
- https://www.qemu.org/docs/master/devel/tcg-plugins.html
- https://gitlab.com/qemu-project/qemu/-/tree/v9.2.0/target/avr

The extension is licensed GPL-2.0-or-later, compatible with integration into
QEMU. See LICENSE and the upstream QEMU COPYING file.

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
