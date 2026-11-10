# SPDX-License-Identifier: GPL-2.0-or-later
[CmdletBinding()]
param(
    [Parameter(Position=0)][string]$Bin,
    [switch]$Headless,
    [switch]$Keyboard,
    [switch]$Monitor,
    [string]$QemuExe
)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
if (-not $QemuExe) {
    $QemuExe = Join-Path $root 'qemu-system-xbc16.exe'
    $versionFile = Join-Path (Split-Path $QemuExe -Parent) 'graphics-version.txt'
    $currentAbi = (Test-Path $versionFile) -and ((Get-Content -LiteralPath $versionFile -Raw).Trim() -eq '2')
    if (-not $currentAbi) {
        throw 'The bundled QEMU needs the revision 2 interrupt ABI update. Run windows\build.ps1 first, or run the project run.bat -BuildQemu.'
    }
}
if (-not (Test-Path -LiteralPath $QemuExe -PathType Leaf)) {
    throw 'Build first with windows\build.ps1, or pass -QemuExe with your native Windows executable path.'
}
if (-not $Bin) { $Bin = Join-Path $root 'examples\boot.bin' }
$QemuExe = (Resolve-Path -LiteralPath $QemuExe).ProviderPath
$binary = (Resolve-Path -LiteralPath $Bin).ProviderPath
if ((Get-Item -LiteralPath $binary).PSIsContainer) { throw 'Choose a binary file, not a directory.' }
$length = (Get-Item -LiteralPath $binary).Length
if ($length -eq 0 -or $length % 2 -ne 0 -or $length -gt 131072) {
    throw 'The binary must contain 1..65536 complete 16-bit words (2..131072 bytes).'
}
if ($Keyboard -and $Monitor) { throw 'Choose -Keyboard or -Monitor; both would claim the console.' }
$machine = if ($Headless) { 'xbc16,exit-on-halt=on' } else { 'xbc16' }
$display = if ($Headless) { 'none' } else { 'sdl' }
$qemuArgs = @('-M', $machine, '-display', $display, '-kernel', $binary)
if ($Monitor) {
    $qemuArgs[1] = 'xbc16'
    $qemuArgs += @('-serial', 'none', '-monitor', 'stdio')
} elseif ($Keyboard) {
    $qemuArgs += @('-serial', 'stdio', '-monitor', 'none')
} else {
    $qemuArgs += @('-serial', 'none', '-monitor', 'none')
}
& $QemuExe @qemuArgs
exit $LASTEXITCODE
