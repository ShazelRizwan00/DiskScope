#!/usr/bin/env bash
# DiskScope build helper (Git Bash / MSYS2 shell).
#
# Why this script exists: the machine has no system CMake/Ninja, and the GCC
# installation at E:/Compilers/mingw64 only works when its own bin directory is
# on PATH (its cc1plus.exe needs the runtime DLLs found there). This script puts
# both in place, then configures, builds and (optionally) tests.
#
# Usage:
#   ./scripts/build.sh              # configure + build
#   ./scripts/build.sh --test       # configure + build + ctest
#   ./scripts/build.sh --smoke      # configure + build + tests + smoke/capture run
#   ./scripts/build.sh --clean      # wipe build/ first
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLCHAIN_BIN="${DISKSCOPE_TOOLCHAIN_BIN:-E:/Compilers/mingw64/bin}"
CMAKE="$ROOT/.tools/cmake/bin/cmake.exe"
NINJA="$ROOT/.tools/ninja/ninja.exe"
BUILD_DIR="$ROOT/build"

export PATH="$TOOLCHAIN_BIN:$PATH"

run_tests=0
run_smoke=0
for arg in "$@"; do
    case "$arg" in
        --test)  run_tests=1 ;;
        --smoke) run_tests=1; run_smoke=1 ;;
        --clean) rm -rf "$BUILD_DIR" ;;
        *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done

if [ ! -x "$CMAKE" ]; then
    echo "error: $CMAKE not found. Provision it with: ./scripts/setup_toolchain.sh" >&2
    exit 1
fi
if [ ! -x "$NINJA" ]; then
    echo "error: $NINJA not found. Provision it with: ./scripts/setup_toolchain.sh" >&2
    exit 1
fi

"$CMAKE" -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="$TOOLCHAIN_BIN/gcc.exe" \
    -DCMAKE_CXX_COMPILER="$TOOLCHAIN_BIN/g++.exe" \
    -DCMAKE_MAKE_PROGRAM="$NINJA"

"$CMAKE" --build "$BUILD_DIR" --parallel

if [ "$run_tests" -eq 1 ]; then
    "$ROOT/.tools/cmake/bin/ctest.exe" --test-dir "$BUILD_DIR" --output-on-failure || exit 1
fi

if [ "$run_smoke" -eq 1 ]; then
    "$BUILD_DIR/DiskScope.exe" --smoke --capture "$ROOT/docs/captures"
fi
