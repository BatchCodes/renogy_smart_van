// SPDX-License-Identifier: GPL-3.0-or-later
#include "dashboard.hpp"

#include "bsp/esp-bsp.h"
#include "cab_dashboard/dashboard_view.hpp"
#include "cab_ui/dashboard_text.hpp"
#include "cab_ui/power_history.hpp"
#include "display.hpp"
#include "esp_log.h"
#include "sdkconfig.h"

namespace cab_dash {

namespace {

constexpr const char* kTag = "dashboard";
constexpr std::uint32_t kRedrawIntervalMs = 500;
// One hour of solar power, one sample every 30 s.
constexpr std::size_t kHistorySamples = 120;
constexpr std::uint32_t kHistoryIntervalMs = 30'000;

struct Dashboard {
    DataHub* hub = nullptr;
    cab_dashboard::DashboardView view;
    cab_ui::PowerHistory history{kHistorySamples, kHistoryIntervalMs};
    bool backlight_on = true;
};

Dashboard dashboard;

void update_backlight() {
    if (CONFIG_CAB_DASH_BACKLIGHT_TIMEOUT_S == 0) {
        return;
    }
    const bool idle = lv_display_get_inactive_time(nullptr) >= CONFIG_CAB_DASH_BACKLIGHT_TIMEOUT_S * 1000U;
    if (idle && dashboard.backlight_on) {
        bsp_display_backlight_off();
        dashboard.backlight_on = false;
        ESP_LOGI(kTag, "Backlight off after %d s with no touch", CONFIG_CAB_DASH_BACKLIGHT_TIMEOUT_S);
    } else if (!idle && !dashboard.backlight_on) {
        bsp_display_backlight_on();
        bsp_display_brightness_set(CONFIG_CAB_DASH_BRIGHTNESS_PERCENT);
        dashboard.backlight_on = true;
        ESP_LOGI(kTag, "Backlight on after a touch");
    }
}

// Runs in the LVGL task, so it can change widgets without the lock.
void on_redraw(lv_timer_t* /*timer*/) {
    const std::uint64_t time_ms = now_ms();
    const auto snapshot = dashboard.hub->snapshot(time_ms);
    if (snapshot.data && snapshot.link_state != charging_data::LinkState::offline) {
        dashboard.history.add(time_ms, snapshot.data->pv_power_w);
    }
    dashboard.view.update(cab_ui::format_dashboard(snapshot),
                          dashboard.history);
    update_backlight();
}

}  // namespace

void start_dashboard(DataHub& hub) {
    dashboard.hub = &hub;
    const DisplayLock lock;
    if (!lock.locked()) {
        ESP_LOGE(kTag, "Display lock failed");
        return;
    }
    dashboard.view.create(lv_screen_active());
    lv_timer_create(on_redraw, kRedrawIntervalMs, nullptr);
}

}  // namespace cab_dash
