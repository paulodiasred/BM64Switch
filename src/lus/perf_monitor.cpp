#include "perf_monitor.h"

#include <algorithm>
#include <cstdio>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace {
    // A frame interval this long is a visible hitch (the game normally presents every 16.7 or 33.3 ms).
    constexpr double hitch_ms = 40.0;
    constexpr double report_seconds = 5.0;

    double seconds_between(bm64lus::PerfMonitor::Clock::time_point a, bm64lus::PerfMonitor::Clock::time_point b) {
        return std::chrono::duration<double>(b - a).count();
    }

    uint32_t cpu_clock_mhz() {
#ifdef __SWITCH__
        ClkrstSession session = {};
        u32 hz = 0;
        if (R_SUCCEEDED(clkrstOpenSession(&session, PcvModuleId_CpuBus, 3))) {
            clkrstGetClockRate(&session, &hz);
            clkrstCloseSession(&session);
        }
        return hz / 1000000;
#else
        return 0;
#endif
    }

    float percentile(std::vector<float> values, float fraction) {
        if (values.empty()) {
            return 0.0f;
        }
        size_t index = std::min(values.size() - 1, static_cast<size_t>(fraction * values.size()));
        std::nth_element(values.begin(), values.begin() + index, values.end());
        return values[index];
    }
}

namespace bm64lus {
    void PerfMonitor::Totals::add(const Frame& frame, double interval_ms, bool hitch) {
        frames++;
        hitches += hitch ? 1 : 0;
        vis += frame.vis;
        translate_sum += frame.translate_ms;
        translate_max = std::max(translate_max, frame.translate_ms);
        draw_sum += frame.draw_ms;
        draw_max = std::max(draw_max, frame.draw_ms);
        present_sum += frame.present_ms;
        present_max = std::max(present_max, frame.present_ms);
        shaders_created += frame.shaders_created;
        textures_uploaded += frame.textures_uploaded;
        textures_staged += frame.textures_staged;
        textures_new += frame.textures_new;
        texture_bytes_hashed += frame.texture_bytes_hashed;
        if (interval_ms > 0.0) {
            intervals.push_back(static_cast<float>(interval_ms));
        }
    }

    void PerfMonitor::record(const Frame& frame, Clock::time_point end) {
        double interval_ms = have_previous ? seconds_between(previous_end, end) * 1000.0 : 0.0;
        if (!have_previous) {
            report_start = end;
            overlay_start = end;
        }
        have_previous = true;
        previous_end = end;

        bool hitch = interval_ms > hitch_ms;
        if (hitch) {
            printf("[perf] hitch %.1f ms: translate %.1f draw %.1f present %.1f, %u VIs, %u new shaders, "
                   "%u texture uploads, %u new textures\n",
                   interval_ms, frame.translate_ms, frame.draw_ms, frame.present_ms, frame.vis, frame.shaders_created,
                   frame.textures_uploaded, frame.textures_new);
        }
        report_totals.add(frame, interval_ms, hitch);
        overlay_totals.add(frame, interval_ms, hitch);

        double report_elapsed = seconds_between(report_start, end);
        if (report_elapsed >= report_seconds) {
            log_report(report_elapsed);
            report_totals = {};
            report_start = end;
            // The log is buffered (see main); write it out along with the report.
            fflush(stdout);
        }
        double overlay_elapsed = seconds_between(overlay_start, end);
        if (overlay_elapsed >= 1.0) {
            update_overlay(overlay_elapsed);
            overlay_totals = {};
            overlay_start = end;
        }
    }

    void PerfMonitor::log_report(double seconds) {
        const Totals& t = report_totals;
        if (t.frames == 0) {
            return;
        }
        float worst = t.intervals.empty() ? 0.0f : *std::max_element(t.intervals.begin(), t.intervals.end());
        printf("[perf] %.1f fps (%.2f VIs/frame), frame p50 %.1f p99 %.1f max %.1f ms, %u hitches | "
               "translate %.1f/%.1f draw %.1f/%.1f present %.1f/%.1f ms (avg/max) | "
               "shaders +%u, uploads %u, staged %u (new %u, %.1f MB hashed) | cpu %u MHz\n",
               t.frames / seconds, static_cast<double>(t.vis) / t.frames, percentile(t.intervals, 0.5f),
               percentile(t.intervals, 0.99f), worst, t.hitches, t.translate_sum / t.frames, t.translate_max,
               t.draw_sum / t.frames, t.draw_max, t.present_sum / t.frames, t.present_max, t.shaders_created,
               t.textures_uploaded, t.textures_staged, t.textures_new, t.texture_bytes_hashed / (1024.0 * 1024.0),
               cpu_clock_mhz());
    }

    void PerfMonitor::update_overlay(double seconds) {
        const Totals& t = overlay_totals;
        if (t.frames == 0) {
            return;
        }
        float worst = t.intervals.empty() ? 0.0f : *std::max_element(t.intervals.begin(), t.intervals.end());
        char text[256];
        snprintf(text, sizeof(text),
                 "%.0f FPS  worst %.1f ms  hitches %u\n"
                 "translate %.1f  draw %.1f  present %.1f ms\n"
                 "shaders +%u  uploads %u",
                 t.frames / seconds, worst, t.hitches, t.translate_sum / t.frames, t.draw_sum / t.frames,
                 t.present_sum / t.frames, t.shaders_created, t.textures_uploaded);
        overlay = text;
    }
}
