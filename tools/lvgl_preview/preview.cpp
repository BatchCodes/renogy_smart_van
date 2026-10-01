// SPDX-License-Identifier: GPL-3.0-or-later
//
// Renders each dashboard page and state at 800 x 480 and writes PPM images, so a
// layout change can be checked with no board.

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "cab_dashboard/dashboard_view.hpp"
#include "cab_ui/dashboard_text.hpp"
#include "cab_ui/power_history.hpp"
#include "charging_data/fake_source.hpp"
#include "lvgl.h"

namespace {

constexpr std::int32_t kWidth = 800;
constexpr std::int32_t kHeight = 480;

std::vector<std::uint16_t> frame(static_cast<std::size_t>(kWidth) * kHeight);
std::vector<std::uint16_t> draw_buffer(static_cast<std::size_t>(kWidth) * kHeight);

void flush(lv_display_t* display, const lv_area_t* area, std::uint8_t* pixels) {
    const auto* source = reinterpret_cast<const std::uint16_t*>(pixels);
    const std::int32_t width = lv_area_get_width(area);
    for (std::int32_t y = area->y1; y <= area->y2; ++y) {
        for (std::int32_t x = area->x1; x <= area->x2; ++x) {
            frame[static_cast<std::size_t>(y) * kWidth + x] = source[(y - area->y1) * width + (x - area->x1)];
        }
    }
    lv_display_flush_ready(display);
}

bool write_ppm(const std::string& path) {
    FILE* file = std::fopen(path.c_str(), "wb");
    if (file == nullptr) {
        return false;
    }
    std::fprintf(file, "P6\n%ld %ld\n255\n", static_cast<long>(kWidth), static_cast<long>(kHeight));
    for (const std::uint16_t pixel : frame) {
        const std::uint8_t rgb[3] = {
            static_cast<std::uint8_t>(((pixel >> 11) & 0x1F) * 255 / 31),
            static_cast<std::uint8_t>(((pixel >> 5) & 0x3F) * 255 / 63),
            static_cast<std::uint8_t>((pixel & 0x1F) * 255 / 31),
        };
        std::fwrite(rgb, 1, sizeof(rgb), file);
    }
    std::fclose(file);
    return true;
}

void render(const std::string& path) {
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(nullptr);
    if (write_ppm(path)) {
        std::printf("Wrote %s\n", path.c_str());
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string out_dir = argc > 1 ? argv[1] : ".";

    lv_init();
    lv_display_t* display = lv_display_create(kWidth, kHeight);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buffer.data(), nullptr, draw_buffer.size() * sizeof(std::uint16_t),
                           LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, flush);

    cab_dashboard::DashboardView view;
    view.create(lv_screen_active());

    // An hour of fake solar data, one sample every 30 s, ending at noon of the fake day.
    const charging_data::FakeSource source({.day_length_ms = 14'400'000, .outage_length_ms = 0});
    cab_ui::PowerHistory history(120, 30'000);
    const std::uint64_t noon_ms = 3'600'000;
    for (std::uint64_t time_ms = noon_ms - 3'600'000; time_ms <= noon_ms; time_ms += 30'000) {
        history.add(time_ms, source.reading_at(time_ms).pv_power_w);
    }

    auto data = source.reading_at(noon_ms);
    data.battery_soc_percent = 87;
    const auto connected = cab_ui::format_dashboard({data, charging_data::LinkState::connected, 2'000});

    const struct {
        cab_dashboard::Page page;
        const char* name;
    } pages[] = {
        {cab_dashboard::Page::solar, "solar"},
        {cab_dashboard::Page::batteries, "batteries"},
        {cab_dashboard::Page::temperatures, "temperatures"},
        {cab_dashboard::Page::energy, "energy"},
    };
    view.update(connected, history);
    for (const auto& page : pages) {
        view.show_page(page.page);
        render(out_dir + "/" + page.name + ".ppm");
    }

    view.show_page(cab_dashboard::Page::solar);
    view.update(cab_ui::format_dashboard({data, charging_data::LinkState::stale, 45'000}), history);
    render(out_dir + "/stale.ppm");

    view.update(cab_ui::format_dashboard({data, charging_data::LinkState::offline, 300'000}), history);
    render(out_dir + "/offline.ppm");

    view.update(cab_ui::format_dashboard(cab_ui::DataSnapshot{}), cab_ui::PowerHistory(120, 30'000));
    render(out_dir + "/waiting.ppm");
    return 0;
}
