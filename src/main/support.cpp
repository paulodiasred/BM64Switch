#include "zelda_support.h"

namespace zelda64 {
    std::filesystem::path get_program_path() {
#if defined(__SWITCH__)
        // Files shipped with the game are embedded in the .nro's RomFS.
        return "romfs:/";
#else
        return "";
#endif
    }
}
