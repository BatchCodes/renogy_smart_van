// SPDX-License-Identifier: GPL-3.0-or-later
//
// Rear e-paper display app for the Heltec Wireless Paper.

#include <memory>

#include "board.hpp"
#include "charging_data/fake_source.hpp"
#include "charging_display.hpp"
#include "esp_log.h"
#include "sdkconfig.h"
#include "test_pattern.hpp"

namespace {

constexpr const char* kTag = "rear_eink";

// A five minute day, so the values change visibly at each refresh. With outages on,
// the source stops at 2 minutes, so the display shows the stale box from 2.5 minutes
// and the offline screen from 4 minutes, with the default timeouts.
constexpr charging_data::FakeSourceConfig kFakeSourceConfig{
    .day_length_ms = 300'000,
    .outage_start_ms = 120'000,
#if CONFIG_REAR_EINK_FAKE_OUTAGES
    .outage_length_ms = 180'000,
#else
    .outage_length_ms = 0,
#endif
};

}  // namespace

extern "C" void app_main() {
    ESP_LOGI(kTag, "Renogy rear e-paper display starting");

    std::unique_ptr<epaper_panel::PanelDevice> device;
    ESP_ERROR_CHECK(epaper_panel::PanelDevice::create(rear_eink::panel_pins(),
                                                      rear_eink::panel_model(),
                                                      rear_eink::detect_panel_model(), device));
    ESP_LOGI(kTag, "Panel: %s", epaper_panel::to_string(device->model()));

#if CONFIG_REAR_EINK_MODE_TEST_PATTERN
    rear_eink::run_test_pattern(*device);
#else
    static charging_data::FakeSource source(kFakeSourceConfig);
    ESP_LOGI(kTag, "Data source: fake");
    rear_eink::run_charging_display(*device, source);
#endif
}
