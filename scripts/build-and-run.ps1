[CmdletBinding()]
param(
    [Parameter(Position=0)][string]$Source = 'main.asm',
    [switch]$BuildOnly,
    [switch]$BuildQemu,
    [switch]$RebuildCompiler,
    [switch]$Headless,
    [switch]$Keyboard,
    [switch]$Monitor,
    [string]$CompilerPath,
    [string]$QemuPath,
    [string]$Msys2Root = 'C:\msys64',
    [string]$QemuWorkDir = 'C:\xbc16-build',
    [ValidateRange(1,256)][int]$Jobs = 4
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$src = Join-Path $root 'src'
$toolDir = Join-Path $root 'build\tools'
$programDir = Join-Path $root 'build\programs'
$pending = $null

function Invoke-Checked([string]$Command, [string[]]$Arguments, [string]$Description) {
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Description failed (exit code $LASTEXITCODE)." }
}
function Get-SourcePath {
    if ([IO.Path]::IsPathRooted($Source)) { $candidates = @($Source) }
    else { $candidates = @((Join-Path $src $Source), (Join-Path $root $Source)) }
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).ProviderPath
        }
    }
    throw "Source file not found: $Source. Put programs in src, or provide an absolute path."
}
function Ensure-Qemu {
    if ($QemuPath) {
        if (-not (Test-Path -LiteralPath $QemuPath -PathType Leaf)) { throw "QEMU executable not found: $QemuPath" }
        return (Resolve-Path -LiteralPath $QemuPath).ProviderPath
    }
    $exe = Join-Path $root 'qemu-xbc16\dist\windows-x64\qemu-system-xbc16.exe'
    $versionFile = Join-Path (Split-Path $exe -Parent) 'graphics-version.txt'
    $graphicsReady = (Test-Path $versionFile) -and ((Get-Content -LiteralPath $versionFile -Raw).Trim() -eq '2')
    if ($BuildQemu -or -not (Test-Path -LiteralPath $exe -PathType Leaf) -or (-not $graphicsReady)) {
        Write-Host 'Building the native Windows 16BC QEMU target (first build can take a while)...'
        $builder = Join-Path $root 'qemu-xbc16\windows\build.ps1'
        $log = Join-Path $root 'build\logs\qemu-build.log'
        # Invoke the script directly, preserving its underlying exception detail.
        $builderArgs = @{ 'Msys2Root'=$Msys2Root; 'WorkDir'=$QemuWorkDir; 'Jobs'=$Jobs; 'LogPath'=$log }
        & $builder @builderArgs

    }
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "QEMU build did not produce $exe" }
    return $exe
}
function Ensure-Compiler {
    if ($CompilerPath) {
        if (-not (Test-Path -LiteralPath $CompilerPath -PathType Leaf)) { throw "Compiler executable not found: $CompilerPath" }
        return (Resolve-Path -LiteralPath $CompilerPath).ProviderPath
    }
    $exe = Join-Path $toolDir 'xbc.exe'
    $compilerSources = Join-Path $root 'Xbc Compiler\src'
    $inputs = @(Get-ChildItem -LiteralPath $compilerSources -Recurse -File |
        Where-Object { $_.Extension -in '.cpp','.h','.hpp' })
    $inputs += Get-ChildItem -LiteralPath (Join-Path $root 'common') -Recurse -File
    $needsBuild = $RebuildCompiler -or -not (Test-Path -LiteralPath $exe -PathType Leaf)
    if (-not $needsBuild) {
        $binaryTime = (Get-Item -LiteralPath $exe).LastWriteTimeUtc
        $needsBuild = @($inputs | Where-Object { $_.LastWriteTimeUtc -gt $binaryTime }).Count -gt 0
    }
    if ($needsBuild) {
        $gxx = Join-Path $Msys2Root 'mingw64\bin\g++.exe'
        if (-not (Test-Path -LiteralPath $gxx -PathType Leaf)) {
            throw 'MinGW C++ compiler is missing. Run setup.bat with MSYS2 installed, or provide -CompilerPath to a compiler built in Visual Studio.'
        }
        [IO.Directory]::CreateDirectory($toolDir) | Out-Null
        $cpp = @($inputs | Where-Object { $_.Extension -eq '.cpp' } | ForEach-Object { $_.FullName })
        if ($cpp.Count -eq 0) { throw 'No compiler C++ sources were found.' }
        Write-Host 'Building xbc.exe from your compiler project sources...'
        # Static MinGW runtime libraries keep xbc.exe independent of the MSYS2 PATH.
        $arguments = @('-std=c++20','-O2','-Wall','-Wextra','-static') + $cpp + @('-o',$exe)
        $oldPath = $env:PATH
        try {
            $env:PATH = (Join-Path $Msys2Root 'mingw64\bin') + ';' + $oldPath
            Invoke-Checked $gxx $arguments 'Compiler build'
        } finally { $env:PATH = $oldPath }
    }
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Compiler build produced no executable.' }
    return $exe
}

try {
    if ($Keyboard -and $Monitor) { throw 'Choose -Keyboard or -Monitor, since both use the console.' }
    $sourceFile = Get-SourcePath
    $extension = [IO.Path]::GetExtension($sourceFile).ToLowerInvariant()
    if ($extension -in '.asm','.s') {
        $mode = 'asm'
        $directive = Get-Content -LiteralPath $sourceFile |
            ForEach-Object { ($_ -split ';',2)[0].Trim() } |
            Where-Object { $_ } | Select-Object -First 1
        if ($directive -ne '.16bc') { throw 'This QEMU target requires .16bc assembly. Put .16bc on the first non-comment line.' }
    } elseif ($extension -eq '.c') {
        $mode = 'c'
    } else { throw 'Supported source files: .asm, .s, and .c (when C compilation is implemented).' }

    $qemu = $null
    if (-not $BuildOnly -or $BuildQemu) { $qemu = Ensure-Qemu }
    $compiler = Ensure-Compiler
    $srcPrefix = [IO.Path]::GetFullPath($src).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
    if ($sourceFile.StartsWith($srcPrefix,[StringComparison]::OrdinalIgnoreCase)) {
        $relative = $sourceFile.Substring($srcPrefix.Length)
    } else { $relative = [IO.Path]::GetFileName($sourceFile) }
    $binary = Join-Path $programDir ([IO.Path]::ChangeExtension($relative,'.bin'))
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($binary)) | Out-Null
    $pending = $binary + '.pending-' + [Guid]::NewGuid().ToString('N')
    Write-Host "Compiling $sourceFile ($mode)..."
    Invoke-Checked $compiler @($sourceFile,$mode,$pending) 'Source compilation'
    if (-not (Test-Path -LiteralPath $pending -PathType Leaf)) { throw 'The compiler reported success but produced no binary.' }
    $length = (Get-Item -LiteralPath $pending).Length
    if ($length -lt 2 -or $length % 2 -ne 0 -or $length -gt 131072) {
        throw 'The output must contain 1..65536 complete 16-bit words.'
    }
    Move-Item -LiteralPath $pending -Destination $binary -Force
    $pending = $null
    Write-Host "Binary: $binary"
    if ($BuildOnly) { exit 0 }

    $machine = if ($Headless) { 'xbc16,exit-on-halt=on' } else { 'xbc16' }
    $display = if ($Headless) { 'none' } else { 'sdl' }
    $qemuArgs = @('-M',$machine,'-display',$display,'-kernel',$binary)
    if ($Monitor) { $qemuArgs[1] = 'xbc16'; $qemuArgs += @('-serial','none','-monitor','stdio') }
    elseif ($Keyboard) { $qemuArgs += @('-serial','stdio','-monitor','none') }
    else { $qemuArgs += @('-serial','none','-monitor','none') }
    Write-Host 'Running 16BC in QEMU...'
    Invoke-Checked $qemu $qemuArgs 'QEMU simulation'
    exit 0
} catch {
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    if ($pending -and [IO.File]::Exists($pending)) { [IO.File]::Delete($pending) }
}
