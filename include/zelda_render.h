#ifndef __ZELDA_RENDER_H__
#define __ZELDA_RENDER_H__

#include <memory>

#include "ultramodern/renderer_context.hpp"

namespace zelda64 {
    namespace renderer {
        // libultraship (Fast3D) renderer, implemented in src/lus/lus_render_context.cpp.
        std::unique_ptr<ultramodern::renderer::RendererContext> create_render_context(uint8_t *rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode);

        // Queried by the patches; the libultraship renderer has no high precision framebuffers.
        bool RT64HighPrecisionFBEnabled();
    }
}

#endif
