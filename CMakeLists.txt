cmake_minimum_required(VERSION 3.20)

# Bomberman 64: Recompiled for the Nintendo Switch.
#
# Two targets share the same code and the same renderer (libultraship's Fast3D over OpenGL):
#   - NintendoSwitch (devkitPro toolchain): BM64Recompiled.nro, the game itself.
#   - Linux: a desktop build used only to test and debug the port.
#
# The recompiled game code (RecompiledFuncs/, rsp/aspMain.cpp) and patches (RecompiledPatches/) are generated from
# the user's ROM by tools/generate.sh (tools/build.ps1 generate <ROM>). None of it is part of the repository.

project(BM64Recompiled LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(BUILD_SHARED_LIBS OFF)

set(BM64_LUS_PATH "${CMAKE_SOURCE_DIR}/lib/libultraship" CACHE PATH "Path to the libultraship source tree")

if (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(BM64_SWITCH ON)
    # newlib only declares POSIX/BSD extras (posix_memalign, M_PI, fileno...) when GNU extensions are enabled.
    set(CMAKE_CXX_EXTENSIONS ON)
    set(CMAKE_C_EXTENSIONS ON)
    # Every dependency (libultraship, ImGui, the runtime) includes SDL2 and glad headers from the portlibs.
    include_directories(
        ${DEVKITPRO}/portlibs/switch/include
        ${DEVKITPRO}/portlibs/switch/include/SDL2
    )

    # devkitPro's tinyxml2 portlib is too old and has no CMake target, so provide a newer one to find_package.
    include(FetchContent)
    set(tinyxml2_BUILD_TESTING OFF)
    FetchContent_Declare(
        tinyxml2
        GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
        GIT_TAG 10.0.0
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(tinyxml2)
elseif (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Only the Nintendo Switch and the Linux test build are supported.")
endif()

# libultraship renders the game. The game uses the F3DEX microcode.
set(GBI_UCODE F3DEX_GBI CACHE STRING "" FORCE)
add_subdirectory(${BM64_LUS_PATH} ${CMAKE_BINARY_DIR}/libultraship)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime)

if (BM64_SWITCH)
    # The runtime's CMake scripts turn GNU extensions off, which hides newlib's POSIX declarations.
    set_target_properties(librecomp ultramodern N64Recomp LiveRecomp fmt PROPERTIES CXX_EXTENSIONS ON C_EXTENSIONS ON)
    # Homebrew can't map executable memory, so the mod live recompiler gets an allocator that always fails.
    # Code mods then fail to load cleanly instead of the build needing mmap.
    target_compile_options(LiveRecomp PRIVATE -include ${CMAKE_SOURCE_DIR}/src/switch/sljit_switch_config.h)
endif()

# RecompiledFuncs - Library containing the primary recompiler output
add_library(RecompiledFuncs STATIC)

target_compile_options(RecompiledFuncs PRIVATE
    -fno-strict-aliasing
    -Wno-unused-variable
    -Wno-implicit-function-declaration
)

target_include_directories(RecompiledFuncs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/ultramodern/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
)

file(GLOB FUNC_C_SOURCES ${CMAKE_SOURCE_DIR}/RecompiledFuncs/*.c)
file(GLOB FUNC_CXX_SOURCES ${CMAKE_SOURCE_DIR}/RecompiledFuncs/*.cpp)
if (NOT FUNC_C_SOURCES AND NOT FUNC_CXX_SOURCES)
    message(FATAL_ERROR "RecompiledFuncs/ is empty. Generate it from your ROM first (tools/build.ps1 generate <ROM>).")
endif()

target_sources(RecompiledFuncs PRIVATE ${FUNC_C_SOURCES} ${FUNC_CXX_SOURCES})

# With GCC the original functions must be weak so that the patched versions in PatchesLib replace them
# (clang builds already emit every recompiled function as weak).
target_compile_definitions(RecompiledFuncs PRIVATE RECOMP_WEAK_FUNCS)

# PatchesLib - Library containing the recompiled output for any custom function patches
add_library(PatchesLib STATIC)

target_compile_options(PatchesLib PRIVATE
    -fno-strict-aliasing
    -Wno-unused-variable
    -Wno-implicit-function-declaration
)

target_include_directories(PatchesLib PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/ultramodern/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
)

target_sources(PatchesLib PRIVATE
    ${CMAKE_SOURCE_DIR}/RecompiledPatches/patches.c
    ${CMAKE_SOURCE_DIR}/RecompiledPatches/patches_bin.c
)

set_source_files_properties(${CMAKE_SOURCE_DIR}/RecompiledPatches/patches.c PROPERTIES COMPILE_FLAGS -fno-strict-aliasing)

# The patches are recompiled by tools/generate.sh (tools/build.ps1 generate / patches), not by CMake.
foreach(PREGENERATED patches.c patches_bin.c patches_bin.h recomp_overlays.inl funcs.h)
    if (NOT EXISTS ${CMAKE_SOURCE_DIR}/RecompiledPatches/${PREGENERATED})
        message(FATAL_ERROR "RecompiledPatches/${PREGENERATED} is missing. Run tools/build.ps1 generate <ROM> first.")
    endif()
endforeach()

# Main executable
add_executable(BM64Recompiled)

target_sources(BM64Recompiled PRIVATE
    ${CMAKE_SOURCE_DIR}/src/main/main.cpp
    ${CMAKE_SOURCE_DIR}/src/main/support.cpp
    ${CMAKE_SOURCE_DIR}/src/main/register_overlays.cpp
    ${CMAKE_SOURCE_DIR}/src/main/register_patches.cpp

    ${CMAKE_SOURCE_DIR}/src/game/input.cpp
    ${CMAKE_SOURCE_DIR}/src/game/controls.cpp
    ${CMAKE_SOURCE_DIR}/src/game/config.cpp
    ${CMAKE_SOURCE_DIR}/src/game/scene_table.cpp
    ${CMAKE_SOURCE_DIR}/src/game/debug.cpp
    ${CMAKE_SOURCE_DIR}/src/game/quicksaving.cpp
    ${CMAKE_SOURCE_DIR}/src/game/recomp_api.cpp
    ${CMAKE_SOURCE_DIR}/src/game/recomp_actor_api.cpp
    ${CMAKE_SOURCE_DIR}/src/game/recomp_data_api.cpp
    ${CMAKE_SOURCE_DIR}/src/game/rom_decompression.cpp
    ${CMAKE_SOURCE_DIR}/src/game/minicoro.cpp
    ${CMAKE_SOURCE_DIR}/src/game/process_coroutines.cpp
    ${CMAKE_SOURCE_DIR}/src/game/translation.cpp
    ${CMAKE_SOURCE_DIR}/src/game/dialogue_translation.cpp

    ${CMAKE_SOURCE_DIR}/src/lus/dl_translator.cpp
    ${CMAKE_SOURCE_DIR}/src/lus/lus_render_context.cpp
    ${CMAKE_SOURCE_DIR}/src/lus/lus_menu.cpp
    ${CMAKE_SOURCE_DIR}/src/lus/lus_controldeck.cpp
    ${CMAKE_SOURCE_DIR}/src/lus/lus_ui_stubs.cpp
    ${CMAKE_SOURCE_DIR}/src/lus/lus_settings.cpp
    ${CMAKE_SOURCE_DIR}/src/lus/perf_monitor.cpp

    ${CMAKE_SOURCE_DIR}/rsp/aspMain.cpp
)

target_include_directories(BM64Recompiled PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
    ${CMAKE_SOURCE_DIR}/lib/GamepadMotionHelpers
    ${CMAKE_SOURCE_DIR}/lib/SlotMap
    ${BM64_LUS_PATH}/include
    ${BM64_LUS_PATH}/src
)

target_compile_options(BM64Recompiled PRIVATE
    -fno-strict-aliasing
    -fms-extensions
)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    # The recompiled RSP microcode uses SSE4.1 on x86.
    target_compile_options(BM64Recompiled PRIVATE -march=nehalem)
endif()

target_link_libraries(BM64Recompiled PRIVATE
    PatchesLib
    RecompiledFuncs
    librecomp
    ultramodern
    libultraship
)

# bm64.o2r holds the libultraship OpenGL shaders. An o2r archive is a plain zip file.
if (BM64_SWITCH)
    # Everything the game needs besides the ROM is embedded in the .nro's RomFS, so a single file can be installed.
    set(BM64_ROMFS_DIR ${CMAKE_BINARY_DIR}/romfs)
    file(MAKE_DIRECTORY ${BM64_ROMFS_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/recompcontrollerdb.txt DESTINATION ${BM64_ROMFS_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/translations DESTINATION ${BM64_ROMFS_DIR})
    set(BM64_O2R ${BM64_ROMFS_DIR}/bm64.o2r)
else()
    file(COPY ${CMAKE_SOURCE_DIR}/recompcontrollerdb.txt DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/translations DESTINATION ${CMAKE_BINARY_DIR})
    set(BM64_O2R ${CMAKE_BINARY_DIR}/bm64.o2r)
endif()
add_custom_command(OUTPUT ${BM64_O2R}
    COMMAND ${CMAKE_COMMAND} -E rm -f ${BM64_O2R}
    COMMAND ${CMAKE_COMMAND} -E tar cf ${BM64_O2R} --format=zip shaders
    WORKING_DIRECTORY ${BM64_LUS_PATH}/src/fast
    DEPENDS ${BM64_LUS_PATH}/src/fast/shaders/opengl/default.shader.glsl
)
add_custom_target(GenerateO2R ALL DEPENDS ${BM64_O2R})
add_dependencies(BM64Recompiled GenerateO2R)

if (BM64_SWITCH)
    find_package(SDL2 REQUIRED)
    # libultraship is a static library that needs glad and the GL driver, so they must come after it on the link line.
    target_link_libraries(libultraship PUBLIC -lglad SDL2::SDL2 -lEGL -lglapi -ldrm_nouveau -lnx)
    target_link_libraries(BM64Recompiled PRIVATE SDL2::SDL2)
    target_sources(BM64Recompiled PRIVATE ${CMAKE_SOURCE_DIR}/src/switch/switch_support.cpp)
    # Gives threads created without an explicit stack size a bigger stack (see switch_support.cpp).
    target_link_options(BM64Recompiled PRIVATE -Wl,--wrap=pthread_create)

    set(BM64_NRO_VERSION "1.0.0")
    # icons/custom-icon.jpg (256x256 JPEG, not committed) replaces the default icon when present.
    set(BM64_ICON ${CMAKE_SOURCE_DIR}/icons/nx-icon.jpg)
    if (EXISTS ${CMAKE_SOURCE_DIR}/icons/custom-icon.jpg)
        set(BM64_ICON ${CMAKE_SOURCE_DIR}/icons/custom-icon.jpg)
    endif()
    nx_generate_nacp(BM64Recompiled.nacp
        NAME "Bomberman 64: Recompiled"
        AUTHOR "Paulo Dias"
        VERSION ${BM64_NRO_VERSION}
    )
    nx_create_nro(BM64Recompiled
        NACP BM64Recompiled.nacp
        ICON ${BM64_ICON}
        ROMFS ${BM64_ROMFS_DIR}
    )
    add_dependencies(BM64Recompiled_nro GenerateO2R)
else()
    find_package(SDL2 REQUIRED)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    target_include_directories(BM64Recompiled PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(BM64Recompiled PRIVATE SDL2::SDL2 "-latomic" ${CMAKE_DL_LIBS} Threads::Threads)
endif()
