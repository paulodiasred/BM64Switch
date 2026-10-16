#!/bin/bash
# Prepares a checkout for building:
#  - checks out the submodules (libultraship is pinned to the last SDL2 main-nx commit)
#  - applies the Switch patches in lib-patches/ to the submodules
# Safe to run more than once. Runs inside the host Docker image (tools/build.ps1 calls it).
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# sf64 only provides headers for the patches; its own tool submodules aren't needed.
git submodule update --init BM64Syms lib/sf64
git submodule update --init --recursive lib/N64ModernRuntime lib/libultraship

apply_patch() {
    local dir="$1" patch="$2"
    if git -C "$dir" apply --reverse --check "$ROOT/$patch" 2>/dev/null; then
        echo "$patch: already applied"
        return
    fi
    # A Windows checkout (core.autocrlf=true) has CRLF files the LF patches don't match. Check the submodule out
    # again with LF line endings first; it has no changes of ours yet, since the patch isn't applied.
    # (Nested submodules, mode 160000, are left alone.)
    git -C "$dir" config core.autocrlf false
    git -C "$dir" ls-files -s | awk '$1 != "160000"' | cut -f2- | (cd "$dir" && xargs -r -d '\n' rm -f)
    git -C "$dir" checkout -q -- .
    git -C "$dir" apply --whitespace=nowarn "$ROOT/$patch"
    echo "$patch: applied"
}

apply_patch lib/libultraship lib-patches/libultraship-switch.patch
apply_patch lib/N64ModernRuntime lib-patches/N64ModernRuntime-switch.patch
apply_patch lib/N64ModernRuntime/N64Recomp lib-patches/N64Recomp-switch.patch
