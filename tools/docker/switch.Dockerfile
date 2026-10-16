# Nintendo Switch toolchain image: devkitA64 + libnx + portlibs, with a CMake recent enough for libultraship.
# The base image already has git, wget, ninja and zip (its Debian bullseye mirrors no longer serve updates).
FROM devkitpro/devkita64:20241023

RUN (apt-get remove -y cmake || true) && \
    wget -q https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.sh && \
    sh cmake-3.28.3-linux-x86_64.sh --skip-license --prefix=/usr/local && rm cmake-3.28.3-linux-x86_64.sh

RUN git config --system --add safe.directory '*'
