# DiskScope build helper (Windows PowerShell).
#
# Why this script exists: the machine has no system CMake/Ninja, and the GCC
# installation at E:/Compilers/mingw64 only works when its own bin directory is
# on PATH (its cc1plus.exe needs the runtime DLLs found there). This script
# restores that PATH, then configures, builds and optionally tests.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Test -Smoke
param(
    [switch]$Test,
    [switch]$Smoke,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$root         = Split-Path -Parent $PSScriptRoot
$toolchainBin = if ($env:DISKSCOPE_TOOLCHAIN_BIN) { $env:DISKSCOPE_TOOLCHAIN_BIN } else { 'E:\Compilers\mingw64\bin' }
$cmake        = Join-Path $root '.tools\cmake\bin\cmake.exe'
$ctest        = Join-Path $root '.tools\cmake\bin\ctest.exe'
$ninja        = Join-Path $root '.tools\ninja\ninja.exe'
$buildDir     = Join-Path $root 'build'

$env:PATH = "$toolchainBin;$env:PATH"

if (-not (Test-Path $cmake)) { throw "missing $cmake - run scripts\setup_toolchain.ps1 first" }
if (-not (Test-Path $ninja)) { throw "missing $ninja - run scripts\setup_toolchain.ps1 first" }

if ($Clean -and (Test-Path $buildDir)) { Remove-Item -Recurse -Force $buildDir }

& $cmake -S $root -B $buildDir -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER="$toolchainBin\gcc.exe" `
    -DCMAKE_CXX_COMPILER="$toolchainBin\g++.exe" `
    -DCMAKE_MAKE_PROGRAM="$ninja"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $cmake --build $buildDir --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Test -or $Smoke) {
    Push-Location $buildDir
    try {
        & $ctest --output-on-failure
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    } finally { Pop-Location }
}

if ($Smoke) {
    & (Join-Path $buildDir 'DiskScope.exe') --smoke --capture (Join-Path $root 'docs\captures')
    exit $LASTEXITCODE
}
