#ifndef __ZELDA_SUPPORT_H__
#define __ZELDA_SUPPORT_H__

#include <filesystem>

namespace zelda64 {
    // Folder with the files shipped alongside the game (the .nro's RomFS on the Switch).
    std::filesystem::path get_program_path();
}

#endif
