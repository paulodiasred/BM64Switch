#include "patches.h"
#include "misc_funcs.h"

// Debugging aid for the translation: with BM64_FILE_DUMP=1, the first file load decompresses every file in the
// game's file system through the game's own loader and hands it to bm64_dump_file (written to filedump/).

extern void func_802267E0(s32);
extern s32 func_80226604(s32, s32);
extern void func_8022616C(void*, s32, s32, s32);
extern s32 func_80292BD0(s32, void*, s32);
extern void func_80226368(s32);

// Banks 0-2 hold 256 files each; bank 3 holds 105 (the rest of its table is unused).
#define BM64_FILE_COUNT (3 * 256 + 105)
#define BM64_DUMP_BUFFER_SIZE 0x100000

void bm64_dump_all_files(void) {
    static int done = 0;
    void* buffer;
    s32 file;

    if (done || !bm64_get_file_dump_enabled()) {
        return;
    }
    done = 1;
    buffer = recomp_alloc(BM64_DUMP_BUFFER_SIZE);
    for (file = 0; file < BM64_FILE_COUNT; file++) {
        s32 handle;
        s32 size = 0;
        func_802267E0(0x18);
        handle = func_80226604(file, 1);
        func_8022616C(&size, 4, 1, handle);
        if (size > 0 && size <= BM64_DUMP_BUFFER_SIZE) {
            func_80292BD0(handle, buffer, size);
            bm64_dump_file(file, buffer, size);
        }
        func_80226368(handle);
    }
}
