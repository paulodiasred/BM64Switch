// ImGui settings menu for the libultraship build, replacing the RmlUi config menu.

#include "lus_menu.h"

#include <array>
#include <cstdio>
#include <atomic>
#include <string>

#include <imgui.h>
#include <ship/Context.h>
#include <ship/config/ConsoleVariable.h>
#include <ship/window/gui/GuiWindow.h>
#include <fast/Fast3dWindow.h>
#ifdef __SWITCH__
#include <ship/port/switch/SwitchImpl.h>
#endif

#include "ultramodern/config.hpp"
#include "recomp_input.h"
#include "zelda_config.h"
#include "zelda_sound.h"
#include "zelda_cheats.h"

namespace {
    constexpr const char* cvar_menu_open = "gOpenWindows.BM64Settings";
    constexpr const char* cvar_internal_resolution = "gInternalResolution";
    constexpr const char* cvar_msaa = "gMSAAValue";
    constexpr const char* cvar_texture_filter = "gTextureFilter";
    constexpr const char* cvar_vsync = "gVsyncEnabled";
    constexpr const char* cvar_switch_perf_mode = "gSwitchPerfMode";
    constexpr const char* cvar_perf_overlay = "gBM64PerfOverlay";

    std::atomic_bool menu_open = false;

    Ship::ConsoleVariable* cvars() {
        return Ship::Context::GetInstance()->GetConsoleVariables().get();
    }

    std::shared_ptr<Fast::Fast3dWindow> fast_window() {
        return std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
    }

    bool on_off_checkbox(const char* label, zelda64::GenericOnOffOption (*getter)(), void (*setter)(zelda64::GenericOnOffOption)) {
        bool value = getter() == zelda64::GenericOnOffOption::On;
        if (ImGui::Checkbox(label, &value)) {
            setter(value ? zelda64::GenericOnOffOption::On : zelda64::GenericOnOffOption::Off);
            return true;
        }
        return false;
    }

    bool volume_slider(const char* label, int (*getter)(), void (*setter)(int)) {
        int value = getter();
        if (ImGui::SliderInt(label, &value, 0, 100, "%d%%")) {
            setter(value);
            return true;
        }
        return false;
    }

    class SettingsWindow final : public Ship::GuiWindow {
    public:
        SettingsWindow()
            : Ship::GuiWindow(cvar_menu_open, false, "Bomberman 64 Settings", ImVec2(640, 480),
                              ImGuiWindowFlags_NoCollapse) {}

    protected:
        void InitElement() override {}

        void UpdateElement() override {
            menu_open = IsVisible();
            // Let the menu be driven by a controller while it's open, without ImGui reacting to the game's input otherwise.
            ImGuiIO& io = ImGui::GetIO();
            if (IsVisible()) {
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard;
            } else {
                io.ConfigFlags &= ~(ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard);
            }
            poll_rebinding();
        }

        void DrawElement() override {
            ImGui::TextDisabled("Press Minus (-) or Esc to close. Settings are saved on close.");
            if (ImGui::BeginTabBar("SettingsTabs")) {
                if (ImGui::BeginTabItem("Graphics")) {
                    draw_graphics();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Sound")) {
                    draw_sound();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Controls")) {
                    draw_controls();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Cheats")) {
                    draw_cheats();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }

        void SetVisibility(bool visible) override {
            bool was_visible = IsVisible();
            Ship::GuiWindow::SetVisibility(visible);
            menu_open = visible;
            if (was_visible && !visible) {
                if (rebinding_input >= 0) {
                    recomp::cancel_scanning_input();
                    rebinding_input = -1;
                }
                zelda64::save_config();
                cvars()->Save();
            }
        }

    private:
        int rebinding_input = -1;
        size_t rebinding_slot = 0;

        void draw_graphics() {
            float resolution = cvars()->GetFloat(cvar_internal_resolution, 1.0f);
            if (ImGui::SliderFloat("Internal resolution", &resolution, 0.5f, 2.0f, "%.2fx")) {
                cvars()->SetFloat(cvar_internal_resolution, resolution);
            }

            static const std::array<int, 3> msaa_values = { 1, 2, 4 };
            static const char* msaa_names[] = { "Off", "2x", "4x" };
            int msaa = cvars()->GetInteger(cvar_msaa, 1);
            int msaa_index = msaa >= 4 ? 2 : (msaa >= 2 ? 1 : 0);
            if (ImGui::Combo("Anti-aliasing (MSAA)", &msaa_index, msaa_names, IM_ARRAYSIZE(msaa_names))) {
                cvars()->SetInteger(cvar_msaa, msaa_values[msaa_index]);
            }

            static const char* filter_names[] = { "Three-point (N64)", "Bilinear", "Nearest" };
            int filter = cvars()->GetInteger(cvar_texture_filter, Fast::FILTER_THREE_POINT);
            if (ImGui::Combo("Texture filtering", &filter, filter_names, IM_ARRAYSIZE(filter_names))) {
                cvars()->SetInteger(cvar_texture_filter, filter);
                if (auto window = fast_window()) {
                    window->SetTextureFilter(static_cast<Fast::FilteringMode>(filter));
                }
            }

            bool vsync = cvars()->GetInteger(cvar_vsync, 1) != 0;
            if (ImGui::Checkbox("VSync", &vsync)) {
                cvars()->SetInteger(cvar_vsync, vsync ? 1 : 0);
            }

            ultramodern::renderer::GraphicsConfig config = ultramodern::renderer::get_graphics_config();
            static const char* rate_names[] = { "Original (60 FPS)", "Display", "Manual" };
            int rate = static_cast<int>(config.rr_option);
            bool config_changed = ImGui::Combo("Framerate", &rate, rate_names, IM_ARRAYSIZE(rate_names));
            if (config_changed) {
                config.rr_option = static_cast<ultramodern::renderer::RefreshRate>(rate);
            }
            if (config.rr_option == ultramodern::renderer::RefreshRate::Manual) {
                config_changed |= ImGui::SliderInt("Target FPS", &config.rr_manual_value, 30, 120);
            }
            if (config_changed) {
                ultramodern::renderer::set_graphics_config(config);
            }

#ifdef __SWITCH__
            static const char* profile_names[] = { "Maximum (1785 MHz)", "High (1581 MHz)", "Boost (1224 MHz)",
                                                   "Stock (1020 MHz)", "Power saving (918 MHz)",
                                                   "Power saving (816 MHz)", "Power saving (714 MHz)" };
            int profile = cvars()->GetInteger(cvar_switch_perf_mode, Ship::MAXIMUM);
            if (ImGui::Combo("CPU clock", &profile, profile_names, IM_ARRAYSIZE(profile_names))) {
                cvars()->SetInteger(cvar_switch_perf_mode, profile);
                Ship::Switch::ApplyOverclock();
            }
#endif
            bool overlay = bm64lus::perf_overlay_enabled();
            if (ImGui::Checkbox("Performance overlay", &overlay)) {
                cvars()->SetInteger(cvar_perf_overlay, overlay ? 1 : 0);
            }
        }

        void draw_sound() {
            volume_slider("Main volume", zelda64::get_main_volume, zelda64::set_main_volume);
            volume_slider("Music volume", zelda64::get_bgm_volume, zelda64::set_bgm_volume);
            volume_slider("Sound effects volume", zelda64::get_sfx_volume, zelda64::set_sfx_volume);
            volume_slider("Voice volume", zelda64::get_voice_volume, zelda64::set_voice_volume);
            if (ImGui::Button("Reset sound settings")) {
                zelda64::reset_sound_settings();
            }
        }

        void draw_controls() {
            int rumble = recomp::get_rumble_strength();
            if (ImGui::SliderInt("Rumble strength", &rumble, 0, 100, "%d%%")) {
                recomp::set_rumble_strength(rumble);
            }
            int deadzone = recomp::get_joystick_deadzone();
            if (ImGui::SliderInt("Stick deadzone", &deadzone, 0, 100, "%d%%")) {
                recomp::set_joystick_deadzone(deadzone);
            }
            int gyro = recomp::get_gyro_sensitivity();
            if (ImGui::SliderInt("Gyro sensitivity", &gyro, 0, 100, "%d%%")) {
                recomp::set_gyro_sensitivity(gyro);
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Controller bindings (select a slot, then press the button to assign)");
            if (ImGui::BeginTable("Bindings", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
                ImGui::TableSetupColumn("Action");
                ImGui::TableSetupColumn("Binding 1");
                ImGui::TableSetupColumn("Binding 2");
                ImGui::TableHeadersRow();
                for (size_t input_index = 0; input_index < recomp::get_num_inputs(); input_index++) {
                    auto input = static_cast<recomp::GameInput>(input_index);
                    // Menu accept/apply only matter for the RmlUi menus.
                    if (input == recomp::GameInput::ACCEPT_MENU || input == recomp::GameInput::APPLY_MENU) {
                        continue;
                    }
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(recomp::get_input_name(input).c_str());
                    for (size_t slot = 0; slot < recomp::bindings_per_input; slot++) {
                        ImGui::TableNextColumn();
                        bool waiting = rebinding_input == static_cast<int>(input_index) && rebinding_slot == slot;
                        std::string label = waiting
                            ? std::string("Press a button...")
                            : recomp::get_input_binding(input, slot, recomp::InputDevice::Controller).to_string();
                        label += "##" + std::to_string(input_index) + "_" + std::to_string(slot);
                        if (ImGui::Button(label.c_str(), ImVec2(-FLT_MIN, 0)) && rebinding_input < 0) {
                            rebinding_input = static_cast<int>(input_index);
                            rebinding_slot = slot;
                            recomp::start_scanning_input(recomp::InputDevice::Controller);
                        }
                    }
                }
                ImGui::EndTable();
            }
            if (ImGui::Button("Reset controller bindings")) {
                zelda64::reset_cont_input_bindings();
            }
        }

        void draw_cheats() {
            on_off_checkbox("Moon jump", zelda64::get_moonjump, zelda64::set_moonjump);
            on_off_checkbox("Always max bombs", zelda64::get_always_max_bomb, zelda64::set_always_max_bomb);
            on_off_checkbox("Always max fire", zelda64::get_always_max_fire, zelda64::set_always_max_fire);
            on_off_checkbox("Always have remote bombs", zelda64::get_always_have_remote_bombs, zelda64::set_always_have_remote_bombs);
            on_off_checkbox("Always have red bombs", zelda64::get_always_have_red_bombs, zelda64::set_always_have_red_bombs);
            on_off_checkbox("Infinite credits", zelda64::get_infinite_credits, zelda64::set_infinite_credits);
            on_off_checkbox("Infinite lives", zelda64::get_infinite_lives, zelda64::set_infinite_lives);
        }

        // The game's input code stores the next pressed button while scanning; assign it once it arrives.
        void poll_rebinding() {
            if (rebinding_input < 0) {
                return;
            }
            recomp::InputField scanned = recomp::get_scanned_input();
            if (scanned.input_type != 0) {
                recomp::set_input_binding(static_cast<recomp::GameInput>(rebinding_input), rebinding_slot,
                                          recomp::InputDevice::Controller, scanned);
                rebinding_input = -1;
            }
        }
    };

    std::shared_ptr<SettingsWindow> settings_window;
}

namespace bm64lus {
    std::vector<std::shared_ptr<Ship::GuiWindow>> create_gui_windows() {
        return {};
    }

    void setup_menu() {
        // Registered as libultraship's menu, which it toggles with Esc and with the controller's Back (Minus) button
        // when controller navigation is on.
        settings_window = std::make_shared<SettingsWindow>();
        settings_window->Init();
        cvars()->SetInteger("gControlNav", 1);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SetMenu(settings_window);
        // The menu always starts closed, even if it was open when the game was last closed.
        settings_window->Hide();
#ifdef __SWITCH__
        Ship::Switch::ApplyOverclock();
#endif
    }

    bool is_menu_open() {
        return menu_open;
    }

    bool perf_overlay_enabled() {
        return cvars()->GetInteger(cvar_perf_overlay, 0) != 0;
    }

    void draw_perf_overlay(const char* text) {
        ImGui::SetNextWindowPos(ImVec2(8.0f, 8.0f));
        ImGui::SetNextWindowBgAlpha(0.6f);
        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                           ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing |
                                           ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin("##bm64perf", nullptr, flags)) {
            ImGui::TextUnformatted(text);
        }
        ImGui::End();
    }
}
