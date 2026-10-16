#!/bin/bash
# Nintendo Switch build. Runs inside the Switch Docker image and puts the .nro (and the .elf, for symbolizing
# crash reports) in dist/.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
if [ ! -f build-switch/build.ninja ]; then
    cmake -G Ninja -S . -B build-switch -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake -DCMAKE_BUILD_TYPE=Release
fi
cmake --build build-switch -j"$(nproc)"
mkdir -p dist
cp build-switch/BM64Recompiled.nro build-switch/BM64Recompiled.elf dist/
echo "== dist/BM64Recompiled.nro"
