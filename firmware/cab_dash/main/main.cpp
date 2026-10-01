// SPDX-License-Identifier: GPL-3.0-or-later
//
// Cab dashboard app for the Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3.

#include "ble.hpp"
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

}  // namespace

extern "C" void app_main() {
    ESP_LOGI(kTag, "Renogy cab dashboard starting");
    if (start_display() == nullptr) {
        ESP_LOGE(kTag, "Display start failed");
        return;
    }
    cab_dash::show_test_screen();

    auto& source = renogy_ble::Bt2BleSource::instance();
    if (source.start(cab_dash::connect_c6_ble_controller) != ESP_OK) {
        ESP_LOGE(kTag, "BLE start failed. The dashboard continues without BT-2 data.");
    }
}
