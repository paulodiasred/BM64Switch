// Runtime translation of the game's text (see bm64_translation.h).
//
// Game text encoding: one byte per character, terminated by 0x00.
//   A-Z = 0x1A-0x33, a-z = 0x3A-0x53, 0-9 = 0x10-0x19, space = 0x0F,
//   ! ? . - / = 0xF6 0xF7 0xF8 0xF9 0xFB, line break = 0xFF.
// Codes 0x55-0xF4 held Japanese glyphs that the USA font replaces with '-'; accented letters are mapped there.
//
// Translations are read from translations/<language>.txt (UTF-8), in blocks of:
//   en: Original text
//   pt: Translated text
// "\n" is a line break and "\xNN" a raw game byte.

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "recomp.h"
#include "librecomp/addresses.hpp"
#include "bm64_translation.h"
#include "zelda_support.h"

namespace {
    using GameString = std::string; // bytes in the game's encoding, without the terminator

    constexpr uint8_t code_space = 0x0F;
    constexpr uint8_t code_newline = 0xFF;
    constexpr uint32_t width_table_vram = 0x802A1630;
    constexpr size_t max_string_length = 1024;

    enum class Accent { Acute, Grave, Circumflex, Tilde, Diaeresis, Cedilla };

    struct AccentedLetter {
        const char* utf8;
        char base;
        Accent accent;
    };

    // Each entry gets the code 0x55 + its index.
    constexpr AccentedLetter accented_letters[] = {
        { "á", 'a', Accent::Acute }, { "à", 'a', Accent::Grave }, { "â", 'a', Accent::Circumflex },
        { "ã", 'a', Accent::Tilde }, { "é", 'e', Accent::Acute }, { "ê", 'e', Accent::Circumflex },
        { "í", 'i', Accent::Acute }, { "ó", 'o', Accent::Acute }, { "ô", 'o', Accent::Circumflex },
        { "õ", 'o', Accent::Tilde }, { "ú", 'u', Accent::Acute }, { "ü", 'u', Accent::Diaeresis },
        { "ç", 'c', Accent::Cedilla },
        { "Á", 'A', Accent::Acute }, { "À", 'A', Accent::Grave }, { "Â", 'A', Accent::Circumflex },
        { "Ã", 'A', Accent::Tilde }, { "É", 'E', Accent::Acute }, { "Ê", 'E', Accent::Circumflex },
        { "Í", 'I', Accent::Acute }, { "Ó", 'O', Accent::Acute }, { "Ô", 'O', Accent::Circumflex },
        { "Õ", 'O', Accent::Tilde }, { "Ú", 'U', Accent::Acute }, { "Ç", 'C', Accent::Cedilla },
    };
    constexpr uint8_t first_accented_code = 0x55;
    constexpr uint8_t accented_count = sizeof(accented_letters) / sizeof(accented_letters[0]);

    std::optional<uint8_t> encode_ascii(char c) {
        if (c >= 'A' && c <= 'Z') return static_cast<uint8_t>(c - 'A' + 0x1A);
        if (c >= 'a' && c <= 'z') return static_cast<uint8_t>(c - 'a' + 0x3A);
        if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0' + 0x10);
        switch (c) {
            case ' ': return code_space;
            case '!': return 0xF6;
            case '?': return 0xF7;
            case '.': return 0xF8;
            case '-': return 0xF9;
            case '/': return 0xFB;
        }
        return std::nullopt;
    }

    // Returns the UTF-8 for a game byte, or an empty string if it has no known character.
    std::string decode_byte(uint8_t b) {
        if (b >= 0x1A && b <= 0x33) return std::string(1, static_cast<char>('A' + b - 0x1A));
        if (b >= 0x3A && b <= 0x53) return std::string(1, static_cast<char>('a' + b - 0x3A));
        if (b >= 0x10 && b <= 0x19) return std::string(1, static_cast<char>('0' + b - 0x10));
        switch (b) {
            case code_space: return " ";
            case 0xF6: return "!";
            case 0xF7: return "?";
            case 0xF8: return ".";
            case 0xF9: return "-";
            case 0xFB: return "/";
            case code_newline: return "\\n";
        }
        if (b >= first_accented_code && b < first_accented_code + accented_count) {
            return accented_letters[b - first_accented_code].utf8;
        }
        return {};
    }

    std::string decode_string(const GameString& s) {
        std::string out;
        for (uint8_t b : s) {
            std::string ch = decode_byte(b);
            if (ch.empty()) {
                char esc[8];
                snprintf(esc, sizeof(esc), "\\x%02X", b);
                ch = esc;
            }
            out += ch;
        }
        return out;
    }

    // Converts a line of the translation file to the game's encoding. Returns nothing if it has an unsupported
    // character (reported with the line number).
    std::optional<GameString> encode_string(const std::string& text, int line_number) {
        GameString out;
        for (size_t i = 0; i < text.size();) {
            char c = text[i];
            if (c == '\\' && i + 1 < text.size()) {
                if (text[i + 1] == 'n') {
                    out += static_cast<char>(code_newline);
                    i += 2;
                    continue;
                }
                if (text[i + 1] == 'x' && i + 3 < text.size()) {
                    out += static_cast<char>(strtoul(text.substr(i + 2, 2).c_str(), nullptr, 16));
                    i += 4;
                    continue;
                }
            }
            if (auto code = encode_ascii(c)) {
                out += static_cast<char>(*code);
                i++;
                continue;
            }
            bool matched = false;
            for (uint8_t a = 0; a < accented_count; a++) {
                size_t len = strlen(accented_letters[a].utf8);
                if (text.compare(i, len, accented_letters[a].utf8) == 0) {
                    out += static_cast<char>(first_accented_code + a);
                    i += len;
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                fprintf(stderr, "[text] line %d: unsupported character in \"%s\"\n", line_number, text.c_str());
                return std::nullopt;
            }
        }
        return out;
    }

    uint8_t read_byte(uint8_t* rdram, uint32_t vram) {
        return rdram[(vram - 0x80000000u) ^ 3];
    }

    void write_byte(uint8_t* rdram, uint32_t vram, uint8_t value) {
        rdram[(vram - 0x80000000u) ^ 3] = value;
    }

    uint16_t read_half(uint8_t* rdram, uint32_t vram) {
        uint16_t value;
        memcpy(&value, rdram + ((vram - 0x80000000u) ^ 2), sizeof(value));
        return value;
    }

    void write_half(uint8_t* rdram, uint32_t vram, uint16_t value) {
        memcpy(rdram + ((vram - 0x80000000u) ^ 2), &value, sizeof(value));
    }

    bool is_rdram_pointer(uint32_t vram) {
        return vram >= 0x80000000u && vram < 0x80000000u + 0x00800000u;
    }

    GameString read_game_string(uint8_t* rdram, uint32_t vram) {
        GameString s;
        for (size_t i = 0; i < max_string_length; i++) {
            uint8_t b = read_byte(rdram, vram + static_cast<uint32_t>(i));
            if (b == 0) {
                break;
            }
            s += static_cast<char>(b);
        }
        return s;
    }

    struct State {
        std::atomic<bm64text::Language> language = bm64text::Language::English;
        std::mutex mutex;
        bool loaded = false;
        std::unordered_map<GameString, GameString> translations;
        // Translated strings already copied into RDRAM, by original text.
        std::unordered_map<GameString, uint32_t> rdram_copies;
        bool width_table_patched = false;
        // Text dump for building the translation inventory (BM64_TEXT_DUMP=1).
        FILE* dump_file = nullptr;
        bool dump_checked = false;
        std::set<GameString> dumped;
    };

    State& state() {
        static State s;
        return s;
    }

    const char* language_file(bm64text::Language language) {
        switch (language) {
            case bm64text::Language::Portuguese: return "pt-BR.txt";
            default: return nullptr;
        }
    }

    void load_translations(State& s) {
        s.loaded = true;
        s.translations.clear();
        s.rdram_copies.clear();
        const char* file = language_file(s.language.load());
        if (file == nullptr) {
            return;
        }
        std::filesystem::path path = zelda64::get_program_path() / "translations" / file;
        std::ifstream in(path);
        if (!in.good()) {
            fprintf(stderr, "[text] Couldn't open %s\n", path.string().c_str());
            return;
        }
        std::string line;
        std::optional<GameString> original;
        int line_number = 0;
        while (std::getline(in, line)) {
            line_number++;
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.rfind("en: ", 0) == 0) {
                original = encode_string(line.substr(4), line_number);
            } else if (line.rfind("pt: ", 0) == 0 && original) {
                if (auto translated = encode_string(line.substr(4), line_number)) {
                    s.translations[*original] = *translated;
                }
                original.reset();
            }
        }
        printf("[text] %zu translations loaded from %s\n", s.translations.size(), path.string().c_str());
    }

    // Gives every accented letter the width of its base letter in the font's width table (u16 start, u16 end per
    // code).
    void patch_width_table(uint8_t* rdram, State& s) {
        s.width_table_patched = true;
        for (uint8_t a = 0; a < accented_count; a++) {
            uint8_t base = *encode_ascii(accented_letters[a].base);
            uint32_t from = width_table_vram + base * 4u;
            uint32_t to = width_table_vram + (first_accented_code + a) * 4u;
            write_half(rdram, to, read_half(rdram, from));
            write_half(rdram, to + 2, read_half(rdram, from + 2));
        }
    }

    void dump_string(State& s, const GameString& text, uint32_t vram) {
        if (!s.dump_checked) {
            s.dump_checked = true;
            const char* env = getenv("BM64_TEXT_DUMP");
            if (env != nullptr && env[0] == '1') {
                s.dump_file = fopen("text_dump.txt", "a");
            }
        }
        if (s.dump_file == nullptr || text.empty() || !s.dumped.insert(text).second) {
            return;
        }
        fprintf(s.dump_file, "# 0x%08X\nen: %s\npt: \n\n", vram, decode_string(text).c_str());
        fflush(s.dump_file);
    }

    // Returns the address to use for the string at `vram`: a translated copy, or `vram` itself.
    uint32_t translate(uint8_t* rdram, uint32_t vram) {
        if (!is_rdram_pointer(vram)) {
            return vram;
        }
        State& s = state();
        std::lock_guard lock{ s.mutex };
        GameString text = read_game_string(rdram, vram);
        dump_string(s, text, vram);
        if (s.language.load() == bm64text::Language::English) {
            return vram;
        }
        if (!s.loaded) {
            load_translations(s);
        }
        if (!s.width_table_patched) {
            patch_width_table(rdram, s);
        }
        auto copy = s.rdram_copies.find(text);
        if (copy != s.rdram_copies.end()) {
            return copy->second;
        }
        auto found = s.translations.find(text);
        if (found == s.translations.end()) {
            return vram;
        }
        // Copied once and kept: the game may keep the pointer for as long as the text is on screen.
        const GameString& translated = found->second;
        uint8_t* host = static_cast<uint8_t*>(recomp::alloc(rdram, translated.size() + 1));
        uint32_t copy_vram = 0x80000000u + static_cast<uint32_t>(host - rdram);
        for (size_t i = 0; i < translated.size(); i++) {
            write_byte(rdram, copy_vram + static_cast<uint32_t>(i), static_cast<uint8_t>(translated[i]));
        }
        write_byte(rdram, copy_vram + static_cast<uint32_t>(translated.size()), 0);
        s.rdram_copies.emplace(text, copy_vram);
        return copy_vram;
    }

    // ---- Accent composition -----------------------------------------------------------------------------------
    // A glyph is a 24x24 4bpp image (12 bytes per row, left pixel in the high nibble), stored bottom row first.
    // Glyph::px is top row first.
    constexpr int glyph_size = 24;

    struct Glyph {
        uint8_t px[glyph_size][glyph_size];
    };

    Glyph read_glyph(uint8_t* rdram, uint32_t vram) {
        Glyph g{};
        for (int y = 0; y < glyph_size; y++) {
            for (int x = 0; x < glyph_size; x += 2) {
                uint8_t b = read_byte(rdram, vram + (glyph_size - 1 - y) * 12 + x / 2);
                g.px[y][x] = b >> 4;
                g.px[y][x + 1] = b & 0xF;
            }
        }
        return g;
    }

    void write_glyph(uint8_t* rdram, uint32_t vram, const Glyph& g) {
        for (int y = 0; y < glyph_size; y++) {
            for (int x = 0; x < glyph_size; x += 2) {
                write_byte(rdram, vram + (glyph_size - 1 - y) * 12 + x / 2,
                           static_cast<uint8_t>((g.px[y][x] << 4) | g.px[y][x + 1]));
            }
        }
    }

    // Debugging aid for tuning the accent marks: with BM64_TEXT_DUMP=1, composed glyphs go to glyph_dump.txt.
    void dump_glyph(const Glyph& g, const char* label) {
        static FILE* file = nullptr;
        static bool checked = false;
        if (!checked) {
            checked = true;
            const char* env = getenv("BM64_TEXT_DUMP");
            if (env != nullptr && env[0] == '1') {
                file = fopen("glyph_dump.txt", "a");
            }
        }
        if (file == nullptr) {
            return;
        }
        fprintf(file, "%s\n", label);
        for (int y = 0; y < glyph_size; y++) {
            for (int x = 0; x < glyph_size; x++) {
                fputc("0123456789ABCDEF"[g.px[y][x]], file);
            }
            fputc('\n', file);
        }
        fflush(file);
    }

    // Accent shapes, '#' = ink. Drawn centered over (or under, for the cedilla) the letter, then outlined like the
    // font's own strokes.
    struct Mark {
        int width;
        int height;
        const char* rows[4];
    };
    constexpr Mark mark_acute = { 5, 3, { "..###", ".###.", "###.." } };
    constexpr Mark mark_grave = { 5, 3, { "###..", ".###.", "..###" } };
    constexpr Mark mark_circumflex = { 8, 3, { "..####..", ".##..##.", "##....##" } };
    constexpr Mark mark_tilde = { 8, 3, { ".###...#", "##.##.##", "#...###." } };
    constexpr Mark mark_diaeresis = { 6, 2, { "##..##", "##..##" } };
    // Drawn from the letter's bottom outline row down, so it joins the letter.
    constexpr Mark mark_cedilla = { 4, 3, { ".##.", "..##", ".##." } };

    const Mark& mark_for(Accent accent) {
        switch (accent) {
            case Accent::Acute: return mark_acute;
            case Accent::Grave: return mark_grave;
            case Accent::Circumflex: return mark_circumflex;
            case Accent::Tilde: return mark_tilde;
            case Accent::Diaeresis: return mark_diaeresis;
            case Accent::Cedilla: return mark_cedilla;
        }
        return mark_acute;
    }

    // Vertically resamples the rows top..bottom of the letter into new_top..new_bottom.
    void resize_rows(Glyph& g, int top, int bottom, int new_top, int new_bottom) {
        Glyph resized{};
        int old_height = bottom - top + 1;
        int new_height = new_bottom - new_top + 1;
        for (int y = 0; y < new_height; y++) {
            memcpy(resized.px[new_top + y], g.px[top + y * old_height / new_height], glyph_size);
        }
        g = resized;
    }

    // The font's letters have a dark outline (color 1 in practice) and a fill whose color changes by row (a
    // gradient). The mark is filled with the letter's color at the rows nearest to it and outlined the same way.
    void compose_accent(Glyph& g, Accent accent, char base) {
        int top = glyph_size, bottom = -1, left = glyph_size, right = -1;
        int edge_count[16] = {};
        auto is_edge = [&g](int x, int y) {
            return x == 0 || y == 0 || x == glyph_size - 1 || y == glyph_size - 1 || g.px[y][x - 1] == 0 ||
                   g.px[y][x + 1] == 0 || g.px[y - 1][x] == 0 || g.px[y + 1][x] == 0;
        };
        for (int y = 0; y < glyph_size; y++) {
            for (int x = 0; x < glyph_size; x++) {
                uint8_t p = g.px[y][x];
                if (p == 0) {
                    continue;
                }
                top = std::min(top, y);
                bottom = std::max(bottom, y);
                left = std::min(left, x);
                right = std::max(right, x);
                if (is_edge(x, y)) {
                    edge_count[p]++;
                }
            }
        }
        if (bottom < 0) {
            return;
        }
        auto most_common = [](const int* counts) {
            int best = 0;
            for (int i = 1; i < 16; i++) {
                if (counts[i] > counts[best]) best = i;
            }
            return best;
        };
        uint8_t outline = static_cast<uint8_t>(most_common(edge_count));

        if (base == 'i') {
            // The accent replaces the dot: clear everything down to the first empty row under the top.
            int gap = top;
            while (gap <= bottom && std::any_of(g.px[gap], g.px[gap] + glyph_size, [](uint8_t p) { return p != 0; })) {
                gap++;
            }
            if (gap < bottom) {
                for (int y = top; y < gap; y++) {
                    memset(g.px[y], 0, glyph_size);
                }
                top = gap;
                while (top <= bottom && std::none_of(g.px[top], g.px[top] + glyph_size, [](uint8_t p) { return p != 0; })) {
                    top++;
                }
            }
        }

        const Mark& mark = mark_for(accent);
        int mark_top;
        int color_from, color_to;
        if (accent != Accent::Cedilla) {
            // The mark plus an outline row above and below it must fit over the letter.
            int needed = mark.height + 2;
            if (top < needed) {
                // Capital letters reach the top of the cell: shrink them from the top to make room.
                resize_rows(g, top, bottom, needed, bottom);
                top = needed;
            }
            mark_top = top - 1 - mark.height;
            color_from = top;
            color_to = top + 5;
        } else {
            // The mark starts on the letter's bottom outline row and needs an outline row below it.
            int max_bottom = glyph_size - mark.height - 1;
            if (bottom > max_bottom) {
                resize_rows(g, top, bottom, top, max_bottom);
                bottom = max_bottom;
            }
            mark_top = bottom;
            color_from = bottom - 5;
            color_to = bottom;
        }

        int fill_count[16] = {};
        for (int y = std::max(color_from, 0); y <= std::min(color_to, glyph_size - 1); y++) {
            for (int x = 0; x < glyph_size; x++) {
                if (g.px[y][x] != 0 && !is_edge(x, y)) {
                    fill_count[g.px[y][x]]++;
                }
            }
        }
        uint8_t fill = static_cast<uint8_t>(most_common(fill_count));
        if (fill == 0) {
            fill = outline;
        }

        int mark_left = (left + right + 1) / 2 - mark.width / 2;
        bool mask[glyph_size][glyph_size] = {};
        for (int my = 0; my < mark.height; my++) {
            for (int mx = 0; mx < mark.width; mx++) {
                int x = mark_left + mx, y = mark_top + my;
                if (mark.rows[my][mx] == '#' && x >= 0 && y >= 0 && x < glyph_size && y < glyph_size) {
                    mask[y][x] = true;
                }
            }
        }
        for (int y = 0; y < glyph_size; y++) {
            for (int x = 0; x < glyph_size; x++) {
                if (mask[y][x]) {
                    g.px[y][x] = fill;
                }
            }
        }
        // Outline the mark on the background only, so it doesn't cut into the letter.
        for (int y = 0; y < glyph_size; y++) {
            for (int x = 0; x < glyph_size; x++) {
                if (g.px[y][x] != 0 || mask[y][x]) {
                    continue;
                }
                for (int dy = -1; dy <= 1 && g.px[y][x] == 0; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && ny >= 0 && nx < glyph_size && ny < glyph_size && mask[ny][nx]) {
                            g.px[y][x] = outline;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Accented letter whose glyph is being loaded, from bm64_glyph_load_begin to bm64_glyph_load_end.
    int pending_accent = -1;
}

bm64text::Language bm64text::get_language() {
    return state().language.load();
}

void bm64text::set_language(Language language) {
    State& s = state();
    std::lock_guard lock{ s.mutex };
    if (s.language.exchange(language) != language) {
        s.loaded = false;
    }
}

// Called from hooks at the start of the game's text functions, whose first argument (a0) is the string.
extern "C" void bm64_translate_a0(uint8_t* rdram, recomp_context* ctx) {
    ctx->r4 = static_cast<int32_t>(translate(rdram, static_cast<uint32_t>(ctx->r4)));
}

// Same, for functions that take the string as their second argument (a1).
extern "C" void bm64_translate_a1(uint8_t* rdram, recomp_context* ctx) {
    ctx->r5 = static_cast<int32_t>(translate(rdram, static_cast<uint32_t>(ctx->r5)));
}

// Hook right before func_80280358 loads a glyph with func_80280274(file, code, buffer): accented letters load
// their base letter instead.
extern "C" void bm64_glyph_load_begin(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    uint8_t code = static_cast<uint8_t>(ctx->r5);
    pending_accent = -1;
    if (code >= first_accented_code && code < first_accented_code + accented_count) {
        pending_accent = code - first_accented_code;
        ctx->r5 = *encode_ascii(accented_letters[pending_accent].base);
    }
}

// Hook right after that load: draws the accent onto the loaded glyph (the buffer is in $s7).
extern "C" void bm64_glyph_load_end(uint8_t* rdram, recomp_context* ctx) {
    if (pending_accent < 0) {
        return;
    }
    uint32_t buffer = static_cast<uint32_t>(ctx->r23);
    Glyph g = read_glyph(rdram, buffer);
    dump_glyph(g, accented_letters[pending_accent].utf8);
    compose_accent(g, accented_letters[pending_accent].accent, accented_letters[pending_accent].base);
    dump_glyph(g, "composed");
    write_glyph(rdram, buffer, g);
    pending_accent = -1;
}

// Debugging aid for building the translation (see patches/text_patches.c): BM64_FILE_DUMP=1 dumps every file of the
// game's file system, decompressed, to filedump/<id>.bin.
extern "C" void bm64_get_file_dump_enabled(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    const char* env = getenv("BM64_FILE_DUMP");
    ctx->r2 = (env != nullptr && env[0] == '1') ? 1 : 0;
}

extern "C" void bm64_dump_file(uint8_t* rdram, recomp_context* ctx) {
    int32_t file = static_cast<int32_t>(ctx->r4);
    uint32_t data = static_cast<uint32_t>(ctx->r5);
    uint32_t size = static_cast<uint32_t>(ctx->r6);
    std::error_code ec;
    std::filesystem::create_directories("filedump", ec);
    char name[64];
    snprintf(name, sizeof(name), "filedump/%04d.bin", file);
    if (FILE* out = fopen(name, "wb")) {
        for (uint32_t i = 0; i < size; i++) {
            fputc(read_byte(rdram, data + i), out);
        }
        fclose(out);
    }
}
