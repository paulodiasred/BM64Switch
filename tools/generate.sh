#!/bin/bash
# Generates the code that comes from the ROM and the patches. Runs inside the host Docker image (tools/build.ps1).
#
#   generate.sh <rom>   everything: decompresses the ROM, recompiles the game (RecompiledFuncs/), the audio microcode
#                       (rsp/aspMain.cpp) and the patches (RecompiledPatches/)
#   generate.sh patches only the patches, after editing patches/*.c (needs a previous full run)
#
# The ROM must be Bomberman 64 (USA), in any byte order (.z64, .n64 or .v64). Nothing generated here may be
# committed: it's all derived from the ROM.
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TOOLS="$ROOT/build-tools"
N64RECOMP_COMMIT=98bf104b1b5ed83126af8bcab0cc964782617dbf
BM64_DECOMP_COMMIT=c7c5fd5a73a9d5969673535030fc4f1df956fa5f
DECOMPRESSED_MD5=d2f9313cccae0af85d23ac37666c0173
DECOMPRESSED_ROM="$ROOT/bm64.decomp.us.z64"

cd "$ROOT"
mkdir -p "$TOOLS"

# N64Recomp/RSPRecomp, bm64decompress (from the Bomberman 64 decompilation) and file_to_c, built once.
build_tools() {
    if [ ! -x "$TOOLS/N64Recomp/build/N64Recomp" ] || [ ! -x "$TOOLS/N64Recomp/build/RSPRecomp" ]; then
        echo "== Building N64Recomp"
        if [ ! -d "$TOOLS/N64Recomp/.git" ]; then
            git clone --quiet https://github.com/N64Recomp/N64Recomp.git "$TOOLS/N64Recomp"
        fi
        git -C "$TOOLS/N64Recomp" -c advice.detachedHead=false checkout --quiet "$N64RECOMP_COMMIT"
        git -C "$TOOLS/N64Recomp" submodule update --init --recursive --quiet
        cmake -G Ninja -S "$TOOLS/N64Recomp" -B "$TOOLS/N64Recomp/build" -DCMAKE_BUILD_TYPE=Release > /dev/null
        cmake --build "$TOOLS/N64Recomp/build" --target N64Recomp RSPRecomp
    fi
    if [ ! -x "$TOOLS/bm64decompress" ]; then
        echo "== Building bm64decompress"
        if [ ! -d "$TOOLS/bm64/.git" ]; then
            git clone --quiet https://github.com/Bomberhackers/bm64.git "$TOOLS/bm64"
        fi
        git -C "$TOOLS/bm64" -c advice.detachedHead=false checkout --quiet "$BM64_DECOMP_COMMIT"
        g++ -O2 -o "$TOOLS/bm64decompress" "$TOOLS/bm64/tools/bm64decompress.cpp"
    fi
    if [ ! -x "$TOOLS/file_to_c" ]; then
        g++ -O2 -std=c++17 -o "$TOOLS/file_to_c" "$ROOT/third_party/rt64/file_to_c/file_to_c.cpp"
    fi
}

generate_patches() {
    echo "== Compiling the patches"
    make -C patches CC=clang LD=ld.lld
    mkdir -p RecompiledPatches
    "$TOOLS/N64Recomp/build/N64Recomp" patches.toml
    "$TOOLS/file_to_c" patches/patches.bin mm_patches_bin RecompiledPatches/patches_bin.c RecompiledPatches/patches_bin.h
}

build_tools

if [ "$1" = "patches" ]; then
    generate_patches
    exit 0
fi

ROM="$1"
if [ -z "$ROM" ] || [ ! -f "$ROM" ]; then
    echo "Usage: generate.sh <Bomberman 64 (USA) ROM> | generate.sh patches" >&2
    exit 1
fi

echo "== Decompressing the ROM"
# bm64decompress expects the big-endian (.z64) byte order.
python3 - "$ROM" /tmp/bm64.z64 <<'PY'
import sys
data = bytearray(open(sys.argv[1], 'rb').read())
magic = bytes(data[:4])
if magic == b'\x37\x80\x40\x12':    # .v64: 16-bit byte swapped
    data[0::2], data[1::2] = data[1::2], data[0::2]
elif magic == b'\x40\x12\x37\x80':  # .n64: little endian 32-bit words
    data[0::4], data[1::4], data[2::4], data[3::4] = data[3::4], data[2::4], data[1::4], data[0::4]
elif magic != b'\x80\x37\x12\x40':
    sys.exit("Not an N64 ROM: " + sys.argv[1])
open(sys.argv[2], 'wb').write(data)
PY
"$TOOLS/bm64decompress" /tmp/bm64.z64 "$DECOMPRESSED_ROM"
rm -f /tmp/bm64.z64
if [ "$(md5sum "$DECOMPRESSED_ROM" | cut -d' ' -f1)" != "$DECOMPRESSED_MD5" ]; then
    echo "The decompressed ROM doesn't match Bomberman 64 (USA). Check that the ROM is the USA version." >&2
    exit 1
fi

echo "== Recompiling the game"
mkdir -p RecompiledFuncs rsp
"$TOOLS/N64Recomp/build/N64Recomp" bm64.us.toml
"$TOOLS/N64Recomp/build/RSPRecomp" aspMain.toml

generate_patches
echo "== Done"
