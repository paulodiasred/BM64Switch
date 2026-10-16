#!/bin/bash
# Installable NSP of the game (icon on the HOME menu, launches as a regular application with full memory).
# build.ps1 nsp runs the phases in this order:
#   fetch  (host image, network)     downloads hacBrewPack at a pinned commit and builds it
#   stage  (Switch image)            main NSO, npdm, NACP and icon from the Switch build
#   pack   (host image, NO network)  hacBrewPack with the console keys mounted read-only at /keys/prod.keys
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
TITLE_ID=0100BB6400000000
HBP_DIR="$ROOT/build-tools/hacBrewPack"
HBP_COMMIT=1d23a9db8e70e8cddede4be4dd627e6811e2fc8d
STAGE="$ROOT/build-switch/nsp"

case "$1" in
fetch)
    if [ ! -d "$HBP_DIR/.git" ]; then
        git clone --quiet https://github.com/tiliarou/hacBrewPack.git "$HBP_DIR"
    fi
    git -C "$HBP_DIR" -c advice.detachedHead=false checkout --quiet "$HBP_COMMIT"
    if [ ! -x "$HBP_DIR/hacbrewpack" ]; then
        cp "$HBP_DIR/config.mk.template" "$HBP_DIR/config.mk"
        make -C "$HBP_DIR" -j"$(nproc)" > /dev/null
    fi
    ;;
stage)
    export PATH="$PATH:$DEVKITPRO/tools/bin"
    rm -rf "$STAGE"
    mkdir -p "$STAGE/exefs" "$STAGE/control"
    elf2nso build-switch/BM64Recompiled.elf "$STAGE/exefs/main"
    npdmtool tools/nsp/bm64.npdm.json "$STAGE/exefs/main.npdm"
    VERSION=$(grep -oP 'BM64_NRO_VERSION "\K[^"]+' CMakeLists.txt)
    nacptool --create "Bomberman 64: Recompiled" "Paulo Dias" "$VERSION" "$STAGE/control/control.nacp" --titleid="$TITLE_ID"
    cp icons/nx-icon.jpg "$STAGE/control/icon_AmericanEnglish.dat"
    cp -r build-switch/romfs "$STAGE/romfs"
    ;;
pack)
    if [ ! -f /keys/prod.keys ]; then
        echo "prod.keys not mounted" >&2
        exit 1
    fi
    WORK=$(mktemp -d)
    # hacBrewPack edits the NACP and npdm in place, so give it copies.
    cp -r "$STAGE/exefs" "$STAGE/control" "$WORK/"
    "$HBP_DIR/hacbrewpack" -k /keys/prod.keys --titleid "$TITLE_ID" --nologo \
        --exefsdir "$WORK/exefs" --romfsdir "$STAGE/romfs" --controldir "$WORK/control" \
        --tempdir "$WORK/temp" --ncadir "$WORK/nca" --nspdir "$WORK/nsp"
    mkdir -p dist
    cp "$WORK"/nsp/*.nsp dist/BM64Recompiled.nsp
    rm -rf "$WORK"
    echo "== dist/BM64Recompiled.nsp"
    ;;
*)
    echo "Usage: build-nsp.sh fetch|stage|pack" >&2
    exit 1
    ;;
esac
