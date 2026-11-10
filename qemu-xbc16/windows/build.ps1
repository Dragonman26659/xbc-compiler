# SPDX-License-Identifier: GPL-2.0-or-later
[CmdletBinding()]
param(
    [string]$Msys2Root = 'C:\msys64',
    [string]$WorkDir = 'C:\xbc16-build',
    [ValidateRange(1,256)][int]$Jobs = 4,
    [string]$LogPath
)
$ErrorActionPreference = 'Stop'
if (-not $LogPath) { $LogPath = Join-Path $PSScriptRoot 'build-windows.log' }
$LogPath = [IO.Path]::GetFullPath($LogPath)
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($LogPath)) | Out-Null
Set-Content -LiteralPath $LogPath -Value ("16BC Windows build: " + [DateTime]::Now.ToString('s')) -Encoding UTF8
function Write-BuildLine([string]$Line) {
    Add-Content -LiteralPath $LogPath -Value $Line -Encoding UTF8
    Write-Host $Line
}
$oldMsystem = $env:MSYSTEM
$oldChere = $env:CHERE_INVOKING
$originalPreference = $ErrorActionPreference
try {
    Write-BuildLine "Log: $LogPath"
    Write-BuildLine "MSYS2: $Msys2Root"
    Write-BuildLine "Workspace: $WorkDir"
    $bash = Join-Path $Msys2Root 'usr\bin\bash.exe'
    $cygpath = Join-Path $Msys2Root 'usr\bin\cygpath.exe'
    foreach ($tool in @($bash,$cygpath)) {
        if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) {
            throw "Missing MSYS2 tool: $tool. Install MSYS2 from https://www.msys2.org/ or pass -Msys2Root with its actual location."
        }
    }
    $WorkDir = [IO.Path]::GetFullPath($WorkDir)
    if ($WorkDir -match '\s') { throw 'Use a build directory without spaces, such as C:\xbc16-build.' }
    $script = Join-Path $PSScriptRoot 'build.sh'
    # Windows editors may save shell files with CRLF/BOM. Bash expects LF here.
    $text = [IO.File]::ReadAllText($script).Replace("`r`n","`n").TrimStart([char]0xFEFF)
    [IO.File]::WriteAllText($script,$text,(New-Object Text.UTF8Encoding($false)))
    $env:MSYSTEM = 'MINGW64'
    $env:CHERE_INVOKING = '1'
    $msysScript = & $cygpath -u $script
    if ($LASTEXITCODE -ne 0) { throw "cygpath could not convert script path: $script" }
    $msysWork = & $cygpath -u $WorkDir
    if ($LASTEXITCODE -ne 0) { throw "cygpath could not convert workspace: $WorkDir" }
    Write-BuildLine 'Starting MSYS2 MINGW64 build...'
    # Windows PowerShell 5 treats native stderr as ErrorRecords. Capture it
    # without terminating early or losing the native program's exit status.
    $ErrorActionPreference = 'Continue'
    & $bash --login $msysScript $msysWork "$Jobs" 2>&1 |
        ForEach-Object { Write-BuildLine ($_.ToString()) }
    $status = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($status -ne 0) { throw "MSYS2/QEMU build exited with code $status." }
    Write-BuildLine 'Windows QEMU build and tests completed successfully.'
} catch {
    $ErrorActionPreference = 'Stop'
    Write-BuildLine ("ERROR: " + $_.Exception.Message)
    throw "$($_.Exception.Message) Full build log: $LogPath"
} finally {
    $env:MSYSTEM = $oldMsystem
    $env:CHERE_INVOKING = $oldChere
    $ErrorActionPreference = $originalPreference
}
