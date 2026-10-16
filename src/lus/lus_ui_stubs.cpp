// Replacements for the RmlUi menu entry points that the rest of the game calls. This port has no
// RmlUi menus; its settings live in the ImGui menu instead (see lus_menu.cpp).

#include <cstdio>

#include "recomp_ui.h"
#include "zelda_config.h"
#include "zelda_render.h"
#include "ultramodern/ultramodern.hpp"
#include "recomp_input.h"
#include "lus_menu.h"
#ifdef __SWITCH__
#include <ship/port/switch/SwitchImpl.h>
#endif

namespace recompui {
    // The game forwards the events the RmlUi menus would handle here; libultraship's ImGui menu reads its input
    // directly (it opens with Esc or the controller's Back/Minus button).
    void queue_event(const SDL_Event&) {}

    bool try_deque_event(SDL_Event&) {
        return false;
    }

    bool is_context_capturing_input() {
        return bm64lus::is_menu_open();
    }

    bool is_context_capturing_mouse() {
        return false;
    }

    bool is_any_context_shown() {
        return false;
    }

    void process_game_started() {}

    void get_window_size(int& width, int& height) {
        SDL_Window* window = SDL_GL_GetCurrentWindow();
        if (window != nullptr) {
            SDL_GetWindowSize(window, &width, &height);
        } else {
            width = 1280;
            height = 720;
        }
    }

    void update_supported_options() {}

    void toggle_fullscreen() {}

    void activate_mouse() {}

    void message_box(const char* msg) {
        fprintf(stderr, "%s\n", msg);
#ifdef __SWITCH__
        // SDL has no message box on the Switch; the system error applet shows the text instead.
        Ship::Switch::ShowErrorApplet("%s", msg);
#else
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bomberman 64: Recompiled", msg, nullptr);
#endif
    }

    void drop_files(const std::list<std::filesystem::path>&) {}

    void register_ui_exports() {}
}

namespace zelda64 {
    void open_quit_game_prompt() {
        ultramodern::quit();
    }
}
