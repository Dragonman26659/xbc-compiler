#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -Eeuo pipefail
trap 'status=$?; printf "ERROR: build.sh line %s exited with status %s\n" "$LINENO" "$status" >&2; exit "$status"' ERR
if [[ "${MSYSTEM:-}" != MINGW64 ]]; then
    echo 'Open MSYS2 MINGW64, or run windows/build.ps1 from PowerShell.' >&2
    exit 1
fi
bundle=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
work=${1:-/c/xbc16-build}
jobs=${2:-4}
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || { echo 'Jobs must be a positive integer.' >&2; exit 1; }
[[ "$work" != *[[:space:]:]* ]] || { echo 'QEMU build paths must have no spaces or colons; use /c/xbc16-build.' >&2; exit 1; }
mkdir -p -- "$work"
work=$(cd -- "$work" && pwd)
# Update MSYS2 separately with pacman -Syu before running this script.
pacman -S --needed --noconfirm \
    base-devel git \
    mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils \
    mingw-w64-x86_64-glib2 mingw-w64-x86_64-pkgconf \
    mingw-w64-x86_64-SDL2 mingw-w64-x86_64-pixman \
    mingw-w64-x86_64-python mingw-w64-x86_64-python-pip mingw-w64-x86_64-python-distlib \
    mingw-w64-x86_64-meson mingw-w64-x86_64-ninja
# Meson runs under native Windows Python: compiler/tool paths must be native,
# not MSYS virtual paths such as /mingw64/bin/gcc.exe. Mixed C:/ paths work
# in both Bash and native subprocess creation.
export CC="$(cygpath -m "$MINGW_PREFIX/bin/gcc.exe")"
export CXX="$(cygpath -m "$MINGW_PREFIX/bin/g++.exe")"
export PKG_CONFIG="$(cygpath -m "$MINGW_PREFIX/bin/pkg-config.exe")"
export PYTHON="$MINGW_PREFIX/bin/python.exe"
if [[ ! -d "$work/qemu" ]]; then
    git -c core.autocrlf=false clone --depth 1 --branch v9.2.0 https://gitlab.com/qemu-project/qemu.git "$work/qemu"
fi
actual=$(git -C "$work/qemu" rev-parse HEAD)
[[ "$actual" == ae35f033b874c627d81d51070187fbf55f0bf1a7 ]] || {
    echo "Expected the QEMU v9.2.0 commit; found $actual. Use another build directory." >&2; exit 1;
}
"$PYTHON" "$bundle/apply.py" "$work/qemu"
# QEMU's meson/pycotap bootstrap is forced offline even with downloads enabled.
# Preload pycotap through a direct wheel path, avoiding MinGW file-URL handling.
"$PYTHON" "$bundle/windows/bootstrap.py" \
    --source "$(cygpath -w "$work/qemu")" --target "$(cygpath -w "$work/python-deps")"
export PYTHONPATH="$(cygpath -w "$work/python-deps")${PYTHONPATH:+;$PYTHONPATH}"
mkdir -p -- "$work/build-windows"
cd -- "$work/build-windows"
"$work/qemu/configure" --target-list=xbc16-softmmu \
    --python="$PYTHON" --cc="$CC" --cxx="$CXX" \
    --disable-docs --disable-tools --disable-guest-agent --disable-slirp \
    --disable-gtk --enable-sdl --disable-vnc --disable-curses \
    --disable-opengl --disable-virglrenderer --disable-capstone \
    --disable-plugins --disable-rust --enable-pixman
"$MINGW_PREFIX/bin/ninja.exe" -j "$jobs" qemu-system-xbc16.exe
"$PYTHON" "$bundle/tests/smoke.py" "$work/build-windows/qemu-system-xbc16.exe"
"$PYTHON" "$bundle/tests/graphics_smoke.py" "$work/build-windows/qemu-system-xbc16.exe"
"$PYTHON" "$bundle/windows/package.py" \
    --exe "$work/build-windows/qemu-system-xbc16.exe" \
    --runtime "$(cygpath -w "$MINGW_PREFIX/bin")" \
    --licenses "$(cygpath -w "$MINGW_PREFIX/share/licenses")" \
    --output "$bundle/dist/windows-x64"
# Verify bundled DLL resolution, independent of the MSYS2 runtime PATH.
"$PYTHON" "$bundle/tests/smoke.py" "$bundle/dist/windows-x64/qemu-system-xbc16.exe" --isolated-runtime
printf '\nBuilt Windows executable and portable folder: %s/dist/windows-x64\n' "$bundle"
