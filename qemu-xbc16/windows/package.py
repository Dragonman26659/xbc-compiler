#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Collect a native MinGW executable and its recursive non-system DLL imports."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

SYSTEM_DLLS = {
    'kernel32.dll','user32.dll','gdi32.dll','ws2_32.dll','advapi32.dll',
    'shell32.dll','ole32.dll','oleaut32.dll','version.dll','bcrypt.dll',
    'crypt32.dll','ntdll.dll','msvcrt.dll','ucrtbase.dll','shlwapi.dll',
    'secur32.dll','imm32.dll','winmm.dll','iphlpapi.dll','comdlg32.dll',
    'setupapi.dll','winspool.drv','psapi.dll','dnsapi.dll','rpcrt4.dll',
    'normaliz.dll','winhttp.dll','dwmapi.dll','mswsock.dll','d3d11.dll',
    'dxgi.dll','netapi32.dll','userenv.dll',
}

def imports(binary, objdump):
    result=subprocess.run([str(objdump),'-p',str(binary)],check=True,
                          capture_output=True,text=True,errors='replace')
    return re.findall(r'^\s*DLL Name:\s*(\S+)\s*$',result.stdout,re.M)

def collect(exe, runtime, inspect):
    """Resolve by exact basename case-insensitively; never copy Windows DLLs."""
    available={p.name.casefold():p for p in runtime.iterdir() if p.is_file() and p.suffix.casefold()=='.dll'}
    pending=[exe];seen={exe.name.casefold()};resolved=[]
    system=Path(os.environ.get('SystemRoot','C:\\Windows'))/'System32'
    while pending:
        for name in inspect(pending.pop()):
            lower=name.casefold()
            if '/' in name or '\\' in name or name in ('.','..'):
                raise ValueError(f'Unsafe import basename: {name}')
            if lower in seen:continue
            seen.add(lower)
            if lower in SYSTEM_DLLS or lower.startswith(('api-ms-win-','ext-ms-win-')):
                continue
            if lower in available:
                dependency=available[lower];resolved.append(dependency);pending.append(dependency)
            elif (system/name).is_file():
                continue
            else:
                raise FileNotFoundError(f'Missing runtime dependency {name}; searched {runtime}')
    return sorted(resolved,key=lambda p:p.name.casefold())

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for flag in ('exe','runtime','output'):parser.add_argument('--'+flag,type=Path,required=True)
    parser.add_argument('--objdump',type=Path)
    parser.add_argument('--licenses',type=Path)
    args=parser.parse_args()
    exe=args.exe.resolve();runtime=args.runtime.resolve();output=args.output.resolve()
    if not exe.is_file() or exe.read_bytes()[:2]!=b'MZ':parser.error('Expected a native Windows PE executable, not a Linux binary.')
    objdump=args.objdump or runtime/'objdump.exe'
    if not objdump.is_file():parser.error(f'Cannot find MinGW objdump: {objdump}')
    dependencies=collect(exe,runtime,lambda path:imports(path,objdump))
    # Validate all DLL imports before writing the destination.
    output.mkdir(parents=True,exist_ok=True)
    bundle=Path(__file__).resolve().parents[1];written=[]
    def copy(source,relative):
        dest=output/relative;dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source,dest);written.append(dest)
    copy(exe,'qemu-system-xbc16.exe')
    (output/'graphics-version.txt').write_text('2\n',encoding='ascii')
    written.append(output/'graphics-version.txt')
    for dll in dependencies:copy(dll,dll.name)
    for name in ('boot.bin','keyboard.bin','graphics.bin'):copy(bundle/'examples'/name,Path('examples')/name)
    copy(bundle/'COPYING','COPYING');copy(bundle/'LICENSE','LICENSE')
    copy(bundle/'windows/run.cmd','run.cmd')
    launcher=(bundle/'windows/run.ps1').read_text()
    launcher=launcher.replace('$root = Split-Path $PSScriptRoot -Parent','$root = $PSScriptRoot')
    launcher=launcher.replace("'dist\\windows-x64\\qemu-system-xbc16.exe'","'qemu-system-xbc16.exe'")
    (output/'run.ps1').write_text(launcher,encoding='utf-8');written.append(output/'run.ps1')
    if args.licenses and args.licenses.is_dir():
        for source in args.licenses.rglob('*'):
            if source.is_file():copy(source,Path('runtime-licenses')/source.relative_to(args.licenses))
    # Include extension sources and the pinned upstream source reference.
    for directory in ('overlay',):
        for source in (bundle/directory).rglob('*'):
            if source.is_file():copy(source,Path('source/xbc16')/source.relative_to(bundle))
    copy(bundle/'apply.py','source/xbc16/apply.py')
    (output/'README.txt').write_text(
        '16BC native Windows x64 QEMU 9.2.0\n\n'
        'Run the default example: run.cmd\n'
        'Run your program: run.cmd "C:\\path with spaces\\program.bin"\n'
        'Character input: run.cmd -Headless -Keyboard "examples\\keyboard.bin"\n'
        'Monitor: run.cmd -Monitor "examples\\boot.bin"\n\n'
        'Keep the EXE and DLL files together. MSYS2 is not needed to run this folder.\n'
        'SDL window: 80x25 VGA-style text, keyboard and relative mouse input.\n'
        'Graphics/input demo: run.cmd examples\\graphics.bin\n'
        'Headless: run.cmd -Headless examples\\boot.bin\n\n'
        'Upstream source: https://gitlab.com/qemu-project/qemu.git\n'
        'QEMU v9.2.0 commit: ae35f033b874c627d81d51070187fbf55f0bf1a7\n'
        'Extension sources are in source/xbc16. The full configured upstream\n'
        'source checkout is retained in the build workspace by build.sh.\n',encoding='utf-8')
    written.append(output/'README.txt')
    manifest={str(p.relative_to(output)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in written}
    (output/'files.sha256.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    written.append(output/'files.sha256.json')
    archive=output.parent/'qemu-xbc16-windows-x64.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as zipped:
        for path in sorted(written):zipped.write(path,path.relative_to(output))
    print(f'Packaged {len(dependencies)} runtime DLLs: {archive}')

if __name__=='__main__':main()
