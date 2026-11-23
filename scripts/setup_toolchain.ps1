# Provisions the portable build tools used by DiskScope into .tools\.
# The machine has no system CMake/Ninja, so they are downloaded as portable
# archives instead of being installed (nothing outside .tools\ is touched).
#
#   .tools\cmake\  CMake 4.5.1 (windows-x86_64)
#   .tools\ninja\  Ninja 1.13.2
#
# The C++ compiler is NOT downloaded: DiskScope uses the GCC 15.2.0 installation
# already present at E:\Compilers\mingw64 (override with DISKSCOPE_TOOLCHAIN_BIN).
# That compiler only works with its own bin directory on PATH, which
# scripts\build.ps1 sets up.
$ErrorActionPreference = 'Stop'

$root         = Split-Path -Parent $PSScriptRoot
$tools        = Join-Path $root '.tools'
$cmakeVersion = if ($env:CMAKE_VERSION) { $env:CMAKE_VERSION } else { '4.5.1' }
$ninjaVersion = if ($env:NINJA_VERSION) { $env:NINJA_VERSION } else { '1.13.2' }

New-Item -ItemType Directory -Force -Path $tools | Out-Null

if (-not (Test-Path (Join-Path $tools 'cmake\bin\cmake.exe'))) {
    $zip = Join-Path $env:TEMP "cmake-$cmakeVersion.zip"
    $url = "https://github.com/Kitware/CMake/releases/download/v$cmakeVersion/cmake-$cmakeVersion-windows-x86_64.zip"
    Write-Host "downloading CMake $cmakeVersion..."
    Invoke-WebRequest -Uri $url -OutFile $zip
    $extract = Join-Path $tools '.cmake-extract'
    if (Test-Path $extract) { Remove-Item -Recurse -Force $extract }
    Expand-Archive -Path $zip -DestinationPath $extract -Force
    if (Test-Path (Join-Path $tools 'cmake')) { Remove-Item -Recurse -Force (Join-Path $tools 'cmake') }
    Move-Item (Join-Path $extract "cmake-$cmakeVersion-windows-x86_64") (Join-Path $tools 'cmake')
    Remove-Item -Recurse -Force $extract
    Remove-Item -Force $zip
} else {
    Write-Host 'CMake already present.'
}

if (-not (Test-Path (Join-Path $tools 'ninja\ninja.exe'))) {
    $zip = Join-Path $env:TEMP "ninja-$ninjaVersion.zip"
    $url = "https://github.com/ninja-build/ninja/releases/download/v$ninjaVersion/ninja-win.zip"
    Write-Host "downloading Ninja $ninjaVersion..."
    Invoke-WebRequest -Uri $url -OutFile $zip
    if (Test-Path (Join-Path $tools 'ninja')) { Remove-Item -Recurse -Force (Join-Path $tools 'ninja') }
    Expand-Archive -Path $zip -DestinationPath (Join-Path $tools 'ninja') -Force
    Remove-Item -Force $zip
} else {
    Write-Host 'Ninja already present.'
}

& (Join-Path $tools 'cmake\bin\cmake.exe') --version | Select-Object -First 1
& (Join-Path $tools 'ninja\ninja.exe') --version
