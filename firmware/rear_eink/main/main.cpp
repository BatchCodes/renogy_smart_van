// SPDX-License-Identifier: GPL-3.0-or-later
//
// Rear e-paper display app for the Heltec Wireless Paper.

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr const char* kTag = "rear_eink";

// The PRG button on the Heltec Wireless Paper is the ESP32-S3 boot pin.
constexpr gpio_num_t kButtonPin = GPIO_NUM_0;

void configure_button() {
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << kButtonPin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
}

}  // namespace

extern "C" void app_main() {
    ESP_LOGI(kTag, "Renogy rear e-paper display starting");
    configure_button();

    bool was_pressed = false;
    for (;;) {
        const bool is_pressed = gpio_get_level(kButtonPin) == 0;
        if (is_pressed != was_pressed) {
            ESP_LOGI(kTag, "PRG button %s", is_pressed ? "pressed" : "released");
            was_pressed = is_pressed;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
