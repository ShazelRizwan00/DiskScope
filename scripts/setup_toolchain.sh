#!/usr/bin/env bash
# Provisions the portable build tools used by DiskScope into .tools/.
# The machine has no system CMake/Ninja, so they are downloaded as portable
# archives instead of being installed (nothing outside .tools/ is touched).
#
#   .tools/cmake/  CMake 4.5.1 (windows-x86_64)
#   .tools/ninja/  Ninja 1.13.2
#
# The C++ compiler itself is NOT downloaded: DiskScope uses the GCC 15.2.0
# installation already present at E:/Compilers/mingw64 (override with
# DISKSCOPE_TOOLCHAIN_BIN). That compiler only works with its own bin directory
# on PATH, which scripts/build.sh sets up.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLS="$ROOT/.tools"
CMAKE_VERSION="${CMAKE_VERSION:-4.5.1}"
NINJA_VERSION="${NINJA_VERSION:-1.13.2}"
CMAKE_ZIP_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-windows-x86_64.zip"
NINJA_ZIP_URL="https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-win.zip"

mkdir -p "$TOOLS"

if [ ! -x "$TOOLS/cmake/bin/cmake.exe" ]; then
    echo "downloading CMake ${CMAKE_VERSION}..."
    tmp="$(mktemp -d)"
    curl -fsSL --retry 3 -o "$tmp/cmake.zip" "$CMAKE_ZIP_URL"
    rm -rf "$TOOLS/cmake" "$TOOLS/.cmake-extract"
    mkdir -p "$TOOLS/.cmake-extract"
    unzip -q "$tmp/cmake.zip" -d "$TOOLS/.cmake-extract"
    mv "$TOOLS/.cmake-extract/cmake-${CMAKE_VERSION}-windows-x86_64" "$TOOLS/cmake"
    rmdir "$TOOLS/.cmake-extract"
    rm -rf "$tmp"
else
    echo "CMake already present."
fi

if [ ! -x "$TOOLS/ninja/ninja.exe" ]; then
    echo "downloading Ninja ${NINJA_VERSION}..."
    tmp="$(mktemp -d)"
    curl -fsSL --retry 3 -o "$tmp/ninja.zip" "$NINJA_ZIP_URL"
    rm -rf "$TOOLS/ninja"
    mkdir -p "$TOOLS/ninja"
    unzip -q "$tmp/ninja.zip" -d "$TOOLS/ninja"
    rm -rf "$tmp"
else
    echo "Ninja already present."
fi

"$TOOLS/cmake/bin/cmake.exe" --version | head -n 1
"$TOOLS/ninja/ninja.exe" --version
