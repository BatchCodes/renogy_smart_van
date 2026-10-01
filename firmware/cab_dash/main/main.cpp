// SPDX-License-Identifier: GPL-3.0-or-later
//
// Cab dashboard app for the Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3.

#include "ble.hpp"
#include "charging_data/fake_source.hpp"
#include "dashboard.hpp"
#include "data_hub.hpp"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "renogy_ble/bt2_ble_source.hpp"
#include "sdkconfig.h"
#include "test_screen.hpp"

namespace {

constexpr const char* kTag = "cab_dash";

lv_display_t* start_display() {
    bsp_display_cfg_t config = {};
    config.lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
#if CONFIG_CAB_DASH_ROTATION_270
    config.rotation = ESP_LV_ADAPTER_ROTATE_270;
#else
    config.rotation = ESP_LV_ADAPTER_ROTATE_90;
#endif
    config.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;
    lv_display_t* display = bsp_display_start_with_config(&config);
    if (display != nullptr) {
        bsp_display_backlight_on();
        bsp_display_brightness_set(CONFIG_CAB_DASH_BRIGHTNESS_PERCENT);
    }
    return display;
}

charging_data::ChargingDataSource& start_source() {
#if CONFIG_CAB_DASH_SOURCE_BT2
    auto& source = renogy_ble::Bt2BleSource::instance();
    if (source.start(cab_dash::connect_c6_ble_controller) != ESP_OK) {
        ESP_LOGE(kTag, "BLE start failed. The dashboard shows offline.");
    }
    ESP_LOGI(kTag, "Data source: Renogy BT-2");
    return source;
#else
    // The same five minute fake day as the rear unit, with no outages.
    static charging_data::FakeSource source({.day_length_ms = 300'000, .outage_length_ms = 0});
    ESP_LOGI(kTag, "Data source: fake");
    return source;
#endif
}

}  // namespace

extern "C" void app_main() {
    ESP_LOGI(kTag, "Renogy cab dashboard starting");
    if (start_display() == nullptr) {
        ESP_LOGE(kTag, "Display start failed");
        return;
    }
#if CONFIG_CAB_DASH_MODE_TEST_SCREEN
    cab_dash::show_test_screen();
#endif

    charging_data::ChargingDataSource& source = start_source();
    static cab_dash::DataHub hub(source, {
                                             .stale_after_ms = CONFIG_CAB_DASH_STALE_AFTER_S * 1000U,
                                             .offline_after_ms = CONFIG_CAB_DASH_OFFLINE_AFTER_S * 1000U,
                                         });
    hub.start(CONFIG_CAB_DASH_POLL_INTERVAL_MS);

#if CONFIG_CAB_DASH_MODE_DASHBOARD
    cab_dash::start_dashboard(hub);
#endif
}
