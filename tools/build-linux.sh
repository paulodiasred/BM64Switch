#!/bin/bash
# Linux test build (same renderer and code as the Switch build). Runs inside the host Docker image.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
if [ ! -f build-linux/build.ninja ]; then
    cmake -G Ninja -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
fi
cmake --build build-linux --target BM64Recompiled -j"$(nproc)"
