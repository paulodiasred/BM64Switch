# Bomberman 64: Recompiled — Nintendo Switch

**[English](README.md)** | Português

> **Port para Nintendo Switch desenvolvido por [Paulo Dias](https://github.com/paulodiasred).**
> Se curtiu, deixe uma ⭐ no repositório!

<p align="center">
  <img src="docs/photos/switch-1.jpg" width="360" alt="Bomberman 64 rodando num Nintendo Switch OLED">
  <img src="docs/photos/switch-2.jpg" width="360" alt="Modo Adventure do Bomberman 64 num Nintendo Switch OLED">
  <br><em>Rodando num Nintendo Switch OLED de verdade.</em>
</p>

Port para Nintendo Switch (homebrew, `.nro`) do [Bomberman 64: Recompiled](https://github.com/RevoSucks/BM64Recomp).
O código do jogo é recompilado estaticamente da ROM original com o [N64Recomp](https://github.com/N64Recomp/N64Recomp)
e roda sobre o [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime). No PC, a renderização é feita pelo
RT64 (Vulkan/D3D12/Metal), que não existe no Switch. Aqui ela é feita pelo renderer Fast3D do
[libultraship](https://github.com/YoshiCrystal9/libultraship) sobre OpenGL, e o menu de configurações é em ImGui.

| | |
|---|---|
| ![Tela inicial](docs/screenshots/title.png) | ![Cena de abertura](docs/screenshots/intro.png) |
| ![Modo Battle](docs/screenshots/battle.png) | ![Modo Adventure](docs/screenshots/adventure.png) |

*Screenshots do renderer do port (o mesmo usado no Switch).*

**Este repositório não contém nada do jogo. É preciso ter a sua própria ROM do Bomberman 64 (USA) para compilar e para
jogar.**

## Instalação no Switch

1. Copie `BM64Recompiled.nro` para `sdmc:/switch/` no cartão SD. É o único arquivo do jogo: shaders e o banco de
   controles vão embutidos nele.
2. Coloque a sua ROM do Bomberman 64 (USA) em um destes lugares:
   - `sdmc:/switch/bm64recomp/` (pasta criada pelo jogo na primeira execução),
   - `sdmc:/switch/`,
   - a raiz do cartão (`sdmc:/`).

   Qualquer formato serve (`.z64`, `.n64` ou `.v64`), com qualquer nome.
3. Abra pelo Homebrew Menu em **modo title override**: segure **R** ao abrir um jogo qualquer. Pelo Álbum (modo
   applet) não há memória suficiente.

Na primeira execução a ROM é validada e copiada para `sdmc:/switch/bm64recomp/`, onde também ficam configurações,
saves, o cache de shaders e os logs (`bm64.log`, `bm64-err.log`).

**Menu de configurações:** aperte **−** (Minus) para abrir ou fechar. Ele tem as abas:
- **Graphics:** resolução interna, anti-aliasing, filtro de textura, VSync, FPS e o clock da CPU do Switch.
- **Sound.**
- **Controls:** vibração, zona morta e remapeamento.
- **Cheats.**

As opções são salvas quando o menu fecha.

## Compilação

Tudo roda em Docker, então no Windows só é preciso o Docker Desktop e o PowerShell. Clone com os submódulos:

```powershell
git clone --recurse-submodules https://github.com/paulodiasred/BM64Switch.git
cd BM64Switch
```

| Comando | O que faz |
|---|---|
| `.\build.ps1 generate C:\caminho\bomberman64.z64` | Uma vez: descomprime a ROM, recompila o jogo (`RecompiledFuncs/`), o microcódigo de áudio (`rsp/aspMain.cpp`) e os patches (`RecompiledPatches/`). |
| `.\build.ps1 switch` | Gera `dist\BM64Recompiled.nro` (e o `.elf`, para ler crash reports). |
| `.\build.ps1 all C:\caminho\bomberman64.z64` | `generate` + `switch`. |
| `.\build.ps1 patches` | Recompila só os patches, depois de editar `patches\*.c`. |
| `.\build.ps1 linux` | Build de teste para Linux (mesmo código e renderer), em `build-linux\`. |
| `.\build.ps1 test run.sh SHOTS="10 20 30"` | Build Linux + execução headless de um script de `tools\test\`, que tira screenshots em `run\`. A ROM precisa estar em `run\`. |
| `.\build.ps1 shell` | Abre um shell na imagem Docker de ferramentas. |
| `.\build.ps1 clean` | Apaga `build-linux\`, `build-switch\` e `dist\`. |

Na primeira vez, o script cria as duas imagens Docker:
- `bm64switch-host`: Ubuntu com clang, N64Recomp, Xvfb e Mesa.
- `bm64switch-nx`: devkitA64 com libnx.

Ele também baixa e compila as ferramentas em `build-tools\`: o N64Recomp e o `bm64decompress` do
[projeto de decompilação](https://github.com/Bomberhackers/bm64).

Nada gerado a partir da ROM pode ir para o git: o `.gitignore` já cobre ROMs, `RecompiledFuncs/`,
`RecompiledPatches/`, `rsp/` e os binários dos patches.

## Estrutura

- `src/lus/`: o renderer.
  - `dl_translator.cpp` traduz as display lists F3DEX e o GBI estendido do RT64 para o libultraship. Também emula o
    framebuffer do motion blur e o fade por fog.
  - `lus_render_context.cpp` é o contexto de renderização.
  - `lus_menu.cpp` é o menu ImGui.
  - Os demais arquivos substituem a UI RmlUi do PC.
- `src/switch/`: glue do Switch.
  - Pilha de 2 MB e TLS zerada para as threads criadas via `pthread_create`.
  - Configuração do sljit sem memória executável.
- `lib-patches/`: mudanças nos submódulos (N64ModernRuntime, N64Recomp e libultraship). O `tools/setup.sh` as aplica,
  e o `build.ps1` roda esse script sozinho.
- `patches/`: patches do jogo (C para MIPS), recompilados junto com o jogo.
- `third_party/rt64/`: o header do GBI estendido e a ferramenta `file_to_c`, copiados do RT64 (MIT).
- `tools/test/`: scripts de teste headless.
  - `run.sh`: screenshots em tempos fixos.
  - `play.sh`: sequência de teclas e screenshots.
  - `audio.sh`: níveis de áudio.
  - `gdbrun.sh`: backtrace.

## Depuração

- **Logs:** `sdmc:/switch/bm64recomp/bm64.log` e `bm64-err.log`.
- **Crash reports do Atmosphère:** ficam em `sdmc:/atmosphere/crash_reports/`. Para ler, use
  `aarch64-none-elf-addr2line -e dist/BM64Recompiled.elf <offset>` dentro da imagem `bm64switch-nx`, com os offsets
  do módulo do jogo.
- **Dump de display lists:** `BM64_DL_DUMP=N` (build Linux) grava a display list de cada N-ésimo frame em
  `dl_<frame>.txt`.

## Limitações conhecidas

- Mods de código não funcionam: homebrew não pode criar memória executável.
- O modo applet (Álbum) não tem memória suficiente. Use o title override.
- O primeiro uso de cada efeito ainda pode dar uma travadinha, enquanto o shader compila. Os shaders ficam salvos em
  `shaders.bin` e são pré-compilados nas execuções seguintes.

## Créditos

- **[Paulo Dias](https://github.com/paulodiasred):** port para Nintendo Switch (renderer libultraship, tradutor de
  display lists, menu ImGui, adaptações do runtime para o Switch e build).
- [BM64Recomp](https://github.com/RevoSucks/BM64Recomp) (RevoSucks e colaboradores), base deste port.
- [N64Recomp / N64ModernRuntime](https://github.com/N64Recomp) (Wiseguy e colaboradores).
- [libultraship](https://github.com/Kenix3/libultraship) e o fork para Switch de
  [YoshiCrystal9](https://github.com/YoshiCrystal9/libultraship).
- [RT64](https://github.com/rt64/rt64): GBI estendido e `file_to_c`.
- [Bomberhackers/bm64](https://github.com/Bomberhackers/bm64): decompilação e `bm64decompress`.

A licença original do projeto está em [LICENSE.txt](LICENSE.txt).
