// SPDX-License-Identifier: GPL-3.0-or-later
#include "ble.hpp"

#include <cstdint>

#include "esp_check.h"
#include "esp_hosted.h"
#include "esp_hosted_bt_host_stack.h"
#include "esp_log.h"

namespace cab_dash {

namespace {

constexpr const char* kTag = "cab_ble";
constexpr std::uint32_t kControllerReadyTimeoutMs = 5000;

void log_c6_firmware_version() {
    esp_hosted_coprocessor_fwver_t version = {};
    if (esp_hosted_get_coprocessor_fwversion(&version) != ESP_OK) {
        ESP_LOGW(kTag, "C6 firmware version not available. The C6 firmware can be too old.");
        return;
    }
    ESP_LOGI(kTag, "C6 esp_hosted firmware %lu.%lu.%lu", static_cast<unsigned long>(version.major1),
             static_cast<unsigned long>(version.minor1), static_cast<unsigned long>(version.patch1));
}

}  // namespace

esp_err_t connect_c6_ble_controller() {
    log_c6_firmware_version();

    // The ESP_HOSTED_BT_HOST_STACK_CONFIG_DEFAULT() macro is a C compound literal, so
    // set the same defaults here.
    esp_hosted_bt_host_stack_cfg_t config = {};
    config.stack = ESP_HOSTED_BT_HOST_STACK_NIMBLE;
    config.bt_mac = nullptr;
    config.bring_up_controller = true;
    config.controller_ready_timeout_ms = kControllerReadyTimeoutMs;
    ESP_RETURN_ON_ERROR(esp_hosted_bt_host_stack_setup(&config), kTag,
                        "C6 BLE controller. Check that the C6 firmware has Bluetooth.");
    ESP_LOGI(kTag, "NimBLE connected to the C6 BLE controller");
    return ESP_OK;
}

}  // namespace cab_dash
