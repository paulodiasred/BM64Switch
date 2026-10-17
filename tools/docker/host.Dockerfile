# Host tools image: generates the recompiled code from the ROM (N64Recomp, clang for the MIPS patches) and runs the
# Linux test build headless (Xvfb + Mesa software rendering). zipcmp/zipmerge/ziptool are only there because
# libzip's CMake package refuses to load without them.
# python3-pil and fonts-liberation build the dialogue font for the translation (tools/text/make_font.py).
FROM ubuntu:24.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential clang lld llvm cmake ninja-build git python3 ca-certificates make pkg-config \
    libsdl2-dev libgl-dev libegl-dev libgles-dev libglew-dev libgl1-mesa-dri \
    zlib1g-dev libzip-dev libbz2-dev nlohmann-json3-dev libtinyxml2-dev libspdlog-dev libogg-dev libvorbis-dev \
    zipcmp zipmerge ziptool \
    xvfb xdotool imagemagick gdb \
    python3-pil fonts-liberation \
    && rm -rf /var/lib/apt/lists/*

# The repository is mounted from the Windows host, owned by another user.
RUN git config --system --add safe.directory '*'
