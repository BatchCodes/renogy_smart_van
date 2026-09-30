// SPDX-License-Identifier: GPL-3.0-or-later
#include "charging_display.hpp"

#include "board.hpp"
#include "charging_data/latest_reading.hpp"
#include "eink_view/charging_view.hpp"
#include "eink_view/refresh_scheduler.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace rear_eink {

namespace {

constexpr const char* kTag = "charging_display";

std::uint64_t now_ms() { return static_cast<std::uint64_t>(esp_timer_get_time() / 1000); }

epaper_panel::RefreshMode to_panel_mode(eink_view::Refresh refresh) {
    return refresh == eink_view::Refresh::full ? epaper_panel::RefreshMode::full
                                               : epaper_panel::RefreshMode::fast;
}

}  // namespace

void run_charging_display(epaper_panel::PanelDevice& device, charging_data::ChargingDataSource& source) {
    static mono_gfx::FrameBuffer frame(rotation());
    charging_data::LatestReading reading({
        .stale_after_ms = CONFIG_REAR_EINK_STALE_AFTER_S * 1000U,
        .offline_after_ms = CONFIG_REAR_EINK_OFFLINE_AFTER_S * 1000U,
    });
    eink_view::RefreshScheduler scheduler({
        .full_every_fast = CONFIG_REAR_EINK_FULL_REFRESH_EVERY,
        .max_unchanged_ms = CONFIG_REAR_EINK_MAX_UNCHANGED_S * 1000U,
    });

    for (;;) {
        const std::uint64_t time_ms = now_ms();
        if (const auto data = source.poll(time_ms)) {
            reading.update(*data, time_ms);
        }

        eink_view::render(frame, reading, time_ms);
        const eink_view::Refresh refresh = scheduler.decide(frame.native(), time_ms);

        if (refresh != eink_view::Refresh::none) {
            const std::int64_t start_us = esp_timer_get_time();
            const esp_err_t result = device.panel().show(frame.native(), to_panel_mode(refresh));
            const std::int64_t elapsed_ms = (esp_timer_get_time() - start_us) / 1000;
            if (result == ESP_OK) {
                scheduler.shown(frame.native(), refresh, time_ms);
                ESP_LOGI(kTag, "%s refresh in %lld ms, link %s",
                         refresh == eink_view::Refresh::full ? "Full" : "Fast", elapsed_ms,
                         charging_data::to_string(reading.link_state(time_ms)));
            } else {
                ESP_LOGE(kTag, "Refresh failed: %s", esp_err_to_name(result));
            }
            device.panel().sleep();
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_REAR_EINK_REFRESH_INTERVAL_S * 1000));
    }
}

}  // namespace rear_eink
