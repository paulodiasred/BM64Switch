// Storage for the settings that the RmlUi config menu (src/ui/ui_config.cpp) owns in the RT64 build.
// config.cpp loads and saves them through these accessors; the ImGui menu edits them.

#include <algorithm>
#include <atomic>

#include <SDL2/SDL.h>

#include "recomp_input.h"
#include "zelda_config.h"
#include "zelda_sound.h"
#include "zelda_render.h"
#include "zelda_debug.h"
#include "zelda_cheats.h"

namespace {
    struct ControlOptions {
        int rumble_strength = 50;
        int gyro_sensitivity = 50;
        int mouse_sensitivity = 0;
        int joystick_deadzone = 5;
        zelda64::TargetingMode targeting_mode{};
        recomp::BackgroundInputMode background_input_mode{};
        zelda64::FilmGrainMode film_grain_mode{};
        zelda64::RadioBoxMode radio_comm_box_mode{};
        zelda64::AimInvertMode invert_y_axis_mode{};
        zelda64::AimInvertMode analog_camera_invert_mode{};
    };

    struct SoundOptions {
        std::atomic<int> main_volume = 100;
        std::atomic<int> bgm_volume = 100;
        std::atomic<int> sfx_volume = 100;
        std::atomic<int> voice_volume = 100;
        std::atomic<bool> low_health_beeps_enabled = true;
    };

    struct CheatOptions {
        zelda64::GenericOnOffOption moonjump = zelda64::GenericOnOffOption::Off;
        zelda64::GenericOnOffOption always_max_bomb = zelda64::GenericOnOffOption::Off;
        zelda64::GenericOnOffOption always_max_fire = zelda64::GenericOnOffOption::Off;
        zelda64::GenericOnOffOption always_have_remote_bombs = zelda64::GenericOnOffOption::Off;
        zelda64::GenericOnOffOption always_have_red_bombs = zelda64::GenericOnOffOption::Off;
        zelda64::GenericOnOffOption infinite_credits = zelda64::GenericOnOffOption::Off;
        zelda64::GenericOnOffOption infinite_lives = zelda64::GenericOnOffOption::Off;
    };

    ControlOptions control_options;
    SoundOptions sound_options;
    CheatOptions cheat_options;
    bool debug_mode_enabled = false;
}

// Input rebinding happens in the ImGui menu, which doesn't use the RmlUi scanning flow.
int recomp::get_scanned_input_index() {
    return -1;
}

void recomp::cancel_scanning_input() {
    recomp::stop_scanning_input();
}

int recomp::get_rumble_strength() { return control_options.rumble_strength; }
void recomp::set_rumble_strength(int strength) { control_options.rumble_strength = strength; }
int recomp::get_gyro_sensitivity() { return control_options.gyro_sensitivity; }
void recomp::set_gyro_sensitivity(int sensitivity) { control_options.gyro_sensitivity = sensitivity; }
int recomp::get_mouse_sensitivity() { return control_options.mouse_sensitivity; }
void recomp::set_mouse_sensitivity(int sensitivity) { control_options.mouse_sensitivity = sensitivity; }
int recomp::get_joystick_deadzone() { return control_options.joystick_deadzone; }
void recomp::set_joystick_deadzone(int deadzone) { control_options.joystick_deadzone = deadzone; }

recomp::BackgroundInputMode recomp::get_background_input_mode() { return control_options.background_input_mode; }
void recomp::set_background_input_mode(recomp::BackgroundInputMode mode) {
    control_options.background_input_mode = mode;
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, mode == recomp::BackgroundInputMode::On ? "1" : "0");
}

zelda64::TargetingMode zelda64::get_targeting_mode() { return control_options.targeting_mode; }
void zelda64::set_targeting_mode(zelda64::TargetingMode mode) { control_options.targeting_mode = mode; }
zelda64::FilmGrainMode zelda64::get_film_grain_mode() { return control_options.film_grain_mode; }
void zelda64::set_film_grain_mode(zelda64::FilmGrainMode mode) { control_options.film_grain_mode = mode; }
zelda64::RadioBoxMode zelda64::get_radio_comm_box_mode() { return control_options.radio_comm_box_mode; }
void zelda64::set_radio_comm_box_mode(zelda64::RadioBoxMode mode) { control_options.radio_comm_box_mode = mode; }
zelda64::AimInvertMode zelda64::get_invert_y_axis_mode() { return control_options.invert_y_axis_mode; }
void zelda64::set_invert_y_axis_mode(zelda64::AimInvertMode mode) { control_options.invert_y_axis_mode = mode; }
zelda64::AimInvertMode zelda64::get_analog_camera_invert_mode() { return control_options.analog_camera_invert_mode; }
void zelda64::set_analog_camera_invert_mode(zelda64::AimInvertMode mode) { control_options.analog_camera_invert_mode = mode; }

void zelda64::reset_sound_settings() {
    sound_options.main_volume = 100;
    sound_options.bgm_volume = 100;
    sound_options.sfx_volume = 100;
    sound_options.voice_volume = 100;
    sound_options.low_health_beeps_enabled = true;
}

int zelda64::get_main_volume() { return sound_options.main_volume.load(); }
void zelda64::set_main_volume(int volume) { sound_options.main_volume = std::clamp(volume, 0, 100); }
int zelda64::get_bgm_volume() { return sound_options.bgm_volume.load(); }
void zelda64::set_bgm_volume(int volume) { sound_options.bgm_volume = std::clamp(volume, 0, 100); }
int zelda64::get_sfx_volume() { return sound_options.sfx_volume.load(); }
void zelda64::set_sfx_volume(int volume) { sound_options.sfx_volume = std::clamp(volume, 0, 100); }
int zelda64::get_voice_volume() { return sound_options.voice_volume.load(); }
void zelda64::set_voice_volume(int volume) { sound_options.voice_volume = std::clamp(volume, 0, 100); }
bool zelda64::get_low_health_beeps_enabled() { return sound_options.low_health_beeps_enabled.load(); }
void zelda64::set_low_health_beeps_enabled(bool enabled) { sound_options.low_health_beeps_enabled = enabled; }

void zelda64::reset_cheats_settings() {
    cheat_options = {};
}

zelda64::GenericOnOffOption zelda64::get_moonjump() { return cheat_options.moonjump; }
void zelda64::set_moonjump(zelda64::GenericOnOffOption mode) { cheat_options.moonjump = mode; }
zelda64::GenericOnOffOption zelda64::get_always_max_bomb() { return cheat_options.always_max_bomb; }
void zelda64::set_always_max_bomb(zelda64::GenericOnOffOption mode) { cheat_options.always_max_bomb = mode; }
zelda64::GenericOnOffOption zelda64::get_always_max_fire() { return cheat_options.always_max_fire; }
void zelda64::set_always_max_fire(zelda64::GenericOnOffOption mode) { cheat_options.always_max_fire = mode; }
zelda64::GenericOnOffOption zelda64::get_always_have_remote_bombs() { return cheat_options.always_have_remote_bombs; }
void zelda64::set_always_have_remote_bombs(zelda64::GenericOnOffOption mode) { cheat_options.always_have_remote_bombs = mode; }
zelda64::GenericOnOffOption zelda64::get_always_have_red_bombs() { return cheat_options.always_have_red_bombs; }
void zelda64::set_always_have_red_bombs(zelda64::GenericOnOffOption mode) { cheat_options.always_have_red_bombs = mode; }
zelda64::GenericOnOffOption zelda64::get_infinite_credits() { return cheat_options.infinite_credits; }
void zelda64::set_infinite_credits(zelda64::GenericOnOffOption mode) { cheat_options.infinite_credits = mode; }
zelda64::GenericOnOffOption zelda64::get_infinite_lives() { return cheat_options.infinite_lives; }
void zelda64::set_infinite_lives(zelda64::GenericOnOffOption mode) { cheat_options.infinite_lives = mode; }

bool zelda64::get_debug_mode_enabled() { return debug_mode_enabled; }
void zelda64::set_debug_mode_enabled(bool enabled) { debug_mode_enabled = enabled; }

// RT64-only graphics capabilities queried by the config code.
bool zelda64::renderer::RT64HighPrecisionFBEnabled() { return false; }
