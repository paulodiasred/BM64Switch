#ifndef __RECOMP_UI__
#define __RECOMP_UI__

#include <filesystem>
#include <list>

#include "SDL2/SDL.h"

// UI entry points the game code calls. This port has no RmlUi menus: the settings menu is libultraship's ImGui
// menu, and these are implemented in src/lus/lus_ui_stubs.cpp.
namespace recompui {
    void queue_event(const SDL_Event& event);
    bool try_deque_event(SDL_Event& out);

    bool is_context_capturing_input();
    bool is_context_capturing_mouse();
    bool is_any_context_shown();

    void process_game_started();
    void get_window_size(int& width, int& height);
    void set_cursor_visible(bool visible);
    void update_supported_options();
    void toggle_fullscreen();
    void activate_mouse();

    void message_box(const char* msg);
    void drop_files(const std::list<std::filesystem::path>& file_list);
    void register_ui_exports();
}

#endif
