// SPDX-License-Identifier: GPL-3.0-or-later
//
// SSD1680 controller with the DEPG0213BNS800 panel (Heltec Wireless Paper V1.0).
// Command sequence and partial refresh waveform from GxEPD2_213_BN by Jean-Marc Zingg
// (GPL-3.0), https://github.com/ZinggJM/GxEPD2

#include "esp_check.h"
#include "panel_base.hpp"

namespace epaper_panel {

namespace {

constexpr const char* kTag = "ssd1680";
constexpr bool kBusyLevel = true;

constexpr std::uint8_t kPartialWaveform[] = {
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
    0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
    0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
    0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,                                //
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                //
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00,                    //
};
static_assert(sizeof(kPartialWaveform) == 153);

}  // namespace

esp_err_t Ssd1680Panel::initialise() {
    io_.hardware_reset();
    ESP_RETURN_ON_ERROR(io_.wait_while_busy(kBusyLevel, kFastRefreshTimeoutMs), kTag, "reset");
    ESP_RETURN_ON_ERROR(io_.command(0x12), kTag, "software reset");
    ESP_RETURN_ON_ERROR(io_.wait_while_busy(kBusyLevel, kFastRefreshTimeoutMs), kTag, "reset");

    ESP_RETURN_ON_ERROR(io_.command(0x01, {0x27, 0x01, 0x00}), kTag, "driver output");
    ESP_RETURN_ON_ERROR(io_.command(0x11, {0x03}), kTag, "data entry mode");
    ESP_RETURN_ON_ERROR(io_.command(0x21, {0x00, 0x80}), kTag, "update control");
    ESP_RETURN_ON_ERROR(io_.command(0x44, {0x00, 0x0F}), kTag, "RAM x range");
    ESP_RETURN_ON_ERROR(io_.command(0x45, {0x00, 0x00, 0xF9, 0x00}), kTag, "RAM y range");
    ESP_RETURN_ON_ERROR(io_.command(0x4E, {0x00}), kTag, "RAM x cursor");
    return io_.command(0x4F, {0x00, 0x00});
}

esp_err_t Ssd1680Panel::show_full(std::span<const std::uint8_t> frame) {
    ESP_RETURN_ON_ERROR(initialise(), kTag, "initialise");
    ESP_RETURN_ON_ERROR(io_.command(0x3C, {0x05}), kTag, "border");
    ESP_RETURN_ON_ERROR(io_.command(0x18, {0x80}), kTag, "temperature sensor");
    ESP_RETURN_ON_ERROR(io_.command(0x24), kTag, "new image");
    ESP_RETURN_ON_ERROR(io_.data(frame), kTag, "new image");
    ESP_RETURN_ON_ERROR(io_.command(0x26), kTag, "old image");
    ESP_RETURN_ON_ERROR(io_.data(frame), kTag, "old image");
    ESP_RETURN_ON_ERROR(io_.command(0x22, {0xF8}), kTag, "power on");
    ESP_RETURN_ON_ERROR(io_.command(0x20), kTag, "power on");
    ESP_RETURN_ON_ERROR(io_.wait_while_busy(kBusyLevel, kFastRefreshTimeoutMs), kTag, "power on");
    ESP_RETURN_ON_ERROR(io_.command(0x22, {0xF4}), kTag, "full update");
    ESP_RETURN_ON_ERROR(io_.command(0x20), kTag, "full update");
    return io_.wait_while_busy(kBusyLevel, kFullRefreshTimeoutMs);
}

esp_err_t Ssd1680Panel::show_fast(std::span<const std::uint8_t> previous,
                                  std::span<const std::uint8_t> frame) {
    ESP_RETURN_ON_ERROR(initialise(), kTag, "initialise");
    ESP_RETURN_ON_ERROR(io_.command(0x3C, {0xC0}), kTag, "border");
    ESP_RETURN_ON_ERROR(io_.command(0x18, {0x80}), kTag, "temperature sensor");
    ESP_RETURN_ON_ERROR(io_.command(0x32), kTag, "waveform");
    ESP_RETURN_ON_ERROR(io_.data(kPartialWaveform), kTag, "waveform");
    ESP_RETURN_ON_ERROR(io_.command(0x26), kTag, "old image");
    ESP_RETURN_ON_ERROR(io_.data(previous), kTag, "old image");
    ESP_RETURN_ON_ERROR(io_.command(0x24), kTag, "new image");
    ESP_RETURN_ON_ERROR(io_.data(frame), kTag, "new image");
    ESP_RETURN_ON_ERROR(io_.command(0x22, {0xF8}), kTag, "power on");
    ESP_RETURN_ON_ERROR(io_.command(0x20), kTag, "power on");
    ESP_RETURN_ON_ERROR(io_.wait_while_busy(kBusyLevel, kFastRefreshTimeoutMs), kTag, "power on");
    ESP_RETURN_ON_ERROR(io_.command(0x22, {0xCC}), kTag, "fast update");
    ESP_RETURN_ON_ERROR(io_.command(0x20), kTag, "fast update");
    return io_.wait_while_busy(kBusyLevel, kFastRefreshTimeoutMs);
}

esp_err_t Ssd1680Panel::sleep() {
    ESP_RETURN_ON_ERROR(io_.command(0x22, {0x83}), kTag, "power off");
    ESP_RETURN_ON_ERROR(io_.command(0x20), kTag, "power off");
    ESP_RETURN_ON_ERROR(io_.wait_while_busy(kBusyLevel, kFastRefreshTimeoutMs), kTag, "power off");
    return io_.command(0x10, {0x01});
}

}  // namespace epaper_panel
