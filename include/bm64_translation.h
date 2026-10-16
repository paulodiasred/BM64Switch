#ifndef __BM64_TRANSLATION_H__
#define __BM64_TRANSLATION_H__

// Runtime translation of the game's text. The game draws every string through a few functions (see the hooks in
// bm64.us.toml); hooks there swap the string pointer for a translated copy in RDRAM, and letters the game's font
// doesn't have (accented ones) are composed from the base letter plus an accent mark while the glyph is loaded.
namespace bm64text {
    enum class Language {
        English = 0,
        Portuguese = 1,
    };

    Language get_language();
    void set_language(Language language);
}

#endif
