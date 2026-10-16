Files copied from [RT64](https://github.com/rt64/rt64) at commit 71b5906f0b991459744dabfcc68d04616ac71797 (MIT, see LICENSE):

- `include/rt64_extended_gbi.h`: the extended GBI the game patches emit (`gEX*` commands). The libultraship
  renderer translates these in `src/lus/dl_translator.cpp`.
- `file_to_c/file_to_c.cpp`: host tool that embeds `patches/patches.bin` as a C array.
