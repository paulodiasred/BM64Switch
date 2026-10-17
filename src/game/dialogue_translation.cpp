// Translation of the dialogue images. The game's conversations aren't text: each one is a file with a tall 4bpp
// image (256 pixels wide, white text on black, plus portraits, icons and diagrams) that the game scrolls through.
// When such a file is loaded, its text lines are erased and redrawn in the selected language, following
// translations/<language>-dialogue.txt:
//
//   [856]                          file ID
//   line y0 y1 x0 x1: text         erase rows y0-y1 / columns x0-x1 and draw the text there (dialogue font)
//   small y0 y1 x0 x1: text        same with the small font of the tutorial diagrams
//   big y0 y1 x0 x1: text          same with the dialogue font at twice its size
//   clear y0 y1 x0 x1              only erase
//   cleartext y0 y1 x0 x1          only erase the pixels in the text color (text over a drawing)
//
// Inside a text, {x0-x1} copies the original pixels of those columns (an inline icon such as a bomb or a button)
// to the current position. Lines starting with # are comments. The font is translations/dialogue_font.bin.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "recomp.h"
#include "bm64_translation.h"
#include "zelda_support.h"

namespace {
    struct Glyph {
        uint8_t advance = 0, width = 0, height = 0;
        int8_t x_offset = 0, y_offset = 0;
        uint32_t bitmap_offset = 0;
    };

    struct Font {
        uint8_t line_height = 0;
        uint8_t ascent = 0;
        Glyph glyphs[224];
        std::vector<uint8_t> bitmap;

        const Glyph* glyph(uint32_t code) const {
            if (code < 32 || code > 255) {
                code = '?';
            }
            return &glyphs[code - 32];
        }
    };

    struct Op {
        enum class Kind { Line, Small, Big, Clear, ClearText } kind = Kind::Line;
        int y0 = 0, y1 = 0, x0 = 0, x1 = 0;
        std::string text; // UTF-8
    };

    struct State {
        std::mutex mutex;
        bool loaded = false;
        bm64text::Language loaded_language = bm64text::Language::English;
        std::vector<Font> fonts;
        std::unordered_map<int, std::vector<Op>> files;
    };

    State& state() {
        static State s;
        return s;
    }

    bool load_fonts(State& s) {
        std::filesystem::path path = zelda64::get_program_path() / "translations" / "dialogue_font.bin";
        std::ifstream in(path, std::ios::binary);
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (data.size() < 5 || memcmp(data.data(), "BMF1", 4) != 0) {
            fprintf(stderr, "[dialogue] Couldn't load %s\n", path.string().c_str());
            return false;
        }
        size_t pos = 5;
        auto u8 = [&](void) { return data[pos++]; };
        auto u32 = [&](void) {
            uint32_t v = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (static_cast<uint32_t>(data[pos + 3]) << 24);
            pos += 4;
            return v;
        };
        for (int f = 0; f < data[4]; f++) {
            Font font;
            font.line_height = u8();
            font.ascent = u8();
            for (Glyph& g : font.glyphs) {
                g.advance = u8();
                g.width = u8();
                g.height = u8();
                g.x_offset = static_cast<int8_t>(u8());
                g.y_offset = static_cast<int8_t>(u8());
                g.bitmap_offset = u32();
            }
            uint32_t size = u32();
            font.bitmap.assign(data.begin() + pos, data.begin() + pos + size);
            pos += size;
            s.fonts.push_back(std::move(font));
        }
        return s.fonts.size() >= 2;
    }

    void load_spec(State& s, bm64text::Language language) {
        s.loaded = true;
        s.loaded_language = language;
        s.files.clear();
        if (language != bm64text::Language::Portuguese) {
            return;
        }
        if (s.fonts.empty() && !load_fonts(s)) {
            return;
        }
        std::filesystem::path path = zelda64::get_program_path() / "translations" / "pt-BR-dialogue.txt";
        std::ifstream in(path);
        std::string line;
        std::vector<Op>* current = nullptr;
        int line_number = 0;
        while (std::getline(in, line)) {
            line_number++;
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty() || line[0] == '#') {
                continue;
            }
            if (line[0] == '[') {
                current = &s.files[atoi(line.c_str() + 1)];
                continue;
            }
            Op op;
            std::string head = line;
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                head = line.substr(0, colon);
                op.text = line.substr(colon + 1);
                if (!op.text.empty() && op.text[0] == ' ') {
                    op.text.erase(0, 1);
                }
            }
            std::istringstream fields(head);
            std::string kind;
            fields >> kind >> op.y0 >> op.y1 >> op.x0 >> op.x1;
            if (kind == "line") op.kind = Op::Kind::Line;
            else if (kind == "small") op.kind = Op::Kind::Small;
            else if (kind == "big") op.kind = Op::Kind::Big;
            else if (kind == "clear") op.kind = Op::Kind::Clear;
            else if (kind == "cleartext") op.kind = Op::Kind::ClearText;
            else {
                fprintf(stderr, "[dialogue] line %d: unknown entry \"%s\"\n", line_number, kind.c_str());
                continue;
            }
            if (current == nullptr || fields.fail()) {
                fprintf(stderr, "[dialogue] line %d: malformed entry\n", line_number);
                continue;
            }
            current->push_back(std::move(op));
        }
        printf("[dialogue] %zu dialogue files loaded from %s\n", s.files.size(), path.string().c_str());
    }

    // Decodes one UTF-8 character (Latin-1 range is all the font has).
    uint32_t next_code_point(const std::string& text, size_t& i) {
        uint8_t c = static_cast<uint8_t>(text[i++]);
        if (c < 0x80) {
            return c;
        }
        if ((c & 0xE0) == 0xC0 && i < text.size()) {
            return ((c & 0x1F) << 6) | (static_cast<uint8_t>(text[i++]) & 0x3F);
        }
        while (i < text.size() && (static_cast<uint8_t>(text[i]) & 0xC0) == 0x80) {
            i++;
        }
        return '?';
    }

    // The image: 4bpp, two pixels per byte (left pixel in the high nibble), in RDRAM.
    struct Image {
        uint8_t* rdram;
        uint32_t vram;
        int width, height;

        uint8_t* byte(int x, int y) const {
            uint32_t addr = vram + static_cast<uint32_t>((y * width + x) / 2);
            return &rdram[(addr - 0x80000000u) ^ 3];
        }
        uint8_t get(int x, int y) const {
            if (x < 0 || y < 0 || x >= width || y >= height) return 0;
            uint8_t b = *byte(x, y);
            return (x & 1) ? (b & 0xF) : (b >> 4);
        }
        void set(int x, int y, uint8_t v) const {
            if (x < 0 || y < 0 || x >= width || y >= height) return;
            uint8_t* b = byte(x, y);
            *b = (x & 1) ? static_cast<uint8_t>((*b & 0xF0) | v) : static_cast<uint8_t>((*b & 0x0F) | (v << 4));
        }
    };

    void apply_op(const Image& image, const std::vector<uint8_t>& original, uint8_t text_color, const Op& op,
                  const State& s, int file) {
        auto original_at = [&](int x, int y) -> uint8_t {
            size_t i = static_cast<size_t>(y) * image.width + x;
            return i < original.size() ? original[i] : 0;
        };
        // The text color: the most common color of the original line (the text is drawn with one palette index;
        // icons use other ones).
        int counts[16] = {};
        for (int y = op.y0; y <= op.y1; y++) {
            for (int x = op.x0; x <= op.x1; x++) {
                counts[original_at(x, y)]++;
            }
        }
        uint8_t ink = 1;
        for (int c = 1; c < 16; c++) {
            if (counts[c] > counts[ink]) {
                ink = static_cast<uint8_t>(c);
            }
        }
        // On short lines an icon can outnumber the text: prefer the file's text color when the line has it.
        if (counts[text_color] > 0) {
            ink = text_color;
        }
        for (int y = op.y0; y <= op.y1; y++) {
            for (int x = op.x0; x <= op.x1; x++) {
                // cleartext only removes the text, leaving a drawing that overlaps it.
                if (op.kind != Op::Kind::ClearText || image.get(x, y) == text_color) {
                    image.set(x, y, 0);
                }
            }
        }
        if (op.kind == Op::Kind::ClearText) {
            // Text that was over a drawing leaves holes in it: fill each erased pixel that is surrounded by the
            // drawing with the drawing's most common color around it.
            for (int y = op.y0; y <= op.y1; y++) {
                for (int x = op.x0; x <= op.x1; x++) {
                    if (original_at(x, y) != text_color) {
                        continue;
                    }
                    int around[16] = {};
                    int drawing = 0;
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            uint8_t v = image.get(x + dx, y + dy);
                            if (v != 0 && v != text_color) {
                                around[v]++;
                                drawing++;
                            }
                        }
                    }
                    if (drawing >= 3) {
                        uint8_t best = 0;
                        for (int c = 1; c < 16; c++) {
                            if (around[c] > around[best]) best = static_cast<uint8_t>(c);
                        }
                        image.set(x, y, best);
                    }
                }
            }
        }
        if (op.kind == Op::Kind::Clear || op.kind == Op::Kind::ClearText) {
            return;
        }
        const Font& font = s.fonts[op.kind == Op::Kind::Small ? 1 : 0];
        // "big" draws the dialogue font at twice its size (for shouts drawn with large letters).
        const int scale = op.kind == Op::Kind::Big ? 2 : 1;
        // Align the top of the capitals with the top of the original line.
        int origin_y = op.y0 - font.glyph('H')->y_offset * scale;
        // A leading '>' right-aligns the text in its area and '^' centers it.
        std::string text = op.text;
        char align = 0;
        if (!text.empty() && (text[0] == '>' || text[0] == '^')) {
            align = text[0];
            text.erase(0, 1);
        }
        // Lays the text out from `start` (pixels); draws it if `draw`. Returns the end position. Glyph advances are
        // in quarter pixels.
        auto run = [&](int start, bool draw) {
            int pen = start * 4;
            for (size_t i = 0; i < text.size();) {
                if (text[i] == '{') {
                    size_t end = text.find('}', i);
                    int from = 0, to = -1;
                    int row_from = op.y0 - 5, row_to = op.y1 + 3;
                    int fields = end == std::string::npos ? 0 :
                        sscanf(text.c_str() + i + 1, "%d-%d %d-%d", &from, &to, &row_from, &row_to);
                    if (fields == 2 || fields == 4) {
                        // Inline icon: its columns of the original line, plus the label some icons have above
                        // them (without the text of the neighboring lines, which is in the text color). The rows
                        // can be given explicitly ({x0-x1 y0-y1}) when another icon is too close.
                        int x_out = (pen + 3) / 4;
                        for (int x = from; x <= to; x++, x_out++) {
                            for (int y = row_from; draw && y <= row_to; y++) {
                                uint8_t v = original_at(x, std::clamp(y, 0, image.height - 1));
                                bool inside = y >= op.y0 && y <= op.y1;
                                if (inside || (v != 0 && v != ink)) {
                                    image.set(x_out, y, v);
                                }
                            }
                        }
                        pen = (x_out + 1) * 4;
                        i = end + 1;
                        continue;
                    }
                }
                const Glyph* g = font.glyph(next_code_point(text, i));
                int x = pen / 4;
                for (int gy = 0; draw && gy < g->height; gy++) {
                    for (int gx = 0; gx < g->width; gx++) {
                        if (font.bitmap[g->bitmap_offset + gy * g->width + gx]) {
                            for (int sy = 0; sy < scale; sy++) {
                                for (int sx = 0; sx < scale + (scale > 1 ? 1 : 0); sx++) {
                                    image.set(x + (g->x_offset + gx) * scale + sx, origin_y + (g->y_offset + gy) * scale + sy, ink);
                                }
                            }
                        }
                    }
                }
                pen += g->advance * scale;
            }
            return (pen + 3) / 4;
        };
        int width = run(0, false);
        int start = op.x0;
        if (align == '>') {
            start = op.x1 + 1 - width;
        } else if (align == '^') {
            start = op.x0 + (op.x1 - op.x0 + 1 - width) / 2;
        }
        int pen = run(start, true);
        if (pen - 1 > op.x1) {
            fprintf(stderr, "[dialogue] file %d, line at y %d: text is %d px too wide\n", file, op.y0, pen - 1 - op.x1);
        }
    }
}

// Called by the file loader patch (func_8026CE28) after a file is loaded into RDRAM: a0 = file ID, a1 = data,
// a2 = size.
extern "C" void bm64_on_file_loaded(uint8_t* rdram, recomp_context* ctx) {
    int file = static_cast<int32_t>(ctx->r4);
    uint32_t data = static_cast<uint32_t>(ctx->r5);
    State& s = state();
    std::lock_guard lock{ s.mutex };
    bm64text::Language language = bm64text::get_language();
    if (!s.loaded || s.loaded_language != language) {
        load_spec(s, language);
    }
    auto found = s.files.find(file);
    if (found == s.files.end()) {
        return;
    }
    auto read16 = [&](uint32_t addr) {
        uint16_t v;
        memcpy(&v, rdram + ((addr - 0x80000000u) ^ 2), 2);
        return v;
    };
    auto read32 = [&](uint32_t addr) {
        uint32_t v;
        memcpy(&v, rdram + (addr - 0x80000000u), 4);
        return v;
    };
    Image image{ rdram, data + read32(data + 0x14), read16(data + 0x10), read16(data + 0x12) };
    if (image.width != 256 || image.height <= 0 || image.height > 1024) {
        fprintf(stderr, "[dialogue] file %d isn't a dialogue image\n", file);
        return;
    }
    // Keep the original pixels: inline icons are copied from them after their line has been erased.
    std::vector<uint8_t> original(static_cast<size_t>(image.width) * image.height);
    for (int y = 0; y < image.height; y++) {
        for (int x = 0; x < image.width; x++) {
            original[static_cast<size_t>(y) * image.width + x] = image.get(x, y);
        }
    }
    // The file's text color: the most common color besides the background in its text lines (a big drawing can
    // have more pixels than the text).
    int counts[16] = {};
    for (const Op& op : found->second) {
        if (op.kind != Op::Kind::Line) {
            continue;
        }
        for (int y = std::max(op.y0, 0); y <= op.y1 && y < image.height; y++) {
            for (int x = std::max(op.x0, 0); x <= op.x1 && x < image.width; x++) {
                counts[original[static_cast<size_t>(y) * image.width + x]]++;
            }
        }
    }
    uint8_t text_color = 1;
    for (int c = 1; c < 16; c++) {
        if (counts[c] > counts[text_color]) {
            text_color = static_cast<uint8_t>(c);
        }
    }
    for (const Op& op : found->second) {
        apply_op(image, original, text_color, op, s, file);
    }
}
