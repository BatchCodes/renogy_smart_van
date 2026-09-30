// SPDX-License-Identifier: GPL-3.0-or-later
//
// SSD1682 controller with the E0213A367-BW panel (Heltec Wireless Paper V1.1.1 and V1.2).
// Command sequence from the Meshtastic firmware E0213A367 driver (GPL-3.0),
// https://github.com/meshtastic/firmware, with values supplied by Heltec.

#include "esp_check.h"
#include "panel_base.hpp"

namespace epaper_panel {

namespace {

constexpr const char* kTag = "ssd1682";
constexpr bool kBusyLevel = true;

}  // namespace

esp_err_t Ssd1682Panel::initialise(RefreshMode mode) {
    io_.hardware_reset();
    ESP_RETURN_ON_ERROR(io_.wait_while_busy(kBusyLevel, kFastRefreshTimeoutMs), kTag, "reset");
    ESP_RETURN_ON_ERROR(io_.command(0x12), kTag, "software reset");
    ESP_RETURN_ON_ERROR(io_.wait_while_busy(kBusyLevel, kFastRefreshTimeoutMs), kTag, "reset");

    // This controller takes single-byte RAM addresses.
    ESP_RETURN_ON_ERROR(io_.command(0x11, {0x03}), kTag, "data entry mode");
    ESP_RETURN_ON_ERROR(io_.command(0x44, {0x00, 0x0F}), kTag, "RAM x range");
    ESP_RETURN_ON_ERROR(io_.command(0x45, {0x00, 0xF9}), kTag, "RAM y range");
    ESP_RETURN_ON_ERROR(io_.command(0x4E, {0x00}), kTag, "RAM x cursor");
    ESP_RETURN_ON_ERROR(io_.command(0x4F, {0x00}), kTag, "RAM y cursor");
    ESP_RETURN_ON_ERROR(io_.command(0x01, {0xF9, 0x00}), kTag, "driver output");
    ESP_RETURN_ON_ERROR(io_.command(0x37, {0x40, 0x80, 0x03, 0x0E}), kTag, "display option");
    const std::uint8_t border = mode == RefreshMode::fast ? 0x81 : 0x01;
    ESP_RETURN_ON_ERROR(io_.command(0x3C, {border}), kTag, "border");
    return io_.wait_while_busy(kBusyLevel, kFastRefreshTimeoutMs);
}

esp_err_t Ssd1682Panel::show_full(std::span<const std::uint8_t> frame) {
    ESP_RETURN_ON_ERROR(initialise(RefreshMode::full), kTag, "initialise");
    ESP_RETURN_ON_ERROR(io_.command(0x24), kTag, "new image");
    ESP_RETURN_ON_ERROR(io_.data(frame), kTag, "new image");
    ESP_RETURN_ON_ERROR(io_.command(0x26), kTag, "old image");
    ESP_RETURN_ON_ERROR(io_.data(frame), kTag, "old image");
    ESP_RETURN_ON_ERROR(io_.command(0x22, {0xF7}), kTag, "full update");
    ESP_RETURN_ON_ERROR(io_.command(0x20), kTag, "full update");
    return io_.wait_while_busy(kBusyLevel, kFullRefreshTimeoutMs);
}

esp_err_t Ssd1682Panel::show_fast(std::span<const std::uint8_t> previous,
                                  std::span<const std::uint8_t> frame) {
    ESP_RETURN_ON_ERROR(initialise(RefreshMode::fast), kTag, "initialise");
    ESP_RETURN_ON_ERROR(io_.command(0x26), kTag, "old image");
    ESP_RETURN_ON_ERROR(io_.data(previous), kTag, "old image");
    ESP_RETURN_ON_ERROR(io_.command(0x24), kTag, "new image");
    ESP_RETURN_ON_ERROR(io_.data(frame), kTag, "new image");
    ESP_RETURN_ON_ERROR(io_.command(0x22, {0xFF}), kTag, "fast update");
    ESP_RETURN_ON_ERROR(io_.command(0x20), kTag, "fast update");
    return io_.wait_while_busy(kBusyLevel, kFastRefreshTimeoutMs);
}

esp_err_t Ssd1682Panel::sleep() {
    // Mode 1 deep sleep. The image memory is lost, but PanelBase resends the old
    // image on the next fast refresh.
    return io_.command(0x10, {0x01});
}

}  // namespace epaper_panel
