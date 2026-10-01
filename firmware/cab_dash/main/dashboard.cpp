// SPDX-License-Identifier: GPL-3.0-or-later
#include "dashboard.hpp"

#include "bsp/esp-bsp.h"
#include "cab_dashboard/dashboard_view.hpp"
#include "cab_ui/dashboard_text.hpp"
#include "cab_ui/mode_switch.hpp"
#include "cab_ui/power_history.hpp"
#include "camera.hpp"
#include "camera_screen.hpp"
#include "display.hpp"
#include "esp_log.h"
#include "reverse_input.hpp"
#include "sdkconfig.h"

namespace cab_dash {

namespace {

constexpr const char* kTag = "dashboard";
constexpr std::uint32_t kRedrawIntervalMs = 500;
constexpr std::uint32_t kModeIntervalMs = 20;
// One hour of solar power, one sample every 30 s.
constexpr std::size_t kHistorySamples = 120;
constexpr std::uint32_t kHistoryIntervalMs = 30'000;

struct Dashboard {
    DataHub* hub = nullptr;
    cab_dashboard::DashboardView view;
    CameraScreen camera_screen;
    cab_ui::PowerHistory history{kHistorySamples, kHistoryIntervalMs};
    cab_ui::ModeSwitch mode{{.camera_hold_ms = CONFIG_CAB_DASH_CAMERA_HOLD_MS}};
    ReverseInput reverse_input;
    cab_ui::Screen shown = cab_ui::Screen::dashboard;
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
        ESP_LOGI(kTag, "Backlight on");
    }
}

// The timers run in the LVGL task, so they can change widgets without the lock.
void on_redraw(lv_timer_t* /*timer*/) {
    const std::uint64_t time_ms = now_ms();
    const auto snapshot = dashboard.hub->snapshot(time_ms);
    if (snapshot.data && snapshot.link_state != charging_data::LinkState::offline) {
        dashboard.history.add(time_ms, snapshot.data->pv_power_w);
    }
    dashboard.view.update(cab_ui::format_dashboard(snapshot), dashboard.history);
    update_backlight();
}

void on_mode(lv_timer_t* /*timer*/) {
    const std::uint64_t time_ms = now_ms();
    const bool reverse = dashboard.reverse_input.sample(time_ms);
    dashboard.mode.set_reverse(reverse, time_ms);
    if (reverse) {
        // Reverse counts as activity, so the backlight comes on for the camera.
        lv_display_trigger_activity(nullptr);
    }

    const cab_ui::Screen wanted = dashboard.mode.screen(time_ms);
    if (wanted != dashboard.shown) {
        lv_screen_load(wanted == cab_ui::Screen::camera ? dashboard.camera_screen.view().screen()
                                                        : dashboard.view.screen());
        dashboard.shown = wanted;
        ESP_LOGI(kTag, "Showing the %s", wanted == cab_ui::Screen::camera ? "camera" : "dashboard");
    }
    dashboard.camera_screen.view().set_reverse(dashboard.mode.reverse());
}

void on_camera_pressed(void* /*context*/) { dashboard.mode.request(cab_ui::Screen::camera); }

void on_dashboard_pressed(void* /*context*/) { dashboard.mode.request(cab_ui::Screen::dashboard); }

}  // namespace

void start_dashboard(DataHub& hub) {
    dashboard.hub = &hub;
    dashboard.reverse_input.init();
    if (Camera::instance().start() != ESP_OK) {
        ESP_LOGE(kTag, "Camera start failed. The camera screen shows no signal.");
    }

    const DisplayLock lock;
    if (!lock.locked()) {
        ESP_LOGE(kTag, "Display lock failed");
        return;
    }
    dashboard.view.create(lv_screen_active(), on_camera_pressed, nullptr);
    dashboard.camera_screen.create(on_dashboard_pressed, nullptr);
    lv_timer_create(on_redraw, kRedrawIntervalMs, nullptr);
    lv_timer_create(on_mode, kModeIntervalMs, nullptr);
}

}  // namespace cab_dash
