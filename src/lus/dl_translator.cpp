#include "dl_translator.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "librecomp/addresses.hpp"

namespace {
    // F3DEX opcodes (the game uses F3DEX.NoN 1.21).
    constexpr uint8_t G_SPNOOP = 0x00;
    constexpr uint8_t G_MTX = 0x01;
    constexpr uint8_t G_MOVEMEM = 0x03;
    constexpr uint8_t G_VTX = 0x04;
    constexpr uint8_t G_DL = 0x06;
    constexpr uint8_t G_LOAD_UCODE = 0xAF;
    constexpr uint8_t G_BRANCH_Z = 0xB0;
    constexpr uint8_t G_RDPHALF_1 = 0xB4;
    constexpr uint8_t G_ENDDL = 0xB8;
    constexpr uint8_t G_MOVEWORD = 0xBC;
    constexpr uint8_t G_CULLDL = 0xBE;
    constexpr uint8_t G_TRI1 = 0xBF;
    constexpr uint8_t G_TRI2 = 0xB1;
    constexpr uint8_t G_QUAD = 0xB5;
    constexpr uint8_t G_SETOTHERMODE_L = 0xB9;
    constexpr uint8_t G_SETOTHERMODE_H = 0xBA;

    // RDP opcodes that carry addresses or describe texture loads.
    constexpr uint8_t G_TEXRECT = 0xE4;
    constexpr uint8_t G_TEXRECTFLIP = 0xE5;
    constexpr uint8_t G_SETSCISSOR = 0xED;
    constexpr uint8_t G_RDPSETOTHERMODE = 0xEF;
    constexpr uint8_t G_SETFOGCOLOR = 0xF8;
    constexpr uint8_t G_SETPRIMCOLOR = 0xFA;
    constexpr uint8_t G_SETCOMBINE = 0xFC;
    constexpr uint8_t G_LOADTLUT = 0xF0;
    constexpr uint8_t G_LOADBLOCK = 0xF3;
    constexpr uint8_t G_LOADTILE = 0xF4;
    constexpr uint8_t G_FILLRECT = 0xF6;
    constexpr uint8_t G_SETTIMG = 0xFD;
    constexpr uint8_t G_SETZIMG = 0xFE;
    constexpr uint8_t G_SETCIMG = 0xFF;

    // G_MOVEMEM indices.
    constexpr uint8_t G_MV_VIEWPORT = 0x80;
    constexpr uint8_t G_MV_LOOKATY = 0x82;
    constexpr uint8_t G_MV_L7 = 0x94;

    constexpr uint8_t G_MW_SEGMENT = 0x06;

    // libultraship's own opcodes for using GPU framebuffers as textures (see OTR_G_* in fast/lus_gbi.h).
    constexpr uint8_t OTR_G_SETTIMG_FB = 0x23;
    constexpr uint8_t OTR_G_COPYFB = 0x3B;
    constexpr uint8_t OTR_G_IMAGERECT = 0x3C;
    // The game renders 320x240 16-bit framebuffers.
    constexpr uint32_t framebuffer_height = 240;

    // RT64 extended GBI (see third_party/rt64/include/rt64_extended_gbi.h).
    constexpr uint32_t RT64_HOOK_MAGIC_NUMBER = 0x525464;
    constexpr uint32_t RT64_HOOK_OP_GETVERSION = 0x0;
    constexpr uint32_t RT64_HOOK_OP_ENABLE = 0x1;
    constexpr uint32_t RT64_HOOK_OP_DISABLE = 0x2;
    constexpr uint32_t RT64_HOOK_OP_DL = 0x3;
    constexpr uint32_t RT64_HOOK_OP_BRANCH = 0x4;
    constexpr uint32_t G_EX_VERSION = 0x1;

    constexpr uint32_t G_EX_TEXRECT_V1 = 0x02;
    constexpr uint32_t G_EX_FILLRECT_V1 = 0x03;
    constexpr uint32_t G_EX_SETVIEWPORT_V1 = 0x04;
    constexpr uint32_t G_EX_SETSCISSOR_V1 = 0x05;
    constexpr uint32_t G_EX_SETRECTALIGN_V1 = 0x06;
    constexpr uint32_t G_EX_SETVIEWPORTALIGN_V1 = 0x07;
    constexpr uint32_t G_EX_SETSCISSORALIGN_V1 = 0x08;
    constexpr uint32_t G_EX_MATRIXGROUP_V1 = 0x0C;
    constexpr uint32_t G_EX_EDITGROUPBYADDRESS_V1 = 0x13;
    constexpr uint32_t G_EX_VERTEX_V1 = 0x14;
    constexpr uint32_t G_EX_MATRIX_FLOAT_V1 = 0x30;
    constexpr uint32_t G_EX_SETVERTEXSEGMENT_V1 = 0x31;
    constexpr uint32_t G_EX_ORIGIN_NONE = 0x800;

    constexpr size_t arena_chunk_size = 1024 * 1024;
    constexpr size_t max_dl_depth = 32;
    // Guards against runaway display lists (e.g. reading garbage after a bad pointer).
    constexpr size_t max_commands_per_task = 1 << 20;
#ifdef __SWITCH__
    // Applet mode leaves little memory for anything beyond the game itself.
    constexpr size_t texture_store_limit = 32 * 1024 * 1024;
#else
    constexpr size_t texture_store_limit = 96 * 1024 * 1024;
#endif
    constexpr uint32_t texture_keep_frames = 120;

    uint64_t hash_bytes(const uint8_t* data, size_t size, uint64_t seed) {
        uint64_t h = seed ^ (size * 0x9E3779B97F4A7C15ULL);
        size_t i = 0;
        for (; i + 8 <= size; i += 8) {
            uint64_t v;
            memcpy(&v, data + i, 8);
            h ^= v * 0xBF58476D1CE4E5B9ULL;
            h = (h << 27) | (h >> 37);
            h *= 0x94D049BB133111EBULL;
        }
        for (; i < size; i++) {
            h ^= data[i];
            h *= 0x100000001B3ULL;
        }
        h ^= h >> 31;
        return h;
    }

    uint32_t texel_bytes(uint32_t siz, uint32_t texels) {
        switch (siz) {
            case 0: return (texels + 1) / 2;
            case 1: return texels;
            case 2: return texels * 2;
            default: return texels * 4;
        }
    }

    // Converts an extended-GBI x coordinate relative to an origin into a 4:3 screen coordinate.
    int32_t apply_origin(uint32_t origin, int32_t value) {
        if (origin == G_EX_ORIGIN_NONE) {
            return value;
        }
        // G_EX_ORIGIN_LEFT = 0, CENTER = 0x200, RIGHT = 0x400: the origin is a fraction of the screen width
        // in 1/1024 units. The game renders a 320 wide framebuffer.
        return value + static_cast<int32_t>((320 * 4 * origin) / 0x400);
    }
}

namespace bm64lus {
    DisplayListTranslator::DisplayListTranslator(uint8_t* rdram) : rdram(rdram) {
        cmds.reserve(64 * 1024);
    }

    void DisplayListTranslator::begin_frame() {
        cmds.clear();
        arena_chunk_index = 0;
        arena_offset = 0;
        frame_counter++;
        timg_is_framebuffer = false;

        if (dump_file != nullptr) {
            fclose(dump_file);
            dump_file = nullptr;
        }
        if (dump_interval != 0 && frame_counter % dump_interval == 0) {
            char name[64];
            snprintf(name, sizeof(name), "dl_%06u.txt", frame_counter);
            dump_file = fopen(name, "w");
            fprintf(stderr, "[LUS] dumping %s\n", name);
        }
        if (texture_bytes > texture_store_limit) {
            evict_old_textures();
        }
    }

    HostGfx* DisplayListTranslator::finish_frame() {
        emit(static_cast<uint32_t>(G_ENDDL) << 24, 0);
        return cmds.data();
    }

    bool DisplayListTranslator::blends_with_fog_alpha() const {
        uint32_t cycle_type = (othermode_h >> 20) & 0x3;
        if (cycle_type > 1) {
            // Copy and fill modes don't use the blender.
            return false;
        }
        // The blender's alpha source (A) of the cycle that produces the final color: bits 26-27 in 1-cycle mode,
        // bits 24-25 for the second cycle in 2-cycle mode. 1 is G_BL_A_FOG.
        uint32_t alpha_source = cycle_type == 0 ? (othermode_l >> 26) & 0x3 : (othermode_l >> 24) & 0x3;
        return alpha_source == 1;
    }

    void DisplayListTranslator::emit_draw(uint32_t w0, uintptr_t w1, const uint32_t* extra_words, size_t extra_count) {
        bool fog_alpha = blends_with_fog_alpha();
        if (fog_alpha) {
            // The interpreter blends with the combiner's alpha output, so fold the fog alpha into it: the primitive
            // alpha becomes the fog alpha and each cycle's alpha becomes (original alpha) * PRIMITIVE. The original
            // alpha still matters, as these modes usually also use it as coverage (e.g. the round character
            // shadows, which are a single triangle cut out by a clamped texture's alpha).
            // Alpha combiner selectors: A/B/D 1 = TEXEL0 ... 3 = PRIMITIVE, 6 = 1.0, 7 = 0; C 3 = PRIMITIVE, 7 = 0.
            auto scaled_alpha = [](uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t& na, uint32_t& nb, uint32_t& nc, uint32_t& nd) {
                (void)a;
                (void)b;
                if (c == 7 && d != 3) {
                    // alpha = D, a single input: use (D - 0) * PRIMITIVE + 0.
                    na = d;
                } else {
                    // alpha = PRIMITIVE already, or an expression that can't be rewritten: use the fog alpha alone.
                    na = 6;
                }
                nb = 7;
                nc = 3;
                nd = 7;
            };
            uint32_t a0 = (combine_w0 >> 12) & 7, c0 = (combine_w0 >> 9) & 7, b0 = (combine_w1 >> 12) & 7, d0 = (combine_w1 >> 9) & 7;
            uint32_t a1 = (combine_w1 >> 21) & 7, c1 = (combine_w1 >> 18) & 7, b1 = (combine_w1 >> 3) & 7, d1 = combine_w1 & 7;
            uint32_t na0, nb0, nc0, nd0, na1, nb1, nc1, nd1;
            scaled_alpha(a0, b0, c0, d0, na0, nb0, nc0, nd0);
            scaled_alpha(a1, b1, c1, d1, na1, nb1, nc1, nd1);
            uint32_t cc_w0 = (combine_w0 & ~0x7E00u) | (na0 << 12) | (nc0 << 9);
            uint32_t cc_w1 = (combine_w1 & ~0x00FC7E3Fu) | (na1 << 21) | (nc1 << 18) | (nb0 << 12) | (nd0 << 9) | (nb1 << 3) | nd1;
            emit(cc_w0, cc_w1);
            emit(prim_color_w0, (prim_color_w1 & 0xFFFFFF00u) | (fog_color & 0xFFu));
        }

        emit(w0, w1);
        for (size_t i = 0; i + 1 < extra_count * 2; i += 2) {
            emit(extra_words[i], extra_words[i + 1]);
        }

        if (fog_alpha) {
            emit(combine_w0, combine_w1);
            emit(prim_color_w0, prim_color_w1);
        }
    }

    void DisplayListTranslator::end_framebuffer_reads() {
        if (!timg_is_framebuffer) {
            return;
        }
        timg_is_framebuffer = false;
        // The game composites its previous frame into the current one (motion blur), so keep a copy of the result
        // for the next frame. Copying right here, rather than once the frame is presented, keeps the copy correct
        // when the next frame's commands are already queued behind this one.
        emit((static_cast<uint32_t>(OTR_G_COPYFB) << 24) | (static_cast<uint32_t>(previous_frame_fb) << 11), 0);
        last_framebuffer_copy_frame = frame_counter;
    }

    bool DisplayListTranslator::find_color_image(uint32_t phys, uint32_t& base_out, uint32_t& width_out) const {
        for (const auto& [base, width] : color_images) {
            if (phys >= base && phys < base + width * framebuffer_height * 2) {
                base_out = base;
                width_out = width;
                return true;
            }
        }
        return false;
    }

    void DisplayListTranslator::emit_framebuffer_rect(uint32_t w0, uint32_t w1, uint32_t st, uint32_t dsdt) {
        // Texture rectangle: U10.2 screen coordinates, S10.5 texture origin and S5.10 per-pixel steps.
        uint32_t lrx = (w0 >> 12) & 0xFFF;
        uint32_t lry = w0 & 0xFFF;
        uint32_t tile = (w1 >> 24) & 0x7;
        uint32_t ulx = (w1 >> 12) & 0xFFF;
        uint32_t uly = w1 & 0xFFF;
        int32_t s = static_cast<int16_t>(st >> 16);
        int32_t t = static_cast<int16_t>(st & 0xFFFF);
        int32_t dsdx = static_cast<int16_t>(dsdt >> 16);
        int32_t dtdy = static_cast<int16_t>(dsdt & 0xFFFF);

        // The texture image pointed at a row inside the framebuffer, so offset t by that row.
        int32_t s0 = s / 32;
        int32_t t0 = t / 32 + static_cast<int32_t>(timg_fb_row);
        int32_t s1 = s0 + static_cast<int32_t>((lrx - ulx) / 4) * dsdx / 1024;
        int32_t t1 = t0 + static_cast<int32_t>((lry - uly) / 4) * dtdy / 1024;

        auto pack = [](int32_t hi, int32_t lo) {
            return (static_cast<uint32_t>(hi & 0xFFFF) << 16) | static_cast<uint32_t>(lo & 0xFFFF);
        };

        emit(static_cast<uint32_t>(OTR_G_SETTIMG_FB) << 24, static_cast<uintptr_t>(previous_frame_fb));
        emit((static_cast<uint32_t>(OTR_G_IMAGERECT) << 24) | tile, pack(timg_fb_width, framebuffer_height));
        emit(pack(ulx, uly), pack(s0, t0));
        emit(pack(lrx, lry), pack(s1, t1));
    }

    void DisplayListTranslator::clear_texture_store() {
        if (on_texture_evicted) {
            for (auto& [key, tex] : texture_store) {
                on_texture_evicted(tex.data.get());
            }
        }
        texture_store.clear();
        texture_bytes = 0;
    }

    void DisplayListTranslator::evict_old_textures() {
        for (auto it = texture_store.begin(); it != texture_store.end();) {
            if (frame_counter - it->second.last_used_frame > texture_keep_frames) {
                if (on_texture_evicted) {
                    on_texture_evicted(it->second.data.get());
                }
                texture_bytes -= it->second.size;
                it = texture_store.erase(it);
            } else {
                ++it;
            }
        }
    }

    void* DisplayListTranslator::arena_alloc(size_t size) {
        size = (size + 15) & ~size_t{15};
        if (arena_chunk_index < arena_chunks.size() && arena_offset + size > arena_chunk_size) {
            arena_chunk_index++;
            arena_offset = 0;
        }
        if (arena_chunk_index >= arena_chunks.size()) {
            arena_chunks.emplace_back(new uint8_t[arena_chunk_size]);
            arena_offset = 0;
        }
        void* ret = arena_chunks[arena_chunk_index].get() + arena_offset;
        arena_offset += size;
        return ret;
    }

    bool DisplayListTranslator::resolve(uint32_t address, uint32_t size, uint32_t& phys_out) const {
        uint32_t phys;
        if ((address & 0xF0000000) == 0x80000000 || (address & 0xF0000000) == 0xA0000000) {
            // KSEG0/KSEG1 virtual address. The RSP would mask these through segment 0, but the patches
            // place display lists in extended RAM above 8MB, so keep the full physical offset.
            phys = address & 0x1FFFFFFF;
        } else {
            phys = (segments[(address >> 24) & 0x0F] + (address & 0x00FFFFFF)) & 0x1FFFFFFF;
        }
        if (static_cast<uint64_t>(phys) + size > recomp::mem_size) {
            return false;
        }
        phys_out = phys;
        return true;
    }

    // RDRAM holds the game's big-endian memory as native-endian 32-bit words, so sub-word accesses
    // flip the low address bits.
    uint32_t DisplayListTranslator::read32(uint32_t phys) const {
        uint32_t ret;
        memcpy(&ret, rdram + phys, sizeof(ret));
        return ret;
    }

    uint16_t DisplayListTranslator::read16(uint32_t phys) const {
        uint16_t ret;
        memcpy(&ret, rdram + (phys ^ 2), sizeof(ret));
        return ret;
    }

    uint8_t DisplayListTranslator::read8(uint32_t phys) const {
        return rdram[phys ^ 3];
    }

    uintptr_t DisplayListTranslator::stage_vertices(uint32_t phys, uint32_t count) {
        // Matches F3DVtx_t in libultraship (non GBI_FLOATS).
        struct HostVtx {
            int16_t ob[3];
            uint16_t flag;
            int16_t tc[2];
            uint8_t cn[4];
        };
        static_assert(sizeof(HostVtx) == 16);

        HostVtx* out = static_cast<HostVtx*>(arena_alloc(sizeof(HostVtx) * count));
        for (uint32_t i = 0; i < count; i++) {
            uint32_t base = phys + i * 16;
            uint32_t w0 = read32(base + 0);
            uint32_t w1 = read32(base + 4);
            uint32_t w2 = read32(base + 8);
            uint32_t w3 = read32(base + 12);
            out[i].ob[0] = static_cast<int16_t>(w0 >> 16);
            out[i].ob[1] = static_cast<int16_t>(w0 & 0xFFFF);
            out[i].ob[2] = static_cast<int16_t>(w1 >> 16);
            out[i].flag = static_cast<uint16_t>(w1 & 0xFFFF);
            out[i].tc[0] = static_cast<int16_t>(w2 >> 16);
            out[i].tc[1] = static_cast<int16_t>(w2 & 0xFFFF);
            out[i].cn[0] = static_cast<uint8_t>(w3 >> 24);
            out[i].cn[1] = static_cast<uint8_t>(w3 >> 16);
            out[i].cn[2] = static_cast<uint8_t>(w3 >> 8);
            out[i].cn[3] = static_cast<uint8_t>(w3);
            if (dump_file != nullptr) {
                fprintf(dump_file, "    v%u: pos %d %d %d flag %04X tc %d %d rgba %02X%02X%02X%02X\n", i, out[i].ob[0], out[i].ob[1],
                        out[i].ob[2], out[i].flag, out[i].tc[0], out[i].tc[1], out[i].cn[0], out[i].cn[1], out[i].cn[2], out[i].cn[3]);
            }
        }
        return reinterpret_cast<uintptr_t>(out);
    }

    uintptr_t DisplayListTranslator::stage_matrix(uint32_t phys) {
        // The interpreter reads s15.16 matrices as 16 native int32 words: 8 words of integer halves followed
        // by 8 words of fractional halves, each word holding two big-endian halves. That's exactly how the
        // recompiled game stores them, so a word copy is enough. The copy keeps the frame's state even if the
        // game reuses the matrix buffer before the frame is presented.
        uint32_t* out = static_cast<uint32_t*>(arena_alloc(64));
        memcpy(out, rdram + phys, 64);
        return reinterpret_cast<uintptr_t>(out);
    }

    uintptr_t DisplayListTranslator::stage_bytes(uint32_t phys, uint32_t size) {
        uint8_t* out = static_cast<uint8_t*>(arena_alloc(size));
        for (uint32_t i = 0; i < size; i++) {
            out[i] = read8(phys + i);
        }
        return reinterpret_cast<uintptr_t>(out);
    }

    uintptr_t DisplayListTranslator::stage_viewport(uint32_t phys) {
        int16_t* out = static_cast<int16_t*>(arena_alloc(16));
        for (uint32_t i = 0; i < 8; i++) {
            out[i] = static_cast<int16_t>(read16(phys + i * 2));
        }
        if (dump_file != nullptr) {
            fprintf(dump_file, "    viewport scale %d %d %d trans %d %d %d\n", out[0], out[1], out[2], out[4], out[5], out[6]);
        }
        return reinterpret_cast<uintptr_t>(out);
    }

    const uint8_t* DisplayListTranslator::stage_texture(uint32_t phys, uint32_t size) {
        // Convert to big-endian byte order into a scratch buffer, then deduplicate by content.
        static thread_local std::vector<uint8_t> scratch;
        scratch.resize(size);
        for (uint32_t i = 0; i < size; i++) {
            scratch[i] = read8(phys + i);
        }

        uint64_t key = hash_bytes(scratch.data(), size, phys);
        stats.textures_staged++;
        stats.texture_bytes_hashed += size;
        auto it = texture_store.find(key);
        if (it == texture_store.end()) {
            stats.textures_new++;
            StagedTexture tex;
            tex.data.reset(new uint8_t[size]);
            memcpy(tex.data.get(), scratch.data(), size);
            tex.size = size;
            texture_bytes += size;
            it = texture_store.emplace(key, std::move(tex)).first;
        }
        it->second.last_used_frame = frame_counter;
        return it->second.data.get();
    }

    void DisplayListTranslator::note_texture_load(uint32_t w0, uint32_t w1) {
        if (!pending_timg.active) {
            return;
        }
        uint8_t op = w0 >> 24;
        uint32_t end = 0;
        switch (op) {
            case G_LOADBLOCK: {
                uint32_t uls = (w0 >> 12) & 0xFFF;
                uint32_t ult = w0 & 0xFFF;
                uint32_t lrs = (w1 >> 12) & 0xFFF;
                end = texel_bytes(pending_timg.siz, ult * pending_timg.width + uls + lrs + 1);
                break;
            }
            case G_LOADTILE: {
                uint32_t lrt = (w1 & 0xFFF) >> 2;
                end = texel_bytes(pending_timg.siz, (lrt + 1) * pending_timg.width);
                break;
            }
            case G_LOADTLUT: {
                uint32_t uls = (w0 >> 14) & 0x3FF;
                uint32_t count = ((w1 >> 14) & 0x3FF) + 1;
                // The interpreter may read a full 256 entry palette for CI8.
                end = std::max<uint32_t>((uls + count) * 2, 512);
                break;
            }
        }
        pending_timg.needed_bytes = std::max(pending_timg.needed_bytes, end);
    }

    void DisplayListTranslator::finalize_texture_image() {
        if (!pending_timg.active) {
            return;
        }
        pending_timg.active = false;

        uint32_t size = (pending_timg.needed_bytes + 7) & ~7u;
        if (size == 0 || static_cast<uint64_t>(pending_timg.phys) + size > recomp::mem_size) {
            cmds[pending_timg.cmd_index].w1 = host_ptr(pending_timg.phys);
            return;
        }
        cmds[pending_timg.cmd_index].w1 = reinterpret_cast<uintptr_t>(stage_texture(pending_timg.phys, size));
    }

    uint32_t DisplayListTranslator::translate_extended(uint32_t w0, uint32_t w1, uint32_t pc) {
        uint32_t ex_op = w0 & 0xFFFFFF;
        switch (ex_op) {
            case G_EX_TEXRECT_V1:
            case G_EX_SETSCISSORALIGN_V1:
                return 2;
            case G_EX_FILLRECT_V1: {
                // Word layout: w1 = origins, then ulx/uly and lrx/lry in 10.2 fixed point.
                uint32_t lorigin = (w1 >> 0) & 0xFFF;
                uint32_t rorigin = (w1 >> 12) & 0xFFF;
                uint32_t ul = read32(pc + 0);
                uint32_t lr = read32(pc + 4);
                int32_t ulx = apply_origin(lorigin, static_cast<int16_t>(ul >> 16));
                int32_t uly = static_cast<int16_t>(ul & 0xFFFF);
                int32_t lrx = apply_origin(rorigin, static_cast<int16_t>(lr >> 16));
                int32_t lry = static_cast<int16_t>(lr & 0xFFFF);
                ulx = std::clamp(ulx, 0, 0xFFF);
                lrx = std::clamp(lrx, 0, 0xFFF);
                uly = std::clamp(uly, 0, 0xFFF);
                lry = std::clamp(lry, 0, 0xFFF);
                emit((static_cast<uint32_t>(G_FILLRECT) << 24) | (lrx << 12) | lry, (ulx << 12) | uly);
                return 1;
            }
            case G_EX_SETSCISSOR_V1: {
                uint32_t mode = w1 & 0x3;
                uint32_t lorigin = (w1 >> 2) & 0xFFF;
                uint32_t rorigin = (w1 >> 14) & 0xFFF;
                uint32_t ul = read32(pc + 0);
                uint32_t lr = read32(pc + 4);
                int32_t ulx = apply_origin(lorigin, static_cast<int16_t>(ul >> 16));
                int32_t uly = static_cast<int16_t>(ul & 0xFFFF);
                int32_t lrx = apply_origin(rorigin, static_cast<int16_t>(lr >> 16));
                int32_t lry = static_cast<int16_t>(lr & 0xFFFF);
                ulx = std::clamp(ulx, 0, 0xFFF);
                lrx = std::clamp(lrx, 0, 0xFFF);
                uly = std::clamp(uly, 0, 0xFFF);
                lry = std::clamp(lry, 0, 0xFFF);
                emit((static_cast<uint32_t>(G_SETSCISSOR) << 24) | (ulx << 12) | uly, (mode << 24) | (lrx << 12) | lry);
                return 1;
            }
            case G_EX_SETVIEWPORT_V1: {
                uint32_t vp_address = read32(pc + 4);
                uint32_t phys;
                if (resolve(vp_address, 16, phys)) {
                    uint32_t mm_w0 = (static_cast<uint32_t>(G_MOVEMEM) << 24) | (G_MV_VIEWPORT << 16) | 16;
                    emit(mm_w0, reinterpret_cast<uintptr_t>(stage_viewport(phys)));
                }
                return 1;
            }
            case G_EX_SETRECTALIGN_V1:
            case G_EX_SETVIEWPORTALIGN_V1:
            case G_EX_MATRIXGROUP_V1:
            case G_EX_EDITGROUPBYADDRESS_V1:
            case G_EX_VERTEX_V1:
            case G_EX_MATRIX_FLOAT_V1:
            case G_EX_SETVERTEXSEGMENT_V1:
                return 1;
            default:
                // Every other extended command is a single word: state hints for RT64's interpolation and
                // widescreen handling that have no equivalent here.
                return 0;
        }
    }

    void DisplayListTranslator::translate_task(uint32_t dl_address) {
        uint32_t stack[max_dl_depth];
        size_t depth = 0;
        uint32_t pc;
        if (!resolve(dl_address, 8, pc)) {
            return;
        }

        for (size_t processed = 0; processed < max_commands_per_task; processed++) {
            if (static_cast<uint64_t>(pc) + 8 > recomp::mem_size) {
                break;
            }
            uint32_t w0 = read32(pc);
            uint32_t w1 = read32(pc + 4);
            if (dump_file != nullptr) {
                fprintf(dump_file, "%08X: %08X %08X d%zu\n", pc, w0, w1, depth);
            }
            pc += 8;
            uint8_t op = w0 >> 24;

            if (extended_gbi_enabled && op == extended_opcode) {
                pc += 8 * translate_extended(w0, w1, pc);
                continue;
            }

            switch (op) {
                case G_SPNOOP: {
                    if ((w0 & 0xFFFFFF) != RT64_HOOK_MAGIC_NUMBER) {
                        break;
                    }
                    uint32_t hook_op = w1 >> 28;
                    uint32_t hook_value = w1 & 0x0FFFFFFF;
                    switch (hook_op) {
                        case RT64_HOOK_OP_GETVERSION: {
                            uint32_t phys;
                            if (resolve(hook_value, 4, phys)) {
                                uint32_t version = G_EX_VERSION;
                                memcpy(rdram + phys, &version, sizeof(version));
                            }
                            break;
                        }
                        case RT64_HOOK_OP_ENABLE:
                            extended_gbi_enabled = true;
                            extended_opcode = hook_value & 0xFF;
                            break;
                        case RT64_HOOK_OP_DISABLE:
                            extended_gbi_enabled = false;
                            break;
                        case RT64_HOOK_OP_DL:
                        case RT64_HOOK_OP_BRANCH: {
                            uint32_t target;
                            if (!resolve(hook_value, 8, target)) {
                                break;
                            }
                            if (hook_op == RT64_HOOK_OP_DL) {
                                if (depth >= max_dl_depth) {
                                    break;
                                }
                                stack[depth++] = pc;
                            }
                            pc = target;
                            break;
                        }
                    }
                    break;
                }

                case G_DL: {
                    uint32_t target;
                    if (!resolve(w1, 8, target)) {
                        break;
                    }
                    bool push = ((w0 >> 16) & 0xFF) == 0;
                    if (push) {
                        if (depth >= max_dl_depth) {
                            break;
                        }
                        stack[depth++] = pc;
                    }
                    pc = target;
                    break;
                }

                case G_ENDDL:
                    if (depth == 0) {
                        finalize_texture_image();
                        end_framebuffer_reads();
                        return;
                    }
                    pc = stack[--depth];
                    break;

                case G_BRANCH_Z: {
                    // The branch depends on the transformed depth of a vertex, which only the interpreter knows.
                    // Always taking it selects the near (high detail) display list, which is a safe default.
                    uint32_t target;
                    if (resolve(last_rdphalf_1, 8, target)) {
                        pc = target;
                    }
                    break;
                }

                case G_RDPHALF_1:
                    last_rdphalf_1 = w1;
                    emit(w0, w1);
                    break;

                case G_CULLDL:
                case G_LOAD_UCODE:
                    break;

                case G_MTX: {
                    uint32_t phys;
                    if (resolve(w1, 64, phys)) {
                        emit(w0, stage_matrix(phys));
                    }
                    break;
                }

                case G_VTX: {
                    uint32_t count = (w0 >> 10) & 0x3F;
                    uint32_t phys;
                    if (count != 0 && resolve(w1, count * 16, phys)) {
                        emit(w0, stage_vertices(phys, count));
                    }
                    break;
                }

                case G_MOVEMEM: {
                    uint8_t index = (w0 >> 16) & 0xFF;
                    uint32_t phys;
                    if (index == G_MV_VIEWPORT) {
                        if (resolve(w1, 16, phys)) {
                            emit(w0, stage_viewport(phys));
                        }
                    } else if (index >= G_MV_LOOKATY && index <= G_MV_L7) {
                        // Light_t/LookAt are byte arrays: col[3], pad, colc[3], pad, dir[3], pad.
                        if (resolve(w1, 16, phys)) {
                            emit(w0, stage_bytes(phys, 16));
                        }
                    }
                    // Partial matrix loads (G_MV_MATRIX_*) aren't supported by the interpreter.
                    break;
                }

                case G_MOVEWORD: {
                    uint8_t index = w0 & 0xFF;
                    if (index == G_MW_SEGMENT) {
                        uint32_t offset = (w0 >> 8) & 0xFFFF;
                        segments[(offset >> 2) & 0x0F] = w1 & 0x1FFFFFFF;
                    } else {
                        emit(w0, w1);
                    }
                    break;
                }

                case G_SETTIMG: {
                    finalize_texture_image();
                    uint32_t phys;
                    if (!resolve(w1, 0, phys)) {
                        phys = 0;
                    }

                    uint32_t fb_base, fb_width;
                    bool is_framebuffer = create_previous_frame_fb && find_color_image(phys, fb_base, fb_width);
                    if (is_framebuffer && previous_frame_fb < 0) {
                        previous_frame_fb = create_previous_frame_fb();
                        is_framebuffer = previous_frame_fb >= 0;
                    }
                    if (is_framebuffer) {
                        if (!timg_is_framebuffer && last_framebuffer_copy_frame + 1 != frame_counter &&
                            last_framebuffer_copy_frame != frame_counter) {
                            // No copy of the previous frame exists, so use the frame rendered so far instead.
                            emit((static_cast<uint32_t>(OTR_G_COPYFB) << 24) | (static_cast<uint32_t>(previous_frame_fb) << 11), 0);
                        }
                        timg_is_framebuffer = true;
                        timg_fb_width = fb_width;
                        timg_fb_row = (phys - fb_base) / (fb_width * 2);
                        break;
                    }
                    end_framebuffer_reads();

                    pending_timg.active = true;
                    pending_timg.cmd_index = cmds.size();
                    pending_timg.phys = phys;
                    pending_timg.siz = (w0 >> 19) & 0x3;
                    pending_timg.width = (w0 & 0xFFF) + 1;
                    pending_timg.needed_bytes = 0;
                    emit(w0, host_ptr(phys));
                    break;
                }

                case G_LOADBLOCK:
                case G_LOADTILE:
                case G_LOADTLUT:
                    if (timg_is_framebuffer) {
                        // The framebuffer is bound as a texture directly when the rectangle is drawn.
                        break;
                    }
                    note_texture_load(w0, w1);
                    emit(w0, w1);
                    break;

                case G_TEXRECT:
                case G_TEXRECTFLIP:
                    if (timg_is_framebuffer && static_cast<uint64_t>(pc) + 16 <= recomp::mem_size) {
                        // gSPTextureRectangle is followed by two RDPHALF commands with the texture coordinates.
                        uint32_t st = read32(pc + 4);
                        uint32_t dsdt = read32(pc + 12);
                        pc += 16;
                        emit_framebuffer_rect(w0, w1, st, dsdt);
                    } else if (static_cast<uint64_t>(pc) + 16 <= recomp::mem_size) {
                        // Keep the two RDPHALF commands attached so state changes for blending wrap the whole rectangle.
                        const uint32_t halves[4] = { read32(pc), read32(pc + 4), read32(pc + 8), read32(pc + 12) };
                        pc += 16;
                        emit_draw(w0, w1, halves, 2);
                    } else {
                        emit(w0, w1);
                    }
                    break;

                case G_TRI1:
                case G_TRI2:
                case G_QUAD:
                case G_FILLRECT:
                    emit_draw(w0, w1);
                    break;

                case G_SETOTHERMODE_L:
                case G_SETOTHERMODE_H: {
                    // F3DEX: w0 = opcode | shift << 8 | length, w1 = the new bits.
                    uint32_t shift = (w0 >> 8) & 0xFF;
                    uint32_t length = w0 & 0xFF;
                    uint32_t mask = (length >= 32 ? 0xFFFFFFFFu : ((1u << length) - 1)) << shift;
                    uint32_t& mode = op == G_SETOTHERMODE_L ? othermode_l : othermode_h;
                    mode = (mode & ~mask) | (w1 & mask);
                    emit(w0, w1);
                    break;
                }

                case G_RDPSETOTHERMODE:
                    othermode_h = w0 & 0x00FFFFFF;
                    othermode_l = w1;
                    emit(w0, w1);
                    break;

                case G_SETFOGCOLOR:
                    fog_color = w1;
                    emit(w0, w1);
                    break;

                case G_SETPRIMCOLOR:
                    prim_color_w0 = w0;
                    prim_color_w1 = w1;
                    emit(w0, w1);
                    break;

                case G_SETCOMBINE:
                    combine_w0 = w0;
                    combine_w1 = w1;
                    emit(w0, w1);
                    break;

                case G_SETCIMG: {
                    uint32_t phys;
                    if (!resolve(w1, 0, phys)) {
                        phys = 0;
                    }
                    color_images[phys] = (w0 & 0xFFF) + 1;
                    emit(w0, host_ptr(phys));
                    break;
                }

                case G_SETZIMG: {
                    uint32_t phys;
                    if (!resolve(w1, 0, phys)) {
                        phys = 0;
                    }
                    emit(w0, host_ptr(phys));
                    break;
                }

                default:
                    emit(w0, w1);
                    break;
            }
        }

        finalize_texture_image();
        end_framebuffer_reads();
        fprintf(stderr, "[LUS] Display list at 0x%08X did not terminate\n", dl_address);
    }
}
