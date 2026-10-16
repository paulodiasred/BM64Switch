#ifndef __BM64_PERF_MONITOR_H__
#define __BM64_PERF_MONITOR_H__

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace bm64lus {
    // Frame timing measurements for the renderer. Every presented frame is recorded with how long each stage took;
    // a summary is logged every few seconds, and every hitch is logged with its breakdown, so a log from a play
    // session shows where the time goes.
    class PerfMonitor {
    public:
        using Clock = std::chrono::steady_clock;

        // Work done for one presented frame. Counters are deltas since the previous presented frame.
        struct Frame {
            double translate_ms = 0.0; // display list translation (send_dl) since the previous frame
            double draw_ms = 0.0;      // interpreter run + GUI, until the swap
            double present_ms = 0.0;   // swap: frame limiter, vsync wait and driver work
            uint32_t vis = 0;          // VIs since the previous presented frame
            uint32_t shaders_created = 0;
            uint32_t textures_uploaded = 0;
            uint32_t textures_staged = 0;
            uint32_t textures_new = 0;
            uint64_t texture_bytes_hashed = 0;
        };

        // Records a frame presented at `end` (the time its swap returned).
        void record(const Frame& frame, Clock::time_point end);
        // Short text for the on-screen overlay, refreshed once per second.
        const std::string& overlay_text() const { return overlay; }

    private:
        struct Totals {
            uint32_t frames = 0;
            uint32_t hitches = 0;
            uint32_t vis = 0;
            double translate_sum = 0.0, translate_max = 0.0;
            double draw_sum = 0.0, draw_max = 0.0;
            double present_sum = 0.0, present_max = 0.0;
            uint32_t shaders_created = 0;
            uint32_t textures_uploaded = 0;
            uint32_t textures_staged = 0;
            uint32_t textures_new = 0;
            uint64_t texture_bytes_hashed = 0;
            std::vector<float> intervals;

            void add(const Frame& frame, double interval_ms, bool hitch);
        };

        bool have_previous = false;
        Clock::time_point previous_end;
        Clock::time_point report_start;
        Clock::time_point overlay_start;
        Totals report_totals;
        Totals overlay_totals;
        std::string overlay;

        void log_report(double seconds);
        void update_overlay(double seconds);
    };
}

#endif
