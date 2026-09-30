// SPDX-License-Identifier: GPL-3.0-or-later
//
// BT-2 probe: connects to a Renogy BT-2 and logs each raw response and each decoded
// reading. It needs no display, so it runs on any ESP32 board with BLE. Use it to test
// the BLE connection and the protocol, and to capture responses for the host tests.

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "renogy_ble/bt2_ble_source.hpp"

namespace {

constexpr const char* kTag = "ble_probe";

}  // namespace

extern "C" void app_main() {
    ESP_LOGI(kTag, "Renogy BT-2 probe starting");
    auto& source = renogy_ble::Bt2BleSource::instance();
    ESP_ERROR_CHECK(source.start());

    // The source logs each reading. Poll it so a reading is not held back.
    for (;;) {
        const auto now_ms = static_cast<std::uint64_t>(esp_timer_get_time() / 1000);
        if (source.poll(now_ms)) {
            ESP_LOGI(kTag, "Reading received");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
