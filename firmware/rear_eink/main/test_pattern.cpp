// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_pattern.hpp"

#include <cstdio>

#include "board.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mono_gfx/text.hpp"
#include "sdkconfig.h"

namespace rear_eink {

namespace {

constexpr const char* kTag = "test_pattern";

using mono_gfx::Align;
using mono_gfx::Color;
using mono_gfx::FrameBuffer;

void draw_pattern(FrameBuffer& frame, const char* panel_name, unsigned counter) {
    frame.clear(Color::white);
    frame.draw_rect(0, 0, FrameBuffer::kWidth, FrameBuffer::kHeight, Color::black);

    // An arrow in the top left corner shows the orientation.
    for (int step = 0; step < 12; ++step) {
        frame.draw_hline(4, 4 + step, 12 - step, Color::black);
    }

    // A checkerboard shows ghosting after fast refreshes.
    constexpr int kCell = 6;
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 8; ++column) {
            if ((row + column + counter) % 2 == 0) {
                frame.fill_rect(196 + column * kCell, 6 + row * kCell, kCell, kCell, Color::black);
            }
        }
    }

    mono_gfx::draw_text(frame, mono_gfx::kFontSmall, 22, 4, panel_name);

    char count_text[16];
    std::snprintf(count_text, sizeof(count_text), "%u", counter);
    mono_gfx::draw_text(frame, mono_gfx::kFontLarge, FrameBuffer::kWidth / 2, 34, count_text,
                        Align::centre);

    mono_gfx::draw_text(frame, mono_gfx::kFontMedium, FrameBuffer::kWidth / 2, 92, "Test pattern",
                        Align::centre);
}

}  // namespace

void run_test_pattern(epaper_panel::PanelDevice& device) {
    static FrameBuffer frame(rotation());
    unsigned counter = 0;

    for (;;) {
        const bool full = counter % CONFIG_REAR_EINK_FULL_REFRESH_EVERY == 0;
        draw_pattern(frame, device.panel().name(), counter);

        const std::int64_t start_us = esp_timer_get_time();
        const esp_err_t result = device.panel().show(
            frame.native(), full ? epaper_panel::RefreshMode::full : epaper_panel::RefreshMode::fast);
        const std::int64_t elapsed_ms = (esp_timer_get_time() - start_us) / 1000;

        if (result == ESP_OK) {
            ESP_LOGI(kTag, "%s refresh %u took %lld ms", full ? "Full" : "Fast", counter, elapsed_ms);
        } else {
            ESP_LOGE(kTag, "Refresh %u failed after %lld ms: %s", counter, elapsed_ms,
                     esp_err_to_name(result));
        }

        device.panel().sleep();
        ++counter;
        vTaskDelay(pdMS_TO_TICKS(CONFIG_REAR_EINK_REFRESH_INTERVAL_S * 1000));
    }
}

}  // namespace rear_eink
