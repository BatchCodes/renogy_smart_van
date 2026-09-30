// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <initializer_list>
#include <span>

#include "epaper_panel/panel.hpp"

namespace epaper_panel {

// SPI and GPIO access to the panel controller. Command bytes go with DC low, data
// bytes with DC high.
class PanelIo {
public:
    ~PanelIo();

    static esp_err_t create(const PanelPins& pins, std::unique_ptr<PanelIo>& out);

    esp_err_t command(std::uint8_t command);
    esp_err_t data(std::span<const std::uint8_t> bytes);
    esp_err_t command(std::uint8_t command, std::initializer_list<std::uint8_t> bytes);

    // Pulls RST low for 10 ms, then releases it and waits 10 ms.
    void hardware_reset();

    // Holds RST low and returns the BUSY level. The BUSY level while a controller is
    // held in reset shows which controller it is.
    bool busy_level_during_reset();

    // Waits while BUSY is at busy_level. Returns ESP_ERR_TIMEOUT after timeout_ms.
    esp_err_t wait_while_busy(bool busy_level, std::uint32_t timeout_ms);

private:
    explicit PanelIo(const PanelPins& pins) : pins_(pins) {}
    esp_err_t transmit(const std::uint8_t* bytes, std::size_t length, bool is_data);

    PanelPins pins_;
    spi_device_handle_t device_ = nullptr;
    bool bus_initialised_ = false;
};

}  // namespace epaper_panel
