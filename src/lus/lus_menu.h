#ifndef __BM64_LUS_MENU_H__
#define __BM64_LUS_MENU_H__

#include <memory>
#include <vector>

namespace Ship {
    class GuiWindow;
    class ControlDeck;
}

namespace bm64lus {
    // The game reads controllers through SDL directly, but libultraship requires a control deck instance.
    std::shared_ptr<Ship::ControlDeck> create_control_deck();

    // GUI windows handed to the Fast3D window on creation.
    std::vector<std::shared_ptr<Ship::GuiWindow>> create_gui_windows();
    // Applies the saved libultraship settings once the window exists.
    void setup_menu();

    // The settings menu replaces the RmlUi config menu. It opens with Esc or the controller's Back (Minus) button
    // and blocks game input while it's open.
    bool is_menu_open();

    // On-screen frame timing readout, toggled in the Graphics tab. draw_perf_overlay must be called during the
    // ImGui frame (between the GUI's StartDraw and EndDraw).
    bool perf_overlay_enabled();
    void draw_perf_overlay(const char* text);
}

#endif
