// Renderer backend that replaces RT64 with libultraship's Fast3D interpreter (OpenGL), so the game can run on
// platforms without Vulkan/D3D12/Metal such as the Nintendo Switch.

#include <cstdio>
#include <filesystem>
#include <cstdlib>
#include <memory>
#include <unordered_map>

#include <SDL2/SDL.h>

#include <ship/Context.h>
#include <ship/config/ConsoleVariable.h>
#include <ship/window/Window.h>
#include <fast/Fast3dWindow.h>
#include <fast/interpreter.h>
#include <ship/window/MouseStateManager.h>
#include <ship/window/gui/Gui.h>
#ifdef __SWITCH__
#include <ship/port/switch/SwitchImpl.h>
#include <switch.h>
#endif

#include "ultramodern/ultramodern.hpp"
#include "ultramodern/renderer_context.hpp"
#include "zelda_render.h"
#include "dl_translator.h"
#include "lus_menu.h"
#include "perf_monitor.h"

bool sdl_event_filter(void* userdata, SDL_Event* event);
// Defined in main.cpp.
void autostart_game();

namespace {
    // Number of VIs a finished frame may wait for a framebuffer swap before it's presented anyway.
    constexpr uint32_t max_unpresented_vis = 3;
    // Number of graphics tasks accumulated for one presented frame before older ones are discarded.
    constexpr uint32_t max_tasks_per_frame = 4;

    int forward_sdl_event(void* userdata, SDL_Event* event) {
        sdl_event_filter(userdata, event);
        return 0;
    }

    class LUSContext final : public ultramodern::renderer::RendererContext {
    public:
        LUSContext(uint8_t* rdram, bool developer_mode) : rdram(rdram), translator(rdram) {
            if (const char* dump_frame = getenv("BM64_RDRAM_DUMP")) {
                rdram_dump_frame = static_cast<uint32_t>(atoi(dump_frame));
            }
            (void)developer_mode;
            // The ultramodern API enum has no OpenGL entry; Auto stands for "whatever libultraship picked".
            chosen_api = ultramodern::renderer::GraphicsApi::Auto;

            printf("[LUS] creating context\n");
            // libultraship names its log file after the app, and ':' isn't allowed in file names on the Switch.
            context = Ship::Context::CreateUninitializedInstance("Bomberman 64 Recompiled", "bm64", "bm64.cfg.json");
            // libultraship's Switch pre-init phase is skipped on purpose: it quits when launched in applet mode (from
            // the album), which this game fits in. The post-init phase runs inside InitResourceManager.
#ifdef __SWITCH__
            // The archive is embedded in the .nro's RomFS, but libultraship makes archive paths absolute with
            // std::filesystem, which doesn't understand device prefixes like "romfs:" (it would prepend the working
            // directory). Copy it next to the game's other files and use a path relative to the working directory.
            // (std::filesystem::copy_file relies on syscalls libnx doesn't implement, so copy with stdio.)
            std::string archive_path = "bm64.o2r";
            size_t copied_bytes = 0;
            if (FILE* in = fopen("romfs:/bm64.o2r", "rb")) {
                if (FILE* out = fopen(archive_path.c_str(), "wb")) {
                    char buffer[16 * 1024];
                    size_t count;
                    while ((count = fread(buffer, 1, sizeof(buffer), in)) > 0) {
                        copied_bytes += fwrite(buffer, 1, count, out);
                    }
                    fclose(out);
                }
                fclose(in);
            }
            printf("[LUS] archive copied from romfs: %zu bytes\n", copied_bytes);
#else
            std::string archive_path = Ship::Context::LocateFileAcrossAppDirs("bm64.o2r");
#endif
            // libultraship's own log file is optional; without it its messages go to spdlog's default logger.
            if (!context->InitLogging()) {
                printf("[LUS] libultraship file logging unavailable, continuing\n");
            }
            const char* failed_step = nullptr;
            if (!context->InitConfiguration()) {
                failed_step = "configuration";
            } else if (!context->InitConsoleVariables()) {
                failed_step = "console variables";
            } else if (!context->InitResourceManager({ archive_path }, {}, 1, true)) {
                failed_step = "resource manager";
            } else if (!context->InitControlDeck(bm64lus::create_control_deck())) {
                failed_step = "control deck";
            } else if (!context->InitConsole()) {
                failed_step = "console";
            }
            if (failed_step != nullptr) {
                printf("[LUS] Failed to initialize libultraship: %s\n", failed_step);
                fflush(stdout);
                setup_result = ultramodern::renderer::SetupResult::GraphicsDeviceNotFound;
                return;
            }
            printf("[LUS] libultraship initialized, archive loaded: %s\n",
                    context->GetResourceManager()->IsLoaded() ? "yes" : "no");

            // Keep ImGui windows inside the game window on every platform (the Switch has no OS windows for them).
            context->GetConsoleVariables()->SetInteger("gEnableMultiViewports", 0);
            printf("[LUS] creating window\n");
            window = std::make_shared<Fast::Fast3dWindow>(bm64lus::create_gui_windows());
            bool window_ok = context->InitWindow(window);
            if (!window_ok || !context->InitGfxDebugger() || !context->InitEventSystem()) {
                printf("[LUS] Failed to create the window (%s)\n", window_ok ? "debugger/events" : "window");
                fflush(stdout);
                setup_result = ultramodern::renderer::SetupResult::GraphicsDeviceNotFound;
                return;
            }
            printf("[LUS] window ready\n");
            window->SetRendererUCode(ucode_f3dex);
            // Compiling a shader the first time an effect appears stalls a frame (very noticeably on the Switch), so
            // keep a list of the shaders the game has used and compile them all up front on later runs.
            if (auto interpreter = window->GetInterpreterWeak().lock()) {
                interpreter->EnablePersistentShaderList("shaders.bin");
            }
            window->SetTargetFps(60);
            bm64lus::setup_menu();

            translator.on_texture_evicted = [this](const uint8_t* ptr) {
                if (auto interpreter = window->GetInterpreterWeak().lock()) {
                    interpreter->TextureCacheDelete(ptr);
                }
            };

            if (const char* dump = getenv("BM64_TEX_DUMP")) {
                translator.texture_dump = dump[0] == '1';
            }
            if (const char* dump = getenv("BM64_DL_DUMP")) {
                translator.dump_interval = static_cast<uint32_t>(atoi(dump));
            }

            // Target for the copies of finished frames that the motion blur effect samples. It follows the window size.
            bool framebuffer_emulation = true;
#ifdef __SWITCH__
            // In applet mode (launched from the album) the GPU shares a small memory pool, and Mesa crashes instead of
            // failing cleanly when an extra framebuffer doesn't fit. Skip the motion blur effect's framebuffer there.
            AppletType applet_type = appletGetAppletType();
            framebuffer_emulation = applet_type == AppletType_Application || applet_type == AppletType_SystemApplication;
            u64 total_memory = 0, used_memory = 0;
            svcGetInfo(&total_memory, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
            svcGetInfo(&used_memory, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
            printf("[LUS] memory: %llu MB used of %llu MB, motion blur %s\n", (unsigned long long)(used_memory >> 20),
                   (unsigned long long)(total_memory >> 20), framebuffer_emulation ? "on" : "off (applet mode)");
#endif
            if (framebuffer_emulation) {
                translator.create_previous_frame_fb = [this]() {
                    auto interpreter = window->GetInterpreterWeak().lock();
                    return interpreter ? interpreter->CreateFrameBuffer(window->GetWidth(), window->GetHeight(), 320, 240, true) : -1;
                };
            }

            // The game's input code expects to see the SDL events, which libultraship now pumps on this thread.
            SDL_AddEventWatch(forward_sdl_event, nullptr);

            // An event watch only sees events pushed after it's added, and controllers that were already connected
            // (such as attached Joy-Cons) were announced during SDL_Init. Announce them to the game again.
            for (int i = 0; i < SDL_NumJoysticks(); i++) {
                if (SDL_IsGameController(i)) {
                    SDL_Event added{};
                    added.type = SDL_CONTROLLERDEVICEADDED;
                    added.cdevice.which = i;
                    sdl_event_filter(nullptr, &added);
                }
            }

            printf("[LUS] renderer ready\n");
            fflush(stdout);
            setup_result = ultramodern::renderer::SetupResult::Success;
            initialized = true;
        }

        ~LUSContext() override {
            if (!initialized) {
                // Tearing down a half-initialized libultraship crashes. The game is about to quit with an error
                // message anyway, so leave it alive.
                new std::shared_ptr<Ship::Context>(std::move(context));
                new std::shared_ptr<Fast::Fast3dWindow>(std::move(window));
            }
        }

        bool valid() override {
            return initialized;
        }

        bool update_config(const ultramodern::renderer::GraphicsConfig& old_config,
                           const ultramodern::renderer::GraphicsConfig& new_config) override {
            (void)old_config;
            switch (new_config.rr_option) {
                case ultramodern::renderer::RefreshRate::Manual:
                    window->SetTargetFps(new_config.rr_manual_value);
                    break;
                case ultramodern::renderer::RefreshRate::Display:
                    window->SetTargetFps(static_cast<int32_t>(get_display_framerate()));
                    break;
                default:
                    window->SetTargetFps(60);
                    break;
            }
            return true;
        }

        void enable_instant_present() override {}

        void send_dl(const OSTask* task) override {
            if (frame_presented || tasks_in_frame >= max_tasks_per_frame) {
                translator.begin_frame();
                frame_presented = false;
                tasks_in_frame = 0;
                vis_waited = 0;
            }
            auto start = bm64lus::PerfMonitor::Clock::now();
            translator.translate_task(task->t.data_ptr);
            frame_perf.translate_ms += std::chrono::duration<double, std::milli>(bm64lus::PerfMonitor::Clock::now() - start).count();
            tasks_in_frame++;
            if (++tasks_logged <= 3) {
                printf("[LUS] display list %u translated\n", tasks_logged);
            }
        }

        void update_screen() override {
            window->HandleEvents();
            if (!window->IsRunning()) {
                ultramodern::quit();
                return;
            }

            // Starting the game needs a valid VI mode, which the VI thread only installs (a dummy one until the game
            // sets its own) right before it queues the first screen update.
            if (!autostart_attempted) {
                autostart_attempted = true;
                autostart_game();
            }

            frame_perf.vis++;
            uint32_t origin = ultramodern::renderer::get_vi_regs()->VI_ORIGIN_REG;
            bool have_frame = !frame_presented && translator.has_commands();
            if (have_frame) {
                vis_waited++;
            }

            if (have_frame && (origin != last_origin || vis_waited >= max_unpresented_vis)) {
                draw_frame(reinterpret_cast<Gfx*>(translator.finish_frame()));
                if (++frames_logged <= 3) {
                    printf("[LUS] frame %u presented\n", frames_logged);
                }
                if (frames_logged == rdram_dump_frame) {
                    // Debugging aid: BM64_RDRAM_DUMP=<frame> saves the game's 8MB of RDRAM (as 32-bit host-endian
                    // words) to rdram.bin when that frame is presented.
                    if (FILE* dump = fopen("rdram.bin", "wb")) {
                        fwrite(rdram, 1, 0x800000, dump);
                        fclose(dump);
                    }
                }
                frame_presented = true;
                last_origin = origin;
            } else if (!have_frame && !ultramodern::is_game_started()) {
                // Keep the menu responsive before the game sends its first frame.
                window->RunGuiOnly();
            }
        }

        void shutdown() override {
            SDL_DelEventWatch(forward_sdl_event, nullptr);
            translator.clear_texture_store();
            window.reset();
            context.reset();
        }

        uint32_t get_display_framerate() const override {
            uint32_t rate = window ? window->GetCurrentRefreshRate() : 60;
            return rate != 0 ? rate : 60;
        }

        float get_resolution_scale() const override {
            if (!window) {
                return 1.0f;
            }
            return static_cast<float>(window->GetHeight()) / 240.0f;
        }

    private:
        // Fast3dWindow::DrawAndRunGraphicsCommands, split up so drawing and presenting are timed separately.
        void draw_frame(Gfx* commands) {
            static const std::unordered_map<Mtx*, MtxF> no_replacements;
            using Clock = bm64lus::PerfMonitor::Clock;
            auto interpreter = window->GetInterpreterWeak().lock();
            auto gui = window->GetGui();

            auto start = Clock::now();
            window->GetMouseStateManager()->StartFrame();
            gui->StartDraw();
            interpreter->StartFrame();
            interpreter->Run(commands, no_replacements);
            if (bm64lus::perf_overlay_enabled()) {
                bm64lus::draw_perf_overlay(perf.overlay_text().c_str());
            }
            gui->EndDraw();
            auto drawn = Clock::now();
            interpreter->EndFrame();
            auto presented = Clock::now();

            frame_perf.draw_ms = std::chrono::duration<double, std::milli>(drawn - start).count();
            frame_perf.present_ms = std::chrono::duration<double, std::milli>(presented - drawn).count();
            frame_perf.shaders_created = interpreter->mShadersCreated - last_shaders_created;
            frame_perf.textures_uploaded = interpreter->mTexturesUploaded - last_textures_uploaded;
            frame_perf.textures_staged = translator.stats.textures_staged - last_stats.textures_staged;
            frame_perf.textures_new = translator.stats.textures_new - last_stats.textures_new;
            frame_perf.texture_bytes_hashed = translator.stats.texture_bytes_hashed - last_stats.texture_bytes_hashed;
            last_shaders_created = interpreter->mShadersCreated;
            last_textures_uploaded = interpreter->mTexturesUploaded;
            last_stats = translator.stats;
            perf.record(frame_perf, presented);
            frame_perf = {};
        }

        bm64lus::PerfMonitor perf;
        bm64lus::PerfMonitor::Frame frame_perf;
        bm64lus::DisplayListTranslator::Stats last_stats;
        uint32_t last_shaders_created = 0;
        uint32_t last_textures_uploaded = 0;

        uint8_t* rdram;
        uint32_t rdram_dump_frame = 0;
        std::shared_ptr<Ship::Context> context;
        std::shared_ptr<Fast::Fast3dWindow> window;
        bm64lus::DisplayListTranslator translator;
        bool initialized = false;
        bool autostart_attempted = false;
        uint32_t tasks_logged = 0;
        uint32_t frames_logged = 0;
        bool frame_presented = true;
        uint32_t tasks_in_frame = 0;
        uint32_t vis_waited = 0;
        uint32_t last_origin = 0;
    };
}

std::unique_ptr<ultramodern::renderer::RendererContext> zelda64::renderer::create_render_context(
    uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode) {
    (void)window_handle;
    return std::make_unique<LUSContext>(rdram, developer_mode);
}
