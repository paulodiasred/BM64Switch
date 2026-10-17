#include <cstdio>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <array>
#include <filesystem>
#include <numeric>
#include <stdexcept>
#include <cinttypes>
#include <algorithm>
#include <cctype>

#include "ultramodern/ultra64.h"
#include "ultramodern/ultramodern.hpp"
#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"

#include "recomp_ui.h"
#include "recomp_input.h"
#include "zelda_config.h"
#include "zelda_sound.h"
#include "zelda_render.h"
#include "zelda_support.h"
#include "zelda_cheats.h"
#include "zelda_game.h"
#include "recomp_data.h"
#include "ovl_patches.hpp"
#include "librecomp/game.hpp"
#include "librecomp/mods.hpp"
#include "librecomp/helpers.hpp"

#include "../../patches/graphics.h"
#include "../../patches/input.h"
#include "../../patches/sound.h"
#include "../../patches/misc_funcs.h"
#include "../../patches/cheats.h"

// #include "mods/mm_recomp_dpad_builtin.h"

const std::string version_string = "1.0.0";

#ifdef __SWITCH__
#include <switch.h>
#include <sys/stat.h>
#include <unistd.h>

// Folder the .nro was launched from, also searched for the ROM.
static std::filesystem::path launch_folder;
#endif

template<typename... Ts>
void exit_error(const char* str, Ts ...args) {
    // TODO pop up an error
    ((void)fprintf(stderr, str, args), ...);
    assert(false);
        
    ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
}

ultramodern::gfx_callbacks_t::gfx_data_t create_gfx() {
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) > 0) {
        exit_error("Failed to initialize SDL2: %s\n", SDL_GetError());
    }

    fprintf(stdout, "SDL Video Driver: %s\n", SDL_GetCurrentVideoDriver());

    return {};
}

// libultraship creates the window, its GL context and pumps SDL events on the graphics thread, so there's
// no window for the main thread to create or update.
ultramodern::renderer::WindowHandle create_window(ultramodern::gfx_callbacks_t::gfx_data_t) {
    return {};
}

void update_gfx(void*) {}

static SDL_AudioCVT audio_convert;
static SDL_AudioDeviceID audio_device = 0;

// Samples per channel per second.
static uint32_t sample_rate = 48000;
static uint32_t output_sample_rate = 48000;
// Channel count.
constexpr uint32_t input_channels = 2;
static uint32_t output_channels = 2;

// Terminology: a frame is a collection of samples for each channel. e.g. 2 input samples is one input frame. This is unrelated to graphical frames.

// Number of frames to duplicate for fixing interpolation at the start and end of a chunk.
constexpr uint32_t duplicated_input_frames = 4;
// The number of output frames to skip for playback (to avoid playing duplicate inputs twice).
static uint32_t discarded_output_frames;

constexpr uint32_t bytes_per_frame = input_channels * sizeof(float);

void queue_samples(int16_t* audio_data, size_t sample_count) {
    // Buffer for holding the output of swapping the audio channels. This is reused across
    // calls to reduce runtime allocations.
    static std::vector<float> swap_buffer;
    static std::array<float, duplicated_input_frames * input_channels> duplicated_sample_buffer;

    // Make sure the swap buffer is large enough to hold the audio data, including any extra space needed for resampling.
    size_t resampled_sample_count = sample_count + duplicated_input_frames * input_channels;
    size_t max_sample_count = std::max(resampled_sample_count, resampled_sample_count * audio_convert.len_mult);
    if (max_sample_count > swap_buffer.size()) {
        swap_buffer.resize(max_sample_count);
    }
    
    // Copy the duplicated frames from last chunk into this chunk
    for (size_t i = 0; i < duplicated_input_frames * input_channels; i++) {
        swap_buffer[i] = duplicated_sample_buffer[i];
    }

    // Convert the audio from 16-bit values to floats and swap the audio channels into the
    // swap buffer to correct for the address xor caused by endianness handling.
    float cur_main_volume = zelda64::get_main_volume() / 100.0f; // Get the current main volume, normalized to 0.0-1.0.
    for (size_t i = 0; i < sample_count; i += input_channels) {
        swap_buffer[i + 0 + duplicated_input_frames * input_channels] = audio_data[i + 1] * (1.0f / 32768.0f) * cur_main_volume;
        swap_buffer[i + 1 + duplicated_input_frames * input_channels] = audio_data[i + 0] * (1.0f / 32768.0f) * cur_main_volume;
    }
    
    // TODO handle cases where a chunk is smaller than the duplicated frame count.
    assert(sample_count > duplicated_input_frames * input_channels);

    // Copy the last converted samples into the duplicated sample buffer to reuse in resampling the next queued chunk.
    for (size_t i = 0; i < duplicated_input_frames * input_channels; i++) {
        duplicated_sample_buffer[i] = swap_buffer[i + sample_count];
    }
    
    audio_convert.buf = reinterpret_cast<Uint8*>(swap_buffer.data());
    audio_convert.len = (sample_count + duplicated_input_frames * input_channels) * sizeof(swap_buffer[0]);

    int ret = SDL_ConvertAudio(&audio_convert);

    if (ret < 0) {
        printf("Error using SDL audio converter: %s\n", SDL_GetError());
        throw std::runtime_error("Error using SDL audio converter");
    }

    uint64_t cur_queued_microseconds = uint64_t(SDL_GetQueuedAudioSize(audio_device)) / bytes_per_frame * 1000000 / sample_rate;
    uint32_t num_bytes_to_queue = audio_convert.len_cvt - output_channels * discarded_output_frames * sizeof(swap_buffer[0]);
    float* samples_to_queue = swap_buffer.data() + output_channels * discarded_output_frames / 2;

    // Prevent audio latency from building up by skipping samples in incoming audio when too many samples are already queued.
    // Skip samples based on how many microseconds of samples are queued already.
    uint32_t skip_factor = cur_queued_microseconds / 100000;
    if (skip_factor != 0) {
        uint32_t skip_ratio = 1 << skip_factor;
        num_bytes_to_queue /= skip_ratio;
        for (size_t i = 0; i < num_bytes_to_queue / (output_channels * sizeof(swap_buffer[0])); i++) {
            samples_to_queue[2 * i + 0] = samples_to_queue[2 * skip_ratio * i + 0];
            samples_to_queue[2 * i + 1] = samples_to_queue[2 * skip_ratio * i + 1];
        }
    }

    // Queue the swapped audio data.
    // Offset the data start by only half the discarded frame count as the other half of the discarded frames are at the end of the buffer.
    SDL_QueueAudio(audio_device, samples_to_queue, num_bytes_to_queue);
}

size_t get_frames_remaining() {
    constexpr float buffer_offset_frames = 1.0f;
    // Get the number of remaining buffered audio bytes.
    uint64_t buffered_byte_count = SDL_GetQueuedAudioSize(audio_device);

    // Scale the byte count based on the ratio of sample rates and channel counts.
    buffered_byte_count = buffered_byte_count * 2 * sample_rate / output_sample_rate / output_channels;

    // Adjust the reported count to be some number of refreshes in the future, which helps ensure that
    // there are enough samples even if the audio thread experiences a small amount of lag. This prevents
    // audio popping on games that use the buffered audio byte count to determine how many samples
    // to generate.
    uint32_t frames_per_vi = (sample_rate / 60);
    if (buffered_byte_count > (buffer_offset_frames * bytes_per_frame * frames_per_vi)) {
        buffered_byte_count -= (buffer_offset_frames * bytes_per_frame * frames_per_vi);
    }
    else {
        buffered_byte_count = 0;
    }
    // Convert from byte count to sample count.
    return static_cast<uint32_t>(buffered_byte_count / bytes_per_frame);
}

void update_audio_converter() {
    int ret = SDL_BuildAudioCVT(&audio_convert, AUDIO_F32, input_channels, sample_rate, AUDIO_F32, output_channels, output_sample_rate);

    if (ret < 0) {
        printf("Error creating SDL audio converter: %s\n", SDL_GetError());
        throw std::runtime_error("Error creating SDL audio converter");
    }

    // Calculate the number of samples to discard based on the sample rate ratio and the duplicate frame count.
    discarded_output_frames = duplicated_input_frames * output_sample_rate / sample_rate;
}

void set_frequency(uint32_t freq) {
    sample_rate = freq;
    
    update_audio_converter();
}

void reset_audio(uint32_t output_freq) {
    SDL_AudioSpec spec_desired{
        .freq = (int)output_freq,
        .format = AUDIO_F32,
        .channels = (Uint8)output_channels,
        .silence = 0, // calculated
#ifdef __SWITCH__
        // The Switch's audio output can't keep up with tiny buffers (constant underruns crackle and hiss).
        .samples = 0x800,
#else
        .samples = 0x100, // Fairly small sample count to reduce the latency of internal buffering
#endif
        .padding = 0, // unused
        .size = 0, // calculated
        .callback = nullptr,
        .userdata = nullptr
    };


    audio_device = SDL_OpenAudioDevice(nullptr, false, &spec_desired, nullptr, 0);
    if (audio_device == 0) {
        exit_error("SDL error opening audio device: %s\n", SDL_GetError());
    }
    SDL_PauseAudioDevice(audio_device, 0);

    output_sample_rate = output_freq;
    update_audio_converter();
}

// extern RspUcodeFunc njpgdspMain;
extern RspUcodeFunc aspMain;

RspUcodeFunc* get_rsp_microcode(const OSTask* task) {
    switch (task->t.type) {
    case M_AUDTASK:
        return aspMain;

    // case M_NJPEGTASK:
    //     return njpgdspMain;

    default:
        fprintf(stderr, "Unknown task: %" PRIu32 "\n", task->t.type);
        return nullptr;
    }
}

extern "C" void recomp_entrypoint(uint8_t * rdram, recomp_context * ctx);
extern "C" void bm64_get_file_dump_enabled(uint8_t* rdram, recomp_context* ctx);
extern "C" void bm64_dump_file(uint8_t* rdram, recomp_context* ctx);
extern "C" void bm64_on_file_loaded(uint8_t* rdram, recomp_context* ctx);
gpr get_entrypoint_address();

// array of supported GameEntry objects
std::vector<recomp::GameEntry> supported_games = {
    {
        .rom_hash = 0xc4c0b74bbb696426ULL,
        .internal_name = "BOMBERMAN64",
        .game_id = u8"bm64_us",
        .mod_game_id = "bm64",
        .save_type = recomp::SaveType::AllowAll,
        .is_enabled = true,
        .entrypoint_address = get_entrypoint_address(),
        .entrypoint = recomp_entrypoint,
    },
};

// TODO: move somewhere else
/*
 * 0 = PI (pri 0x96)
 * 0 = VI (pri 0xFE)
 * 1 = Init (pri 0xA)
 * 3 = Game (pri 0xA)
 * 4 = Looks like RSP Thread (pri 0x7F)
 */
namespace zelda64 {
    std::string get_game_thread_name(const OSThread* t) {
        std::string name = "[Game] ";

        switch (t->id) {
            case 0:
                switch (t->priority) {
                    case 0x96:
                        name += "PIMGR";
                        break;
                    case 0xFE:
                        name += "VIMGR";
                        break;
                    default:
                        name += std::to_string(t->id);
                        break;
                }
                break;
            case 1:
                name += "INIT";
                break;
            case 3:
                name += "GAME";
                break;
            case 4:
                name += "RSP";
                break;
            default:
                name += std::to_string(t->id);
                break;
        }
        return name;
    }
}

// There's no launcher menu, so start the game as soon as the renderer is up. The ROM is taken from the stored copy
// or, on first boot, from any N64 ROM file (.z64, .n64 or .v64, any byte order) in the app or program folder.
void autostart_game() {
    std::u8string game_id = supported_games[0].game_id;

    if (!recomp::is_rom_valid(game_id)) {
#ifdef __SWITCH__
        const std::filesystem::path search_dirs[] = { zelda64::get_app_folder_path(), launch_folder, "sdmc:/switch", "sdmc:/" };
#else
        const std::filesystem::path search_dirs[] = { zelda64::get_app_folder_path(), zelda64::get_program_path(), std::filesystem::current_path() };
#endif
        for (const auto& dir : search_dirs) {
            // Iterate with error codes: the throwing filesystem calls fail on some Switch paths.
            std::error_code ec;
            std::filesystem::directory_iterator it(dir.empty() ? "." : dir, ec);
            for (; !ec && it != std::filesystem::directory_iterator(); it.increment(ec)) {
                const std::filesystem::path path = it->path();
                std::string ext = path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
                if (ext != ".z64" && ext != ".n64" && ext != ".v64") {
                    continue;
                }
                // select_rom stores a copy of the ROM when it matches, which makes is_rom_valid true.
                recomp::RomValidationError error = recomp::select_rom(path, game_id);
                fprintf(stdout, "ROM %s: %s\n", path.string().c_str(), error == recomp::RomValidationError::Good ? "valid" : "rejected");
                if (error == recomp::RomValidationError::Good) {
                    recomp::check_all_stored_roms();
                    break;
                }
            }
            if (recomp::is_rom_valid(game_id)) {
                break;
            }
        }
    }

    if (!recomp::is_rom_valid(game_id)) {
#ifdef __SWITCH__
        std::string message = "No valid Bomberman 64 (USA) ROM was found.\n\nPlace your Bomberman 64 (USA) ROM (.z64, .n64 or .v64) "
            "on the SD card, in sdmc:/switch/bm64recomp/, sdmc:/switch/ or the root of the card.";
#else
        std::string message = "No valid Bomberman 64 (USA) ROM was found.\n\nPlace your Bomberman 64 (USA) ROM (.z64, .n64 or .v64) in:\n" +
            zelda64::get_app_folder_path().string();
#endif
        recompui::message_box(message.c_str());
        ultramodern::quit();
        return;
    }

    recomp::start_game(game_id);
}

void on_gfx_init() {
    recompui::update_supported_options();
}


#define REGISTER_FUNC(name) recomp::overlays::register_base_export(#name, name)

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

#ifdef __SWITCH__
    // Shaders and the controller database are embedded in the .nro's RomFS, so the .nro is the only file to install.
    romfsInit();
    std::error_code switch_ec;
    launch_folder = std::filesystem::current_path(switch_ec);
    // std::filesystem doesn't understand the "sdmc:" device prefix when walking parent folders, so create them directly.
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/bm64recomp", 0777);
    chdir("sdmc:/switch/bm64recomp");

    // There's no console on the Switch, so keep logs on the SD card. Every write to the SD card is slow (an unbuffered
    // log line stalled the game for ~100 ms), so stdout is buffered and flushed every few seconds by the renderer's
    // performance report; stderr stays unbuffered so errors survive a crash.
    // stderr gets its own file because the SD card can't have one file open for writing twice.
    if (freopen("sdmc:/switch/bm64recomp/bm64.log", "w", stdout) != nullptr) {
        setvbuf(stdout, nullptr, _IOFBF, 64 * 1024);
    }
    if (freopen("sdmc:/switch/bm64recomp/bm64-err.log", "w", stderr) != nullptr) {
        setvbuf(stderr, nullptr, _IONBF, 0);
    }
    printf("[Switch] applet type %d, launched from %s\n", (int)appletGetAppletType(), launch_folder.string().c_str());
#endif
    recomp::Version project_version{};
    if (!recomp::Version::from_string(version_string, project_version)) {
        ultramodern::error_handling::message_box(("Invalid version string: " + version_string).c_str());
        return EXIT_FAILURE;
    }

    // Initialize SDL audio and set the output frequency.
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    reset_audio(48000);

    // Source controller mappings file
    std::u8string controller_db_path = (zelda64::get_program_path() / "recompcontrollerdb.txt").u8string();
    if (SDL_GameControllerAddMappingsFromFile(reinterpret_cast<const char *>(controller_db_path.c_str())) < 0) {
        fprintf(stderr, "Failed to load controller mappings: %s\n", SDL_GetError());
    }

    recomp::register_config_path(zelda64::get_app_folder_path());

    // Register supported games and patches
    for (const auto& game : supported_games) {
        recomp::register_game(game);
    }

    //recomp::mods::register_embedded_mod("mm_recomp_dpad_builtin", { (const uint8_t*)(mm_recomp_dpad_builtin), std::size(mm_recomp_dpad_builtin)});

    //REGISTER_FUNC(recomp_get_window_resolution);
    REGISTER_FUNC(recomp_get_target_aspect_ratio);
    REGISTER_FUNC(recomp_get_target_framerate);
    REGISTER_FUNC(recomp_get_film_grain_enabled);
    REGISTER_FUNC(recomp_get_invert_y_axis_mode);
    REGISTER_FUNC(recomp_get_radio_comm_box_mode);
    REGISTER_FUNC(recomp_get_camera_inputs);
    REGISTER_FUNC(recomp_get_targeting_mode);
    REGISTER_FUNC(recomp_get_bgm_volume);
    REGISTER_FUNC(recomp_get_low_health_beeps_enabled);
    REGISTER_FUNC(recomp_get_gyro_deltas);
    REGISTER_FUNC(recomp_get_mouse_deltas);
    REGISTER_FUNC(recomp_get_inverted_axes);
    REGISTER_FUNC(recomp_get_analog_inverted_axes);
    REGISTER_FUNC(recomp_get_moonjump);
    REGISTER_FUNC(recomp_get_always_max_bomb);
    REGISTER_FUNC(recomp_get_always_max_fire);
    REGISTER_FUNC(recomp_get_always_have_remote_bombs);
    REGISTER_FUNC(recomp_get_always_have_red_bombs);
    REGISTER_FUNC(recomp_get_infinite_credits);
    REGISTER_FUNC(recomp_get_infinite_lives);
    REGISTER_FUNC(bm64_get_file_dump_enabled);
    REGISTER_FUNC(bm64_dump_file);
    REGISTER_FUNC(bm64_on_file_loaded);
    recompui::register_ui_exports();
    recomputil::register_data_api_exports();

    zelda64::register_overlays();
    zelda64::register_patches();
    // recomputil::init_extended_actor_data();
    zelda64::load_config();

    recomp::rsp::callbacks_t rsp_callbacks{
        .get_rsp_microcode = get_rsp_microcode,
    };

    ultramodern::renderer::callbacks_t renderer_callbacks{
        .create_render_context = zelda64::renderer::create_render_context,
    };

    ultramodern::gfx_callbacks_t gfx_callbacks{
        .create_gfx = create_gfx,
        .create_window = create_window,
        .update_gfx = update_gfx,
    };

    ultramodern::audio_callbacks_t audio_callbacks{
        .queue_samples = queue_samples,
        .get_frames_remaining = get_frames_remaining,
        .set_frequency = set_frequency,
    };

    ultramodern::input::callbacks_t input_callbacks{
        .poll_input = recomp::poll_inputs,
        .get_input = recomp::get_n64_input,
        .set_rumble = recomp::set_rumble,
        .get_connected_device_info = recomp::get_connected_device_info,
    };

    ultramodern::events::callbacks_t thread_callbacks{
        .vi_callback = recomp::update_rumble,
        .gfx_init_callback = on_gfx_init,
    };

    ultramodern::error_handling::callbacks_t error_handling_callbacks{
        .message_box = recompui::message_box,
    };

    ultramodern::threads::callbacks_t threads_callbacks{
        .get_game_thread_name = zelda64::get_game_thread_name,
    };

    recomp::Configuration cfg{
        .project_version = project_version,
        .window_handle = {},
        .rsp_callbacks = rsp_callbacks,
        .renderer_callbacks = renderer_callbacks,
        .audio_callbacks = audio_callbacks,
        .input_callbacks = input_callbacks,
        .gfx_callbacks = gfx_callbacks,
        .events_callbacks = thread_callbacks,
        .error_handling_callbacks = error_handling_callbacks,
        .threads_callbacks = threads_callbacks,
        .message_queue_control = {},
    };

    recomp::start(cfg);

    return EXIT_SUCCESS;
}
