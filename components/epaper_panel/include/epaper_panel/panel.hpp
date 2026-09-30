// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

namespace epaper_panel {

// GPIO pins for the panel. The board app sets them.
struct PanelPins {
    gpio_num_t sck;
    gpio_num_t mosi;
    gpio_num_t cs;
    gpio_num_t dc;
    gpio_num_t rst;
    gpio_num_t busy;
    gpio_num_t power;  // GPIO_NUM_NC if the panel power is not switched.
    bool power_active_low;
    spi_host_device_t spi_host;
};

// Panel and controller combinations of the Heltec Wireless Paper.
enum class PanelModel : std::uint8_t {
    heltec_v1_0_ssd1680,   // DEPG0213BNS800
    heltec_v1_1_jd79656,   // LCMEN2R13EFC1, green protective film
    heltec_v1_2_ssd1682,   // E0213A367-BW, V1.1.1 and V1.2
};

enum class RefreshMode : std::uint8_t {
    full,  // Slow, with flashing. Removes ghosting.
    fast,  // Quicker, no flashing. Ghosting builds up over many refreshes.
};

class PanelIo;

// One e-paper panel controller. Frames use the mono_gfx::FrameBuffer native layout:
// 250 rows of 16 bytes, most significant bit first, bit value 1 = white.
class Panel {
public:
    virtual ~Panel() = default;

    [[nodiscard]] virtual const char* name() const = 0;

    // Shows the frame and waits until the refresh is complete.
    virtual esp_err_t show(std::span<const std::uint8_t> frame, RefreshMode mode) = 0;

    // Low power mode. The next show() wakes the controller.
    virtual esp_err_t sleep() = 0;
};

// Owns the SPI bus and the GPIO pins, and the controller driver.
class PanelDevice {
public:
    PanelDevice(const PanelDevice&) = delete;
    PanelDevice& operator=(const PanelDevice&) = delete;
    ~PanelDevice();

    // Sets up the bus, powers the panel and creates the driver for the model. With
    // detect = true, the model comes from the BUSY pin level during reset instead. This
    // tells V1.1 and V1.1.1/V1.2 apart. V1.0 cannot be detected, so select it.
    static esp_err_t create(const PanelPins& pins, PanelModel model, bool detect,
                            std::unique_ptr<PanelDevice>& out);

    [[nodiscard]] Panel& panel() { return *panel_; }
    [[nodiscard]] PanelModel model() const { return model_; }

private:
    PanelDevice(std::unique_ptr<PanelIo> io, std::unique_ptr<Panel> panel, PanelModel model);

    std::unique_ptr<PanelIo> io_;
    std::unique_ptr<Panel> panel_;
    PanelModel model_;
};

[[nodiscard]] const char* to_string(PanelModel model);

}  // namespace epaper_panel
