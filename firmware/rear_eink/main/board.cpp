// SPDX-License-Identifier: GPL-3.0-or-later
#include "board.hpp"

#include "sdkconfig.h"

namespace rear_eink {

epaper_panel::PanelPins panel_pins() {
    return {
        .sck = static_cast<gpio_num_t>(CONFIG_REAR_EINK_PIN_SCK),
        .mosi = static_cast<gpio_num_t>(CONFIG_REAR_EINK_PIN_MOSI),
        .cs = static_cast<gpio_num_t>(CONFIG_REAR_EINK_PIN_CS),
        .dc = static_cast<gpio_num_t>(CONFIG_REAR_EINK_PIN_DC),
        .rst = static_cast<gpio_num_t>(CONFIG_REAR_EINK_PIN_RST),
        .busy = static_cast<gpio_num_t>(CONFIG_REAR_EINK_PIN_BUSY),
        .power = static_cast<gpio_num_t>(CONFIG_REAR_EINK_PIN_POWER),
#if CONFIG_REAR_EINK_POWER_ACTIVE_LOW
        .power_active_low = true,
#else
        .power_active_low = false,
#endif
        .spi_host = SPI2_HOST,
    };
}

epaper_panel::PanelModel panel_model() {
#if CONFIG_REAR_EINK_PANEL_V1_0
    return epaper_panel::PanelModel::heltec_v1_0_ssd1680;
#elif CONFIG_REAR_EINK_PANEL_V1_1
    return epaper_panel::PanelModel::heltec_v1_1_jd79656;
#else
    return epaper_panel::PanelModel::heltec_v1_2_ssd1682;
#endif
}

bool detect_panel_model() {
#if CONFIG_REAR_EINK_PANEL_AUTO
    return true;
#else
    return false;
#endif
}

mono_gfx::Rotation rotation() {
#if CONFIG_REAR_EINK_ROTATION_90
    return mono_gfx::Rotation::clockwise_90;
#else
    return mono_gfx::Rotation::clockwise_270;
#endif
}

}  // namespace rear_eink
