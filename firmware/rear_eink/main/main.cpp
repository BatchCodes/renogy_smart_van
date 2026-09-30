// SPDX-License-Identifier: GPL-3.0-or-later
//
// Rear e-paper display app for the Heltec Wireless Paper.

#include <memory>

#include "board.hpp"
#include "esp_log.h"
#include "test_pattern.hpp"

namespace {

constexpr const char* kTag = "rear_eink";

}  // namespace

extern "C" void app_main() {
    ESP_LOGI(kTag, "Renogy rear e-paper display starting");

    std::unique_ptr<epaper_panel::PanelDevice> device;
    ESP_ERROR_CHECK(epaper_panel::PanelDevice::create(rear_eink::panel_pins(),
                                                      rear_eink::panel_model(),
                                                      rear_eink::detect_panel_model(), device));
    ESP_LOGI(kTag, "Panel: %s", epaper_panel::to_string(device->model()));

    rear_eink::run_test_pattern(*device);
}
