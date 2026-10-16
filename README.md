# Bomberman 64: Recompiled — Nintendo Switch

English | **[Português](README.pt-BR.md)**

> **Nintendo Switch port developed by [Paulo Dias](https://github.com/paulodiasred).**
> If you enjoy it, leave a ⭐ on the repository!

<p align="center">
  <img src="docs/photos/switch-1.jpg" width="360" alt="Bomberman 64 running on a Nintendo Switch OLED">
  <img src="docs/photos/switch-2.jpg" width="360" alt="Bomberman 64 Adventure mode on a Nintendo Switch OLED">
  <br><em>Running on a real Nintendo Switch OLED.</em>
</p>

A Nintendo Switch homebrew port (`.nro`) of [Bomberman 64: Recompiled](https://github.com/RevoSucks/BM64Recomp).
The game code is statically recompiled from the original ROM with [N64Recomp](https://github.com/N64Recomp/N64Recomp)
and runs on [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime). The PC version renders with RT64
(Vulkan/D3D12/Metal), which isn't available on the Switch. This port renders with the Fast3D renderer from
[libultraship](https://github.com/YoshiCrystal9/libultraship) over OpenGL, and its settings menu uses ImGui.

| | |
|---|---|
| ![Title screen](docs/screenshots/title.png) | ![Intro cutscene](docs/screenshots/intro.png) |
| ![Battle mode](docs/screenshots/battle.png) | ![Adventure mode](docs/screenshots/adventure.png) |

*Screenshots from the port's renderer (the same one used on the Switch).*

**This repository contains no game assets. You need your own Bomberman 64 (USA) ROM to build and to play.**

## Installing on the Switch

1. Copy `BM64Recompiled.nro` to `sdmc:/switch/` on the SD card. It's the only file to install: the shaders and the
   controller database are embedded in it.
2. Put your Bomberman 64 (USA) ROM in one of these places:
   - `sdmc:/switch/bm64recomp/` (the game creates this folder on first launch),
   - `sdmc:/switch/`,
   - the root of the SD card (`sdmc:/`).

   Any format works (`.z64`, `.n64` or `.v64`), with any file name.
3. Launch it from the Homebrew Menu in **title override mode**: hold **R** while opening any game. The album (applet
   mode) doesn't have enough memory.

On first launch the ROM is validated and copied to `sdmc:/switch/bm64recomp/`. Settings, saves, the shader cache and
the logs (`bm64.log`, `bm64-err.log`) also live in that folder.

### Optional: install as an NSP (icon on the HOME menu)

The game can also be installed as a regular title. It then shows up on the HOME menu, launches directly with full
memory (no need to hold R) and doesn't need the `.nro`.

1. Build `dist\BM64Recompiled.nsp` with `.\build.ps1 nsp C:\path\prod.keys`. You need your own console's
   `prod.keys`, dumped with Lockpick_RCM. The keys stay on your PC: only the packing step reads them, from a
   container without network access, and they never go into the repository.
2. Install it with a title installer (DBI, Tinfoil, Goldleaf...). Installing NSPs requires sigpatches in Atmosphère.
3. The ROM goes in the same places as above. Settings and saves are shared with the `.nro`
   (`sdmc:/switch/bm64recomp/`).

**Settings menu:** press **−** (Minus) to open or close it. It has these tabs:
- **Graphics:** internal resolution, anti-aliasing, texture filtering, VSync, FPS and the Switch CPU clock.
- **Sound.**
- **Controls:** rumble, deadzone and remapping.
- **Cheats.**

Settings are saved when the menu closes.

## Building

Everything runs in Docker, so on Windows you only need Docker Desktop and PowerShell. Clone with submodules:

```powershell
git clone --recurse-submodules https://github.com/paulodiasred/BM64Switch.git
cd BM64Switch
```

| Command | What it does |
|---|---|
| `.\build.ps1 generate C:\path\bomberman64.z64` | Once: decompresses the ROM and recompiles the game (`RecompiledFuncs/`), the audio microcode (`rsp/aspMain.cpp`) and the patches (`RecompiledPatches/`). |
| `.\build.ps1 switch` | Builds `dist\BM64Recompiled.nro`, plus the `.elf` for reading crash reports. |
| `.\build.ps1 all C:\path\bomberman64.z64` | `generate` + `switch`. |
| `.\build.ps1 patches` | Recompiles only the patches, after editing `patches\*.c`. |
| `.\build.ps1 linux` | Linux test build (same code and renderer) in `build-linux\`. |
| `.\build.ps1 test run.sh SHOTS="10 20 30"` | Linux build + headless run of a `tools\test\` script, which takes screenshots in `run\`. The ROM must be in `run\`. |
| `.\build.ps1 shell` | Opens a shell in the tools Docker image. |
| `.\build.ps1 clean` | Deletes `build-linux\`, `build-switch\` and `dist\`. |

The first run creates two Docker images:
- `bm64switch-host`: Ubuntu with clang, N64Recomp, Xvfb and Mesa.
- `bm64switch-nx`: devkitA64 with libnx.

It also downloads and builds tools into `build-tools\`: N64Recomp, and `bm64decompress` from the
[decompilation project](https://github.com/Bomberhackers/bm64).

Nothing generated from the ROM may be committed. The `.gitignore` already covers ROMs, `RecompiledFuncs/`,
`RecompiledPatches/`, `rsp/` and the patch binaries.

## Layout

- `src/lus/`: the renderer.
  - `dl_translator.cpp` translates F3DEX display lists and RT64's extended GBI for libultraship. It also emulates the
    motion blur framebuffer and the fog-based fades.
  - `lus_render_context.cpp` is the render context.
  - `lus_menu.cpp` is the ImGui menu.
  - The other files replace the PC version's RmlUi UI.
- `src/switch/`: Switch glue.
  - 2 MB stacks and cleared TLS for threads created through `pthread_create`.
  - sljit configured without executable memory.
- `lib-patches/`: changes to the submodules (N64ModernRuntime, N64Recomp and libultraship). `tools/setup.sh` applies
  them, and `build.ps1` runs it automatically.
- `patches/`: game patches (C for MIPS), recompiled along with the game.
- `third_party/rt64/`: the extended GBI header and the `file_to_c` tool, copied from RT64 (MIT).
- `tools/test/`: headless test scripts.
  - `run.sh`: screenshots at fixed times.
  - `play.sh`: a sequence of key presses and screenshots.
  - `audio.sh`: audio levels.
  - `gdbrun.sh`: backtrace.

## Debugging

- **Logs:** `sdmc:/switch/bm64recomp/bm64.log` and `bm64-err.log`.
- **Atmosphère crash reports:** they're in `sdmc:/atmosphere/crash_reports/`. To read them, use
  `aarch64-none-elf-addr2line -e dist/BM64Recompiled.elf <offset>` in the `bm64switch-nx` image, with the offsets
  inside the game module.
- **Display list dumps:** `BM64_DL_DUMP=N` (Linux build) writes every Nth frame's display list to `dl_<frame>.txt`.

## Known limitations

- Code mods don't work: homebrew can't create executable memory.
- Applet mode (album) doesn't have enough memory. Use title override.
- The first time an effect appears it can still hitch briefly while its shader compiles. Shaders are saved to
  `shaders.bin` and precompiled on later launches.

## Credits

- **[Paulo Dias](https://github.com/paulodiasred):** Nintendo Switch port (libultraship renderer, display list
  translator, ImGui menu, Switch runtime adaptations and build).
- [BM64Recomp](https://github.com/RevoSucks/BM64Recomp) (RevoSucks and contributors): the base of this port.
- [N64Recomp / N64ModernRuntime](https://github.com/N64Recomp) (Wiseguy and contributors).
- [libultraship](https://github.com/Kenix3/libultraship) and the Switch fork by
  [YoshiCrystal9](https://github.com/YoshiCrystal9/libultraship).
- [RT64](https://github.com/rt64/rt64): extended GBI and `file_to_c`.
- [Bomberhackers/bm64](https://github.com/Bomberhackers/bm64): decompilation and `bm64decompress`.

The original project's license is in [LICENSE.txt](LICENSE.txt).
