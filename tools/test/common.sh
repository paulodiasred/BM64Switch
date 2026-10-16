# Shared setup for the headless test scripts. They run inside the bm64-host Docker image (tools/build.ps1 test),
# with the repository mounted at /src. The game runs from $RUN_DIR (portable mode), which needs the ROM in it.
SRC=${SRC:-/src}
BIN=${BIN:-$SRC/build-linux/BM64Recompiled}
RUN_DIR=${RUN_DIR:-$SRC/run}
mkdir -p "$RUN_DIR"
cp -f "$(dirname "$BIN")/bm64.o2r" "$SRC/recompcontrollerdb.txt" "$RUN_DIR"/
touch "$RUN_DIR/portable.txt"
cd "$RUN_DIR"
Xvfb :99 -screen 0 1280x720x24 >/dev/null 2>&1 &
export DISPLAY=:99 LIBGL_ALWAYS_SOFTWARE=1
export SDL_AUDIODRIVER=${SDL_AUDIODRIVER:-dummy}
sleep 1
