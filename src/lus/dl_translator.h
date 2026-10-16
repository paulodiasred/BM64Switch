#ifndef __BM64_DL_TRANSLATOR_H__
#define __BM64_DL_TRANSLATOR_H__

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

// Converts the F3DEX display lists the recompiled game writes into emulated RDRAM into the
// pointer-sized, host-endian command stream that libultraship's Fast3D interpreter executes.
//
// The interpreter expects every address in a command to be a host pointer to data in host byte order
// (except textures, which it decodes as big-endian bytes). So the translator follows the display list
// in execution order, flattens G_DL calls/branches, resolves segmented and KSEG0 addresses itself and
// stages vertices, matrices, lights, viewports and texture data into buffers the interpreter can read.
namespace bm64lus {
    struct HostGfx {
        uintptr_t w0;
        uintptr_t w1;
    };

    class DisplayListTranslator {
    public:
        explicit DisplayListTranslator(uint8_t* rdram);

        // Discards the commands and per-frame data of the previous frame.
        void begin_frame();
        // Appends the commands of one graphics task, starting at the given RDRAM address.
        void translate_task(uint32_t dl_address);
        // Returns the accumulated commands for this frame, terminated with G_ENDDL.
        HostGfx* finish_frame();
        bool has_commands() const { return !cmds.empty(); }

        // Drops all staged textures. The caller must also clear the interpreter's texture cache.
        void clear_texture_store();
        size_t texture_store_bytes() const { return texture_bytes; }

        // Called with the staged pointer of every texture copy that gets evicted from the store.
        std::function<void(const uint8_t*)> on_texture_evicted;

        // Creates the libultraship framebuffer that receives copies of the frame while the game samples its own
        // framebuffers (the motion blur effect). Unset disables framebuffer emulation. It's only called once the game
        // first samples a framebuffer: the Switch's Mesa crashes when the interpreter resizes a framebuffer that was
        // created before the first frame and never drawn to.
        std::function<int()> create_previous_frame_fb;

        // Running totals for performance measurements.
        struct Stats {
            uint32_t textures_staged = 0;
            uint32_t textures_new = 0;
            uint64_t texture_bytes_hashed = 0;
        } stats;

        // Debugging aid: when non-zero, every Nth frame's raw display list is written to dl_<frame>.txt
        // (set from the BM64_DL_DUMP environment variable).
        uint32_t dump_interval = 0;
        // Debugging aid: when set, every new texture is written to texdump/ as a grayscale image
        // (BM64_TEX_DUMP=1; the folder must exist).
        bool texture_dump = false;

    private:
        struct PendingTextureImage {
            bool active = false;
            size_t cmd_index = 0;
            uint32_t phys = 0;
            uint32_t fmt = 0;
            uint32_t siz = 0;
            uint32_t width = 0;
            uint32_t needed_bytes = 0;
        };

        struct StagedTexture {
            std::unique_ptr<uint8_t[]> data;
            uint32_t size = 0;
            uint32_t last_used_frame = 0;
        };

        uint8_t* rdram;
        std::vector<HostGfx> cmds;

        // Per-frame bump allocator for converted vertices, matrices, lights and viewports.
        std::vector<std::unique_ptr<uint8_t[]>> arena_chunks;
        size_t arena_chunk_index = 0;
        size_t arena_offset = 0;

        // Content-addressed texture copies, so the interpreter's pointer-keyed texture cache
        // sees a new pointer whenever the bytes at an address change.
        std::unordered_map<uint64_t, StagedTexture> texture_store;
        size_t texture_bytes = 0;
        uint32_t frame_counter = 0;

        // Color images the game has rendered to, keyed by physical address, with their width in pixels.
        // The game never sees the RDP write them, so reads from them are redirected to GPU framebuffers.
        std::unordered_map<uint32_t, uint32_t> color_images;
        // Set while the current texture image is one of the game's framebuffers.
        bool timg_is_framebuffer = false;
        uint32_t timg_fb_row = 0;
        uint32_t timg_fb_width = 0;
        uint32_t last_framebuffer_copy_frame = 0;

        FILE* dump_file = nullptr;
        int previous_frame_fb = -1;

        // RDP state needed to emulate blender modes the interpreter doesn't support.
        uint32_t othermode_h = 0;
        uint32_t othermode_l = 0;
        uint32_t fog_color = 0;
        uint32_t prim_color_w0 = 0xFA000000;
        uint32_t prim_color_w1 = 0;
        uint32_t combine_w0 = 0xFC000000;
        uint32_t combine_w1 = 0;

        uint32_t segments[16] = {};
        PendingTextureImage pending_timg;
        uint32_t last_rdphalf_1 = 0;
        bool extended_gbi_enabled = false;
        uint8_t extended_opcode = 0;

        void* arena_alloc(size_t size);
        bool resolve(uint32_t address, uint32_t size, uint32_t& phys_out) const;
        uintptr_t host_ptr(uint32_t phys) const { return reinterpret_cast<uintptr_t>(rdram + phys); }

        uint32_t read32(uint32_t phys) const;
        uint16_t read16(uint32_t phys) const;
        uint8_t read8(uint32_t phys) const;

        void emit(uint32_t w0, uintptr_t w1) { cmds.push_back({ w0, w1 }); }

        uintptr_t stage_vertices(uint32_t phys, uint32_t count);
        uintptr_t stage_matrix(uint32_t phys);
        uintptr_t stage_bytes(uint32_t phys, uint32_t size);
        uintptr_t stage_viewport(uint32_t phys);
        const uint8_t* stage_texture(uint32_t phys, uint32_t size);

        void finalize_texture_image();
        bool find_color_image(uint32_t phys, uint32_t& base_out, uint32_t& width_out) const;
        // Called when the game stops sampling its framebuffer; saves the composited result for the next frame.
        void end_framebuffer_reads();
        // Replaces a texture rectangle that samples a framebuffer with a rectangle over the previous frame's copy.
        void emit_framebuffer_rect(uint32_t w0, uint32_t w1, uint32_t st, uint32_t dsdt);
        void note_texture_load(uint32_t w0, uint32_t w1);
        // True when the blender takes its alpha from the fog color, which the interpreter ignores.
        bool blends_with_fog_alpha() const;
        // Emits a draw command (plus the given follow-up words), emulating fog-alpha blending around it if needed.
        void emit_draw(uint32_t w0, uintptr_t w1, const uint32_t* extra_words = nullptr, size_t extra_count = 0);
        // Returns the number of extra 64-bit words an extended (RT64) command occupies.
        uint32_t translate_extended(uint32_t w0, uint32_t w1, uint32_t pc);
        void evict_old_textures();
        void dump_texture(const uint8_t* data, uint32_t size);
        uint32_t texture_dump_count = 0;
    };
}

#endif
